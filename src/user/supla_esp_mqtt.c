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

#include "supla_esp_mqtt.h"

#include <espconn.h>
#include <osapi.h>

#include "mqtt.h"
#include "supla-dev/log.h"
#include "supla_esp_cfg.h"
#include "supla_esp_state.h"
#include "supla_esp_wifi.h"

#define RECONNECT_RETRY_TIME_MS 5000

typedef struct {
  uint8 started;
  ETSTimer iterate_timer;
  struct espconn esp_conn;
  esp_tcp esptcp;
  ip_addr_t ipaddr;
  struct mqtt_client client;
  unsigned _supla_int64_t wait_until_ms;
  uint32 ip;
} _supla_esp_mqtt_vars_t;

_supla_esp_mqtt_vars_t *supla_esp_mqtt_vars = NULL;

void ICACHE_FLASH_ATTR supla_esp_mqtt_init(void) {
  supla_esp_mqtt_vars = malloc(sizeof(_supla_esp_mqtt_vars_t));
  memset(supla_esp_mqtt_vars, 0, sizeof(_supla_esp_mqtt_vars_t));
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_before_system_restart(void) {}

void ICACHE_FLASH_ATTR supla_esp_mqtt_iterate(void *ptr) {
  if (supla_esp_mqtt_vars->wait_until_ms &&
      supla_esp_mqtt_vars->wait_until_ms > uptime_msec()) {
    return;
  }

  mqtt_sync(&supla_esp_mqtt_vars->client);

  if (mqtt_mq_length(&supla_esp_mqtt_vars->client.mq) > 0) {
    mqtt_mq_clean(&supla_esp_mqtt_vars->client.mq);
  }
}

ssize_t mqtt_pal_sendall(mqtt_pal_socket_handle methods, const void *buf,
                         size_t len, int flags) {
  supla_log(LOG_DEBUG, "sendall=%i", len);
  return MQTT_ERROR_SOCKET_ERROR;
}

ssize_t mqtt_pal_recvall(mqtt_pal_socket_handle methods, void *buf,
                         size_t bufsz, int flags) {
  return MQTT_ERROR_SOCKET_ERROR;
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_conn_recv_cb(void *arg, char *pdata,
                                                   unsigned short len) {}

void ICACHE_FLASH_ATTR supla_esp_mqtt_on_message_received(
    void **adapter_instance, struct mqtt_response_publish *message) {}

sint8 ICACHE_FLASH_ATTR supla_esp_mqtt_espconn_connect(void) {
  if (supla_esp_cfg.Flags & CFG_FLAG_MQTT_TLS) {
    return espconn_secure_connect(&supla_esp_mqtt_vars->esp_conn);
  } else {
    return espconn_connect(&supla_esp_mqtt_vars->esp_conn);
  }
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_espconn_diconnect(void) {
  if (supla_esp_cfg.Flags & CFG_FLAG_MQTT_TLS) {
    espconn_secure_disconnect(&supla_esp_mqtt_vars->esp_conn);
  } else {
    espconn_disconnect(&supla_esp_mqtt_vars->esp_conn);
  }

  memset(&supla_esp_mqtt_vars->esp_conn, 0, sizeof(struct espconn));
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_conn_on_connect(void *arg) {
  supla_esp_mqtt_vars->wait_until_ms = 0;
  supla_log(LOG_DEBUG, "on connect");
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_conn_on_disconnect(void *arg) {
  supla_esp_mqtt_vars->wait_until_ms = 0;
  supla_log(LOG_DEBUG, "on disconnect");
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_reconnect(struct mqtt_client *client,
                                                void **state) {
  if (client->error != MQTT_ERROR_INITIAL_RECONNECT &&
      supla_esp_mqtt_vars->wait_until_ms &&
      supla_esp_mqtt_vars->wait_until_ms > uptime_msec()) {
    return;
  }

  supla_esp_mqtt_vars->wait_until_ms = uptime_msec() + RECONNECT_RETRY_TIME_MS;

  supla_esp_mqtt_espconn_diconnect();

  supla_log(LOG_INFO, "Connecting to %s:%i", supla_esp_cfg.Server,
            supla_esp_cfg.Port);

  supla_esp_mqtt_vars->esp_conn.proto.tcp = &supla_esp_mqtt_vars->esptcp;
  supla_esp_mqtt_vars->esp_conn.type = ESPCONN_TCP;
  supla_esp_mqtt_vars->esp_conn.state = ESPCONN_NONE;

  supla_esp_mqtt_vars->esp_conn.proto.tcp->local_port = espconn_port();
  supla_esp_mqtt_vars->esp_conn.proto.tcp->remote_port = supla_esp_cfg.Port;
  os_memcpy(supla_esp_mqtt_vars->esp_conn.proto.tcp->remote_ip,
            &supla_esp_mqtt_vars->ip, 4);

  espconn_regist_recvcb(&supla_esp_mqtt_vars->esp_conn,
                        supla_esp_mqtt_conn_recv_cb);
  espconn_regist_connectcb(&supla_esp_mqtt_vars->esp_conn,
                           supla_esp_mqtt_conn_on_connect);
  espconn_regist_disconcb(&supla_esp_mqtt_vars->esp_conn,
                          supla_esp_mqtt_conn_on_disconnect);

  sint8 ret = supla_esp_mqtt_espconn_connect();
  supla_log(LOG_DEBUG, "ret=%i", ret);
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_dns__found(ip_addr_t *ip) {
  if (ip == NULL) {
    supla_esp_set_state(LOG_NOTICE, "Domain not found.");
    return;
  }

  os_memcpy(&supla_esp_mqtt_vars->ip, ip, 4);
  os_timer_disarm(&supla_esp_mqtt_vars->iterate_timer);

  mqtt_init_reconnect(&supla_esp_mqtt_vars->client, supla_esp_mqtt_reconnect,
                      &supla_esp_mqtt_vars, supla_esp_mqtt_on_message_received);

  os_timer_setfn(&supla_esp_mqtt_vars->iterate_timer,
                 (os_timer_func_t *)supla_esp_mqtt_iterate, NULL);
  os_timer_arm(&supla_esp_mqtt_vars->iterate_timer, 100, 1);
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_dns_found_cb(const char *name,
                                                   ip_addr_t *ip, void *arg) {
#ifndef ADDITIONAL_DNS_CLIENT_DISABLED
  if (ip == NULL) {
    supla_esp_dns_resolve(supla_esp_cfg.Server, supla_esp_mqtt_dns__found);
    return;
  }
#endif /*ADDITIONAL_DNS_CLIENT_DISABLED*/

  supla_esp_mqtt_dns__found(ip);
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_on_wifi_status_changed(uint8 status) {
  if (!supla_esp_mqtt_vars->started || status != STATION_GOT_IP) {
    return;
  }

  uint32_t _ip = ipaddr_addr(supla_esp_cfg.Server);

  if (_ip == -1) {
    espconn_gethostbyname(&supla_esp_mqtt_vars->esp_conn, supla_esp_cfg.Server,
                          &supla_esp_mqtt_vars->ipaddr,
                          supla_esp_mqtt_dns_found_cb);
  } else {
    supla_esp_mqtt_dns_found_cb(supla_esp_cfg.Server, (ip_addr_t *)&_ip, NULL);
  }
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_client_start(void) {
  supla_esp_mqtt_client_stop();

  if (!supla_esp_mqtt_vars) {
    return;
  }

  supla_esp_mqtt_vars->started = 1;
  supla_esp_wifi_station_connect(supla_esp_mqtt_on_wifi_status_changed);
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_client_stop(void) {
  if (!supla_esp_mqtt_vars) {
    return;
  }

  os_timer_disarm(&supla_esp_mqtt_vars->iterate_timer);
  supla_esp_mqtt_vars->started = 0;
  mqtt_disconnect(&supla_esp_mqtt_vars->client);
  supla_esp_mqtt_espconn_diconnect();
}
