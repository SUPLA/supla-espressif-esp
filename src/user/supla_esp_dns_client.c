/*
 Copyright (C) AC SOFTWARE SP. Z O.O.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "supla_esp_dns_client.h"
#include "espmissingincludes.h"
#include "supla-dev/log.h"

#include <espconn.h>
#include <osapi.h>
#include <string.h>

// https://tools.ietf.org/html/rfc1035
// https://www.zytrax.com/books/dns/ch15

#ifndef ADDITIONAL_DNS_CLIENT_DISABLED

#define DNS_SERVER_COUNT 4
uint8 dns_server_ip[DNS_SERVER_COUNT][4] = {
    {8, 8, 8, 8}, {1, 1, 1, 1}, {8, 8, 4, 4}, {1, 0, 0, 1}};

#define DOMAIN_MAX_LEN 63

#define IP_MIN_LEN (7)
#define DOMAIN_MIN_LEN (4)

#define TYPE_A 1
#define CLASS_IN 1

#define RCODE_NO_ERROR 0

#define DNS_TIMEOUT_PER_REQUEST_MS 5000
#define RETRY_DELAY_MS 200

#pragma pack(push, 1)

typedef struct {
  unsigned short ID;  // IDentifier

  unsigned char RD : 1;      // Recursion Desired
  unsigned char TC : 1;      // TrunCation
  unsigned char AA : 1;      // Authoritative Answer
  unsigned char OPCODE : 4;  // Kind of query
  unsigned char QR : 1;      // Query or Response

  unsigned char RCODE : 4;  // Response CODE
  unsigned char Z : 3;      // Reserved for future use
  unsigned char RA : 1;     // Recursion Available

  unsigned short QDCOUNT;  // Number of entries in the question section
  unsigned short ANCOUNT;  // Number of resource records in the answer section.
  unsigned short NSCOUNT;  // Number of name server resource records in the
                           // authority records section
  unsigned short
      ARCOUNT;  // Number of resource records in the additional records section

} _t_dns_header;

typedef struct {
  unsigned short TYPE;
  unsigned short CLASS;
} _t_dns_question_suffix;

typedef struct {
  unsigned short TYPE;
  unsigned short CLASS;
  unsigned int TTL;
  unsigned short RDLENGTH;
} _t_dns_answer_suffix;

typedef struct {
  char *data;
  unsigned short data_len;
  _t_dns_header *header;
  char *encoded_domain;
  unsigned char encoded_domain_len;
  _t_dns_question_suffix *question_suffix;
} _t_dns_request;

#pragma pack(pop)

typedef struct {
  uint8 success : 1;
  ip_addr_t result_ipv4;
  _dns_query_result_cb dns_query_result_cb;
  struct espconn conn;
  esp_tcp ESPTCP;
  _t_dns_request request;
  ETSTimer timeout_timer;
  ETSTimer retry_timer;
  uint8 try_counter;
} _t_dns_client_vars;

_t_dns_client_vars dns_client_vars;

uint16 DNS_ICACHE_FLASH_ATTR htons(uint16 n) {
  return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

#define ntohs htons

void DNS_ICACHE_FLASH_ATTR supla_esp_dns_encode_name(const char *name,
                                                     uint8 len, char *dest) {
  unsigned char last_pos = 0;

  if (len == 0 || name == NULL || dest == NULL) {
    return;
  }

  for (unsigned char a = 0; a < len + 1; a++) {
    if (name[a] == '.' || name[a] == 0) {
      dest[last_pos] = a - last_pos;
      if (dest[last_pos] == 0) {
        return;
      }
      memcpy(&dest[last_pos + 1], &name[last_pos], a - last_pos);
      last_pos = a + 1;
    }
  }

  dest[len + 1] = 0;
}

void DNS_ICACHE_FLASH_ATTR supla_esp_dns_request_release(void) {
  if (dns_client_vars.request.data != NULL) {
    free(dns_client_vars.request.data);
    dns_client_vars.request.data = NULL;
  }

  dns_client_vars.request.header = NULL;
  dns_client_vars.request.encoded_domain = NULL;
  dns_client_vars.request.question_suffix = NULL;
}

void DNS_ICACHE_FLASH_ATTR supla_esp_dns_retry_cb(void *ptr);

void DNS_ICACHE_FLASH_ATTR supla_esp_dns_result() {
  if (dns_client_vars.success == 0 &&
      dns_client_vars.try_counter < DNS_SERVER_COUNT) {
    os_timer_disarm(&dns_client_vars.retry_timer);
    os_timer_setfn(&dns_client_vars.retry_timer,
                   (os_timer_func_t *)supla_esp_dns_retry_cb, NULL);
    os_timer_arm(&dns_client_vars.retry_timer, RETRY_DELAY_MS, 0);

  } else {
    supla_esp_dns_request_release();

    if (dns_client_vars.dns_query_result_cb) {
      dns_client_vars.dns_query_result_cb(
          dns_client_vars.success ? &dns_client_vars.result_ipv4 : NULL);
      dns_client_vars.dns_query_result_cb = NULL;
    }
  }
}

void DNS_ICACHE_FLASH_ATTR supla_esp_dns_recv_cb(void *arg, char *pdata,
                                                 unsigned short len) {
  if (len < dns_client_vars.request.data_len ||
      ntohs(*(unsigned short *)pdata) != len - sizeof(unsigned short)) {
    supla_esp_dns_result();
    return;
  }

  _t_dns_header *header = (_t_dns_header *)&pdata[sizeof(unsigned short)];
  header->ANCOUNT = ntohs(header->ANCOUNT);

  if (header->RCODE != RCODE_NO_ERROR || header->ANCOUNT < 1) {
    supla_esp_dns_result();
    return;
  }

  pdata += dns_client_vars.request.data_len;
  len -= dns_client_vars.request.data_len;

  unsigned short a = 0;

  for (a = 0; a < len; a++) {
    if (((unsigned char)pdata[a]) >> 6 == 3) {
      a += 2;
      break;
    } else if (pdata[a] == 0) {
      a++;
      break;
    }
  }

  if (a + sizeof(_t_dns_answer_suffix) >= len) {
    supla_esp_dns_result();
    return;
  }

  _t_dns_answer_suffix *suffix = (_t_dns_answer_suffix *)&pdata[a];
  suffix->TYPE = ntohs(suffix->TYPE);
  suffix->CLASS = ntohs(suffix->CLASS);
  suffix->RDLENGTH = ntohs(suffix->RDLENGTH);

  if (suffix->TYPE != TYPE_A || suffix->CLASS != CLASS_IN ||
      suffix->RDLENGTH != sizeof(unsigned int) ||
      a + sizeof(_t_dns_answer_suffix) + suffix->RDLENGTH > len) {
    return;
  }

  memcpy(&dns_client_vars.result_ipv4,
         (ip_addr_t *)&pdata[a + sizeof(_t_dns_answer_suffix)],
         sizeof(ip_addr_t));
  dns_client_vars.success = 1;
  espconn_disconnect(&dns_client_vars.conn);
}

void DNS_ICACHE_FLASH_ATTR supla_esp_dns_disconnect_cb(void *arg) {
  supla_esp_dns_result();
}

void DNS_ICACHE_FLASH_ATTR supla_esp_dns_connect_cb(void *arg) {
  if (espconn_sent(&dns_client_vars.conn,
                   (unsigned char *)dns_client_vars.request.data,
                   dns_client_vars.request.data_len) != 0) {
    espconn_disconnect(&dns_client_vars.conn);
    supla_esp_dns_result();
  }
}

void DNS_ICACHE_FLASH_ATTR supla_esp_dns_timeout_cb(void *ptr) {
  espconn_disconnect(&dns_client_vars.conn);
  supla_esp_dns_result();
}

void DNS_ICACHE_FLASH_ATTR supla_esp_dns__resolve(void) {
  dns_client_vars.success = 0;
  memset(&dns_client_vars.result_ipv4, 0, sizeof(ip_addr_t));

  os_timer_disarm(&dns_client_vars.timeout_timer);
  os_timer_setfn(&dns_client_vars.timeout_timer,
                 (os_timer_func_t *)supla_esp_dns_timeout_cb, NULL);
  os_timer_arm(&dns_client_vars.timeout_timer, DNS_TIMEOUT_PER_REQUEST_MS, 0);

  espconn_disconnect(&dns_client_vars.conn);
  dns_client_vars.conn.proto.tcp = &dns_client_vars.ESPTCP;
  dns_client_vars.conn.type = ESPCONN_TCP;
  dns_client_vars.conn.state = ESPCONN_NONE;

  dns_client_vars.try_counter++;

  memcpy(dns_client_vars.conn.proto.tcp->remote_ip,
         &dns_server_ip[(dns_client_vars.try_counter - 1) % DNS_SERVER_COUNT],
         4);

  dns_client_vars.conn.proto.tcp->local_port = espconn_port();
  dns_client_vars.conn.proto.tcp->remote_port = 53;

  espconn_regist_recvcb(&dns_client_vars.conn, supla_esp_dns_recv_cb);
  espconn_regist_connectcb(&dns_client_vars.conn, supla_esp_dns_connect_cb);
  espconn_regist_disconcb(&dns_client_vars.conn, supla_esp_dns_disconnect_cb);

  espconn_connect(&dns_client_vars.conn);
}

void DNS_ICACHE_FLASH_ATTR supla_esp_dns_retry_cb(void *ptr) {
  supla_esp_dns__resolve();
}

void DNS_ICACHE_FLASH_ATTR supla_esp_dns_resolve(
    const char *domain, _dns_query_result_cb dns_query_result_cb) {
  os_timer_disarm(&dns_client_vars.timeout_timer);
  os_timer_disarm(&dns_client_vars.retry_timer);

  supla_esp_dns_request_release();
  dns_client_vars.dns_query_result_cb = dns_query_result_cb;

  if (domain == NULL) {
    return;
  }

  unsigned short domain_len = strnlen(domain, DOMAIN_MAX_LEN);

  if (strnlen(domain, DOMAIN_MAX_LEN) < DOMAIN_MIN_LEN) {
    supla_esp_dns_result();
    return;
  }

  dns_client_vars.request.data_len = sizeof(unsigned short) +
                                     sizeof(_t_dns_header) + domain_len + 2 +
                                     sizeof(_t_dns_question_suffix);

  dns_client_vars.request.data = malloc(dns_client_vars.request.data_len);

  if (dns_client_vars.request.data == NULL) {
    supla_esp_dns_result();
    return;
  }

  memset(dns_client_vars.request.data, 0, dns_client_vars.request.data_len);

  dns_client_vars.request.header =
      (_t_dns_header *)&dns_client_vars.request.data[sizeof(unsigned short)];
  dns_client_vars.request.encoded_domain =
      &((char *)dns_client_vars.request.header)[sizeof(_t_dns_header)];

  supla_esp_dns_encode_name(domain, domain_len,
                            dns_client_vars.request.encoded_domain);

  dns_client_vars.request.question_suffix =
      (_t_dns_question_suffix *)&dns_client_vars.request
          .encoded_domain[domain_len + 2];

  dns_client_vars.try_counter = 0;

  *(unsigned short *)dns_client_vars.request.data = htons(
      (dns_client_vars.request.data_len - sizeof(unsigned short)) & 0xffffu);

  dns_client_vars.request.header->ID = 1;
  dns_client_vars.request.header->RD = 1;
  dns_client_vars.request.header->QDCOUNT = htons(1);

  dns_client_vars.request.question_suffix->TYPE = htons(TYPE_A);
  dns_client_vars.request.question_suffix->CLASS = htons(CLASS_IN);

  supla_esp_dns__resolve();
}

void DNS_ICACHE_FLASH_ATTR supla_esp_dns_client_init(void) {
  memset(&dns_client_vars, 0, sizeof(_t_dns_client_vars));
}

#endif /*ADDITIONAL_DNS_CLIENT_DISABLED*/
