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

#ifdef MQTT_SUPPORT_ENABLED

#include <espconn.h>
#include <osapi.h>

#include "mqtt.h"
#include "supla-dev/log.h"
#include "supla_esp_cfg.h"
#include "supla_esp_gpio.h"
#include "supla_esp_state.h"
#include "supla_esp_wifi.h"

#define RECONNECT_RETRY_TIME_MS 5000
#define CONN_STATUS_UNKNOWN 0
#define CONN_STATUS_CONNECTING 1
#define CONN_STATUS_CONNECTED 2
#define CONN_STATUS_READY 3
#define CONN_STATUS_DISCONNECTED 4

#define MQTT_CLIENTID_MAX_SIZE 23
#define MQTT_KEEP_ALIVE_SEC 32

typedef struct {
  uint8 started;
  ETSTimer iterate_timer;
  uint8 status;
  struct espconn esp_conn;
  esp_tcp esptcp;
  ip_addr_t ipaddr;
  struct mqtt_client client;
  unsigned _supla_int64_t wait_until_ms;
  uint32 ip;

  unsigned short recv_len;
  uint8 recvbuf[MQTT_RECVBUF_SIZE];
  uint8 sendbuf[MQTT_SENDBUF_SIZE] __attribute__((aligned(4)));

  uint8 subscribe_idx;
  uint8 publish_idx[32];

} _supla_esp_mqtt_vars_t;

_supla_esp_mqtt_vars_t *supla_esp_mqtt_vars = NULL;

void ICACHE_FLASH_ATTR supla_esp_mqtt_init(void) {
  supla_esp_mqtt_vars = malloc(sizeof(_supla_esp_mqtt_vars_t));
  memset(supla_esp_mqtt_vars, 0, sizeof(_supla_esp_mqtt_vars_t));
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_before_system_restart(void) {}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_subscribe(void) {
  if (!supla_esp_mqtt_vars->subscribe_idx) {
    return 0;
  }
  char *topic_name = NULL;
  if (!supla_esp_board_mqtt_get_subscription_topic(
          &topic_name, supla_esp_mqtt_vars->subscribe_idx) ||
      topic_name == NULL) {
    supla_esp_mqtt_vars->subscribe_idx = 0;
  } else {
    enum MQTTErrors r = mqtt_subscribe(&supla_esp_mqtt_vars->client, topic_name,
                                       supla_esp_cfg.MqttQoS);

    if (r == MQTT_OK) {
      supla_esp_mqtt_vars->subscribe_idx++;
    } else {
      if (r == MQTT_ERROR_SEND_BUFFER_IS_FULL) {
        supla_esp_mqtt_vars->client.error = MQTT_OK;
      }
      supla_log(LOG_DEBUG, "MQTT Subscribe Error %i/%i. Retry...", r,
                MQTT_ERROR_SEND_BUFFER_IS_FULL);
    }
  }

  if (topic_name) {
    free(topic_name);
  }

  return 1;
}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_publish(void) {
  uint8 ret = 0;

  do {
    uint8 idx = 0;
    uint8 bit = 0;
    for (uint8 a = 0; a < 255; a++) {
      bit = 1 << (a % 8);
      if (supla_esp_mqtt_vars->publish_idx[a / 8] & bit) {
        idx = a + 1;
        break;
      }
    }

    if (!idx) {
      return 0;
    }

    char *topic_name = NULL;
    void *message = NULL;
    size_t message_size = 0;

    if (!supla_esp_board_mqtt_get_topic_for_publication(&topic_name, &message,
                                                        &message_size, idx) ||
        topic_name == NULL) {
      supla_esp_mqtt_vars->publish_idx[(idx - 1) / 8] &= ~bit;

    } else {
      ret = 1;
      uint8 publish_flags = 0;

      switch (supla_esp_cfg.MqttQoS) {
        case 1:
          publish_flags = MQTT_PUBLISH_QOS_1;
          break;
        case 2:
          publish_flags = MQTT_PUBLISH_QOS_2;
          break;
        default:
          publish_flags = MQTT_PUBLISH_QOS_0;
          break;
      }

      if (!(supla_esp_cfg.Flags & CFG_FLAG_MQTT_NO_RETAIN)) {
        publish_flags |= MQTT_PUBLISH_RETAIN;
      }

      enum MQTTErrors r = mqtt_publish(&supla_esp_mqtt_vars->client, topic_name,
                                       message, message_size, publish_flags);

      if (r == MQTT_OK) {
        supla_esp_mqtt_vars->publish_idx[(idx - 1) / 8] &= ~bit;
      } else {
        if (r == MQTT_ERROR_SEND_BUFFER_IS_FULL) {
          supla_esp_mqtt_vars->client.error = MQTT_OK;
        }
        supla_log(LOG_DEBUG, "MQTT Publish Error %i/%i. Retry...", r,
                  MQTT_ERROR_SEND_BUFFER_IS_FULL);
      }
    }

    if (topic_name) {
      free(topic_name);
    }

    if (message) {
      free(message);
    }

  } while (!ret);

  return 1;
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_iterate(void *ptr) {
  mqtt_sync(&supla_esp_mqtt_vars->client);

  if (mqtt_mq_length(&supla_esp_mqtt_vars->client.mq) > 0) {
    mqtt_mq_clean(&supla_esp_mqtt_vars->client.mq);
  }

  if (supla_esp_mqtt_vars->status != CONN_STATUS_READY) {
    return;
  }

  if (supla_esp_mqtt_subscribe()) {
    return;
  }

  if (supla_esp_mqtt_publish()) {
    return;
  }
}

sint8 supla_esp_mqtt_conn_sent(uint8 *psent, uint16 length) {
  if (supla_esp_cfg.Flags & CFG_FLAG_MQTT_TLS) {
    return espconn_secure_sent(&supla_esp_mqtt_vars->esp_conn, psent, length);
  } else {
    return espconn_sent(&supla_esp_mqtt_vars->esp_conn, psent, length);
  }
}

ssize_t mqtt_pal_sendall(mqtt_pal_socket_handle methods, const void *buf,
                         size_t len, int flags) {
  if (supla_esp_mqtt_vars->status != CONN_STATUS_CONNECTED &&
      supla_esp_mqtt_vars->status != CONN_STATUS_READY) {
    return MQTT_ERROR_SOCKET_ERROR;
  }

  sint8 r = supla_esp_mqtt_conn_sent((uint8 *)buf, len);
  if (r == 0) {
    return len;
  } else if (r == ESPCONN_INPROGRESS) {
    return 0;
  }

  return MQTT_ERROR_SOCKET_ERROR;
}

ssize_t mqtt_pal_recvall(mqtt_pal_socket_handle methods, void *buf,
                         size_t bufsz, int flags) {
  ssize_t result = supla_esp_mqtt_vars->recv_len;
  supla_esp_mqtt_vars->recv_len = 0;
  if (result > 0) {
    return result;
  }

  return supla_esp_mqtt_vars->status == CONN_STATUS_CONNECTED ||
                 supla_esp_mqtt_vars->status == CONN_STATUS_CONNECTING ||
                 supla_esp_mqtt_vars->status == CONN_STATUS_READY
             ? 0
             : MQTT_ERROR_SOCKET_ERROR;
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_conn_recv_cb(void *arg, char *pdata,
                                                   unsigned short len) {
  // The buffer filling is based on the assumption that
  // supla_esp_mqtt_conn_recv_cb callback cannot interrupt mqtt_sync(). If it
  // turns out otherwise, you have to use an additional intermediate buffer.

  if (len + supla_esp_mqtt_vars->recv_len > MQTT_RECVBUF_SIZE) {
    supla_log(LOG_DEBUG, "MQTT recv buffer is too small! %i",
              len + supla_esp_mqtt_vars->recv_len - MQTT_RECVBUF_SIZE);
    return;
  }

  memcpy(&supla_esp_mqtt_vars->recvbuf[supla_esp_mqtt_vars->recv_len], pdata,
         len);
  supla_esp_mqtt_vars->recv_len += len;
}

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

  supla_esp_mqtt_vars->status = CONN_STATUS_DISCONNECTED;
  memset(&supla_esp_mqtt_vars->esp_conn, 0, sizeof(struct espconn));
  supla_esp_mqtt_vars->recv_len = 0;
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_conn_on_connect(void *arg) {
  supla_esp_mqtt_vars->status = CONN_STATUS_CONNECTED;
  supla_log(LOG_DEBUG, "on_connect");

  supla_esp_mqtt_vars->subscribe_idx = 0;
  memset(supla_esp_mqtt_vars->publish_idx, 0,
         sizeof(supla_esp_mqtt_vars->publish_idx));

  mqtt_reinit(&supla_esp_mqtt_vars->client, 0, supla_esp_mqtt_vars->sendbuf,
              sizeof(supla_esp_mqtt_vars->sendbuf),
              supla_esp_mqtt_vars->recvbuf,
              sizeof(supla_esp_mqtt_vars->recvbuf));

  char clientId[MQTT_CLIENTID_MAX_SIZE];

  ets_snprintf(
      clientId, sizeof(clientId),
      "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
      (unsigned char)supla_esp_cfg.GUID[0],
      (unsigned char)supla_esp_cfg.GUID[1],
      (unsigned char)supla_esp_cfg.GUID[2],
      (unsigned char)supla_esp_cfg.GUID[3],
      (unsigned char)supla_esp_cfg.GUID[4],
      (unsigned char)supla_esp_cfg.GUID[5],
      (unsigned char)supla_esp_cfg.GUID[6],
      (unsigned char)supla_esp_cfg.GUID[7],
      (unsigned char)supla_esp_cfg.GUID[8],
      (unsigned char)supla_esp_cfg.GUID[9],
      (unsigned char)supla_esp_cfg.GUID[10],
      (unsigned char)supla_esp_cfg.GUID[11],
      (unsigned char)supla_esp_cfg.GUID[12],
      (unsigned char)supla_esp_cfg.GUID[13],
      (unsigned char)supla_esp_cfg.GUID[14],
      (unsigned char)supla_esp_cfg.GUID[15]);

  if (MQTT_OK ==
      mqtt_connect(&supla_esp_mqtt_vars->client, clientId, NULL, NULL, 0,
                   supla_esp_cfg.Username, supla_esp_cfg.Password,
                   MQTT_CONNECT_CLEAN_SESSION, MQTT_KEEP_ALIVE_SEC)) {
    supla_esp_gpio_state_connected();
    supla_esp_mqtt_vars->status = CONN_STATUS_READY;
    supla_esp_mqtt_wants_subscribe();
    supla_esp_mqtt_wants_publish(MQTT_WANTS_PUBLISH_ALL);
  }
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_conn_on_disconnect(void *arg) {
  supla_esp_mqtt_vars->status = CONN_STATUS_DISCONNECTED;
  supla_log(LOG_DEBUG, "on_disconnect");
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

  supla_esp_mqtt_vars->status = CONN_STATUS_CONNECTING;

  if (supla_esp_mqtt_espconn_connect() != 0) {
    supla_esp_mqtt_vars->status = CONN_STATUS_DISCONNECTED;
    return;
  }
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_wants_publish(uint8 idx) {
  if (idx == 0) {
    uint8 n = sizeof(supla_esp_mqtt_vars->publish_idx);

    for (uint8 a = 0; a < n; a++) {
      supla_esp_mqtt_vars->publish_idx[a] = 0xFF;
    }
  } else {
    supla_esp_mqtt_vars->publish_idx[(idx - 1) / 8] |= 1 << ((idx - 1) % 8);
  }
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_wants_subscribe(void) {
  supla_esp_mqtt_vars->subscribe_idx = 1;
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_dns__found(ip_addr_t *ip) {
  if (ip == NULL) {
    supla_esp_set_state(LOG_NOTICE, "Domain not found.");
    return;
  }

  os_memcpy(&supla_esp_mqtt_vars->ip, ip, 4);
  os_timer_disarm(&supla_esp_mqtt_vars->iterate_timer);

  mqtt_init_reconnect(&supla_esp_mqtt_vars->client, supla_esp_mqtt_reconnect,
                      NULL, supla_esp_mqtt_on_message_received);

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

#endif /*MQTT_SUPPORT_ENABLED*/
