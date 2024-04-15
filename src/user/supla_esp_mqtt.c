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

#include "supla_esp_cfgmode.h"
#include "supla_esp_countdown_timer.h"
#include "supla_esp_electricity_meter.h"
#include "supla_esp_impulse_counter.h"
#include "supla_update.h"

#ifdef MQTT_SUPPORT_ENABLED

#include <ctype.h>
#include <espconn.h>
#include <math.h>
#include <osapi.h>

#include "mqtt.h"
#include "supla-dev/log.h"
#include "supla_esp_cfg.h"
#include "supla_esp_gpio.h"
#include "supla_esp_rs_fb.h"
#include "supla_esp_state.h"
#include "supla_esp_wifi.h"

#define BOARD_MAX_IDX 208
#define RECONNECT_RETRY_TIME_MS 5000
#define CONN_STATUS_UNKNOWN 0
#define CONN_STATUS_CONNECTING 1
#define CONN_STATUS_CONNECTED 2
#define CONN_STATUS_READY 3
#define CONN_STATUS_DISCONNECTED 4

#define MQTT_CLIENTID_MAX_SIZE 23
#define MQTT_KEEP_ALIVE_SEC 32

#define UPTIME_REFRESH_INTERVAL_MSEC 1000
#define MQTT_SEND_BUFFER_FULL_HOLD_TIME_MS 5000
#define MQTT_SENDING_STUCK_TIME_MS 1000

typedef struct {
  uint8 started;
  ETSTimer iterate_timer;
  ETSTimer watchdog_timer;
  uint8 status;
  struct espconn esp_conn;
  esp_tcp esptcp;
  ip_addr_t ipaddr;
  uint8 error_notification;
  struct mqtt_client client;
  unsigned _supla_int64_t wait_until_ms;
  uint32 connected_at_sec;
  uint32 disconnected_at_sec;
  unsigned _supla_int64_t uptime_refresh_time_ms;
  unsigned _supla_int64_t hold_publishing_until_ms;
  uint32 ip;

  unsigned short recv_len;
  uint8 recvbuf[MQTT_RECVBUF_SIZE];
  uint8 sendbuf[MQTT_SENDBUF_SIZE] __attribute__((aligned(4)));

  uint8 subscribe_idx;
  uint8 publish_idx[32];  // Publication pool
  uint8 publish_next;
  uint32 pool_overrun_counter;

  uint16 prefix_len;
  char *prefix;
  char *device_id;

  bool resolving_started;

} _supla_esp_mqtt_vars_t;

_supla_esp_mqtt_vars_t *supla_esp_mqtt_vars = NULL;

#ifdef MQTT_HA_RELAY_SUPPORT
void ICACHE_FLASH_ATTR supla_esp_mqtt_on_countdown_timer_finish(
    uint8 gpio_id, uint8 channel_number,
    char target_value[SUPLA_CHANNELVALUE_SIZE]);

char ICACHE_FLASH_ATTR supla_esp_mqtt_channel_set_value(int port, char v,
                                                        int channel_number);
#endif /*MQTT_HA_RELAY_SUPPORT*/

void ICACHE_FLASH_ATTR supla_esp_mqtt_set_status(uint8 status) {
  supla_esp_mqtt_vars->status = status;
  if (status != CONN_STATUS_READY) {
    if (!supla_esp_mqtt_vars->disconnected_at_sec) {
      supla_esp_mqtt_vars->disconnected_at_sec = uptime_sec();
    }

    if (supla_esp_mqtt_vars->connected_at_sec) {
      supla_esp_mqtt_vars->connected_at_sec = 0;
    }

  } else {
    if (supla_esp_mqtt_vars->disconnected_at_sec) {
      supla_esp_mqtt_vars->disconnected_at_sec = 0;
    }

    if (!supla_esp_mqtt_vars->connected_at_sec) {
      supla_esp_mqtt_vars->connected_at_sec = uptime_sec();
    }
  }
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_watchdog(void *ptr) {
  if (supla_esp_cfgmode_started() == 0 && supla_esp_update_started() == 0 &&
      supla_esp_mqtt_vars->disconnected_at_sec &&
      uptime_sec() - supla_esp_mqtt_vars->disconnected_at_sec >=
          WATCHDOG_TIMEOUT_SEC) {
    os_timer_disarm(&supla_esp_mqtt_vars->watchdog_timer);
    supla_log(LOG_DEBUG, "WATCHDOG TIMEOUT");
    supla_system_restart();
  }
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_init(void) {
  supla_esp_mqtt_vars = malloc(sizeof(_supla_esp_mqtt_vars_t));
  memset(supla_esp_mqtt_vars, 0, sizeof(_supla_esp_mqtt_vars_t));

  if (!supla_esp_mqtt_vars) {
    return;
  }

  size_t user_prefix_len =
      strnlen(supla_esp_cfg.MqttTopicPrefix, MQTT_PREFIX_SIZE);
  if (user_prefix_len > 0) {
    user_prefix_len += 1;
  }
  uint8 name_len = strnlen(MQTT_DEVICE_NAME, 100);
  size_t prefix_size = user_prefix_len + name_len + 22;
  supla_esp_mqtt_vars->prefix = malloc(prefix_size);

  if (supla_esp_mqtt_vars->prefix) {
    unsigned char mac[6] = {};
    wifi_get_macaddr(STATION_IF, mac);
    ets_snprintf(supla_esp_mqtt_vars->prefix, prefix_size,
                 "%s%ssupla/devices/%s-%02x%02x%02x",
                 user_prefix_len ? supla_esp_cfg.MqttTopicPrefix : "",
                 user_prefix_len ? "/" : "", MQTT_DEVICE_NAME, mac[3], mac[4],
                 mac[5]);

    for (uint8 a = 0; a < name_len; a++) {
      supla_esp_mqtt_vars->prefix[a + user_prefix_len + 14] = (char)tolower(
          (int)supla_esp_mqtt_vars->prefix[a + user_prefix_len + 14]);
    }

    supla_esp_mqtt_vars->prefix_len =
        strnlen(supla_esp_mqtt_vars->prefix, prefix_size);

    supla_esp_mqtt_vars->device_id =
        &supla_esp_mqtt_vars->prefix[14 + user_prefix_len];
  }

  supla_esp_mqtt_set_status(CONN_STATUS_DISCONNECTED);

  os_timer_disarm(&supla_esp_mqtt_vars->watchdog_timer);
  os_timer_setfn(&supla_esp_mqtt_vars->watchdog_timer,
                 (os_timer_func_t *)supla_esp_mqtt_watchdog, NULL);
  os_timer_arm(&supla_esp_mqtt_vars->watchdog_timer, 1000, 1);

#ifdef MQTT_HA_RELAY_SUPPORT
#ifndef COUNTDOWN_TIMER_DISABLED
  supla_esp_countdown_set_finish_cb(supla_esp_mqtt_on_countdown_timer_finish);
#endif /*COUNTDOWN_TIMER_DISABLED*/
#endif /*MQTT_HA_RELAY_SUPPORT*/
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_before_system_restart(void) {}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_get_message_for_publication(
    char **topic_name, void **message, size_t *message_size, uint8 index) {
  if (index == 209) {
    return supla_esp_mqtt_prepare_message(topic_name, message, message_size,
                                          "state/connected", "true");
  }

#ifndef MQTT_DEVICE_STATE_SUPPORT_DISABLED
  if (index >= 210 && index <= 216) {
    return supla_esp_mqtt_device_state_message(topic_name, message,
                                               message_size, index - 209);
  }
#endif /*MQTT_DEVICE_STATE_SUPPORT_DISABLED*/

  return 0;
}

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
      supla_log(LOG_DEBUG, "MQTT Subscribe Error %s. Retry...",
                mqtt_error_str(r));
    }
  }

  if (topic_name) {
    free(topic_name);
  }

  return 1;
}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_publish(void) {
  uint8 idx = 0;
  uint8 bit = 0;
  uint8 find_next = 1;

  if (supla_esp_mqtt_vars->hold_publishing_until_ms) {
    if (supla_esp_mqtt_vars->hold_publishing_until_ms > uptime_msec()) {
      return 0;
    }
    supla_esp_mqtt_vars->hold_publishing_until_ms = 0;
  }

  while (find_next) {
    if (supla_esp_mqtt_vars->publish_next == 255) {
      supla_esp_mqtt_vars->publish_next = 0;
      supla_esp_mqtt_vars->pool_overrun_counter++;

      if (supla_esp_cfg.MqttPoolPublicationDelay > 0 &&
          supla_esp_mqtt_vars->pool_overrun_counter > 1) {
        supla_esp_mqtt_vars->hold_publishing_until_ms =
            uptime_msec() + supla_esp_cfg.MqttPoolPublicationDelay * 1000;
        return 0;
      }
    }

    idx = 0;

    while (supla_esp_mqtt_vars->publish_next < 255) {
      bit = 1 << (supla_esp_mqtt_vars->publish_next % 8);
      if (supla_esp_mqtt_vars
              ->publish_idx[supla_esp_mqtt_vars->publish_next / 8] &
          bit) {
        supla_esp_mqtt_vars->publish_next++;
        idx = supla_esp_mqtt_vars->publish_next;
        break;
      } else {
        supla_esp_mqtt_vars->publish_next++;
      }
    }

    if (!idx) {
      return 0;
    }

    char *topic_name = NULL;
    void *message = NULL;
    size_t message_size = 0;
    bool retain = !(supla_esp_cfg.Flags & CFG_FLAG_MQTT_NO_RETAIN);

    if ((idx <= BOARD_MAX_IDX &&
         !supla_esp_board_mqtt_get_message_for_publication(
             &topic_name, &message, &message_size, idx, &retain)) ||
        (idx > BOARD_MAX_IDX &&
         !supla_esp_mqtt_get_message_for_publication(&topic_name, &message,
                                                     &message_size, idx)) ||
        topic_name == NULL || topic_name[0] == 0) {
      supla_esp_mqtt_vars->publish_idx[(idx - 1) / 8] &= ~bit;
    } else {
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

      if (retain) {
        publish_flags |= MQTT_PUBLISH_RETAIN;
      }

      // supla_log(LOG_DEBUG, "Publish %i %s", idx, topic_name);

      enum MQTTErrors r = mqtt_publish(&supla_esp_mqtt_vars->client, topic_name,
                                       message, message_size, publish_flags);

      if (r == MQTT_OK) {
        supla_esp_mqtt_vars->publish_idx[(idx - 1) / 8] &= ~bit;
      } else {
        if (r == MQTT_ERROR_SEND_BUFFER_IS_FULL) {
          supla_esp_mqtt_vars->client.error = MQTT_OK;

          // The idea of waiting a predetermined amount of time when the queue
          // is full is not entirely satisfactory. Perhaps this will require a
          // change of concept.
          supla_esp_mqtt_vars->hold_publishing_until_ms =
              uptime_msec() + MQTT_SEND_BUFFER_FULL_HOLD_TIME_MS;
        }
        supla_log(LOG_DEBUG, "MQTT Publish Error %s. MQ_len %i. Will retry...",
                  mqtt_mq_length(&supla_esp_mqtt_vars->client.mq),
                  mqtt_error_str(r));
      }
      find_next = 0;
    }

    if (topic_name) {
      free(topic_name);
      topic_name = NULL;
    }

    if (message) {
      free(message);
      message = NULL;
    }
  }

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

#ifndef MQTT_DEVICE_STATE_SUPPORT_DISABLED
  if (uptime_msec() - supla_esp_mqtt_vars->uptime_refresh_time_ms >=
      UPTIME_REFRESH_INTERVAL_MSEC) {
    supla_esp_mqtt_vars->uptime_refresh_time_ms = uptime_msec();
    supla_esp_mqtt_wants_publish(212, 214);
  }
#endif /*MQTT_DEVICE_STATE_SUPPORT_DISABLED*/

  if (supla_esp_mqtt_subscribe()) {
    return;
  }

  if (supla_esp_mqtt_publish()) {
    return;
  }
}

sint8 ICACHE_FLASH_ATTR supla_esp_mqtt_conn_sent(uint8 *psent, uint16 length) {
  if (supla_esp_cfg.Flags & CFG_FLAG_MQTT_TLS) {
    return espconn_secure_sent(&supla_esp_mqtt_vars->esp_conn, psent, length);
  } else {
    return espconn_sent(&supla_esp_mqtt_vars->esp_conn, psent, length);
  }
}

ssize_t ICACHE_FLASH_ATTR mqtt_pal_sendall(mqtt_pal_socket_handle methods,
                                           const void *buf, size_t len,
                                           int flags) {
  if (supla_esp_mqtt_vars->status != CONN_STATUS_CONNECTED &&
      supla_esp_mqtt_vars->status != CONN_STATUS_READY) {
    return MQTT_ERROR_SOCKET_ERROR;
  }

  sint8 r = supla_esp_mqtt_conn_sent((uint8 *)buf, len);
  if (r == 0) {
    return len;
  } else if (r == ESPCONN_INPROGRESS || r == ESPCONN_MAXNUM) {
    supla_esp_mqtt_vars->hold_publishing_until_ms =
        uptime_msec() + MQTT_SENDING_STUCK_TIME_MS;

    return 0;
  }

  return MQTT_ERROR_SOCKET_ERROR;
}

ssize_t ICACHE_FLASH_ATTR mqtt_pal_recvall(mqtt_pal_socket_handle methods,
                                           void *buf, size_t bufsz, int flags) {
  ssize_t result = supla_esp_mqtt_vars->recv_len;
  supla_esp_mqtt_vars->recv_len = 0;

  return result;
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

  mqtt_sync(&supla_esp_mqtt_vars->client);
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_on_message_received(
    void **adapter_instance, struct mqtt_response_publish *message) {
#ifdef SUPLA_DEBUG
  char tmp[200];
  int str_size = message->topic_name_size > 199 
    ? 199
    : message->topic_name_size;
  strncpy(tmp, message->topic_name, str_size);
  tmp[str_size] = 0;
  supla_log(LOG_DEBUG, "MQTT received topic: %s", tmp);

  str_size = message->application_message_size >= 199
    ? 199
    : message->application_message_size;
  strncpy(tmp, message->application_message, str_size);
  tmp[str_size] = 0;
  supla_log(LOG_DEBUG, "MQTT received msg: %s", tmp);
#endif

  supla_esp_board_mqtt_on_message_received(
      message->dup_flag, message->qos_level, message->retain_flag,
      message->topic_name, message->topic_name_size,
      (const char *)message->application_message,
      message->application_message_size);
}

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

  supla_esp_mqtt_set_status(CONN_STATUS_DISCONNECTED);
  memset(&supla_esp_mqtt_vars->esp_conn, 0, sizeof(struct espconn));
  supla_esp_mqtt_vars->recv_len = 0;
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_conn_on_connect(void *arg) {
  supla_esp_mqtt_set_status(CONN_STATUS_CONNECTED);

  supla_esp_mqtt_vars->subscribe_idx = 0;
  memset(supla_esp_mqtt_vars->publish_idx, 0,
         sizeof(supla_esp_mqtt_vars->publish_idx));

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

  char *will_topic = NULL;
  supla_esp_mqtt_prepare_topic(&will_topic, "state/connected");

  char *username = NULL;
  char password[300];

  if (!(supla_esp_cfg.Flags & CFG_FLAG_MQTT_NO_AUTH)) {
    username = supla_esp_cfg.Username;
    int passwordLen =
        strnlen(supla_esp_cfg.Password, SUPLA_LOCATION_PWD_MAXSIZE);
    memcpy(password, supla_esp_cfg.Password, passwordLen);
    password[passwordLen] = '\0';
    if (passwordLen >= SUPLA_LOCATION_PWD_MAXSIZE) {
      int usernameLength = strnlen(supla_esp_cfg.Username,
                                   SUPLA_EMAIL_MAXSIZE);
      if (usernameLength < SUPLA_EMAIL_MAXSIZE - 1) {
        int passwordPartLen =
            strnlen(supla_esp_cfg.Username + usernameLength + 1,
                    SUPLA_EMAIL_MAXSIZE - usernameLength - 1);
        if (passwordPartLen < SUPLA_EMAIL_MAXSIZE - usernameLength - 1 - 1) {
          memcpy(password + passwordLen,
                 supla_esp_cfg.Username + usernameLength + 1,
                 passwordPartLen + 1);
        }
      }
    }
  }

  if (MQTT_OK == mqtt_connect(&supla_esp_mqtt_vars->client, clientId,
                              will_topic, "false", 5, username, password,
                              MQTT_CONNECT_CLEAN_SESSION,
                              MQTT_KEEP_ALIVE_SEC)) {
    supla_esp_gpio_state_connected();
    supla_esp_set_state(LOG_NOTICE, "Broker connected");
    supla_esp_mqtt_set_status(CONN_STATUS_READY);
    supla_esp_mqtt_wants_subscribe();
    supla_esp_mqtt_vars->hold_publishing_until_ms = 0;
    supla_esp_mqtt_vars->pool_overrun_counter = 0;
    supla_esp_mqtt_vars->publish_next = BOARD_MAX_IDX;
    supla_esp_mqtt_wants_publish(1, 255);

#ifdef ELECTRICITY_METER_COUNT
    supla_esp_em_device_registered();
#endif

#ifdef IMPULSE_COUNTER_COUNT
    supla_esp_ic_device_registered();
#endif

#ifdef BOARD_ON_DEVICE_REGISTERED
    BOARD_ON_DEVICE_REGISTERED;
#endif
  }

  if (will_topic) {
    free(will_topic);
  }
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_conn_on_disconnect(void *arg) {
  supla_esp_mqtt_set_status(CONN_STATUS_DISCONNECTED);
  supla_esp_gpio_state_ipreceived();  // We go back to the state after
                                      // connecting to wifi, and before
                                      // connecting to the broker
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_reconnect(struct mqtt_client *client,
                                                void **state) {
  if (client->error != MQTT_ERROR_INITIAL_RECONNECT) {
    if (!supla_esp_mqtt_vars->error_notification) {
      if (client->error == MQTT_ERROR_CONNECTION_REFUSED) {
        supla_esp_set_state(LOG_NOTICE, "Connection refused");
      } else {
        supla_log(LOG_DEBUG, mqtt_error_str(client->error));
      }

      supla_esp_mqtt_vars->error_notification = 1;
    }

    if (supla_esp_mqtt_vars->wait_until_ms &&
        supla_esp_mqtt_vars->wait_until_ms > uptime_msec()) {
      return;
    }
  }

  supla_esp_mqtt_vars->error_notification = 0;
  supla_esp_mqtt_vars->wait_until_ms = uptime_msec() + RECONNECT_RETRY_TIME_MS;

  supla_esp_mqtt_espconn_diconnect();

  supla_esp_set_state(LOG_NOTICE, "Connecting to MQTT Broker");
  supla_log(LOG_INFO, "Broker address %s:%i", supla_esp_cfg.Server,
            supla_esp_cfg.Port);

  supla_esp_mqtt_vars->esp_conn.proto.tcp = &supla_esp_mqtt_vars->esptcp;
  supla_esp_mqtt_vars->esp_conn.type = ESPCONN_TCP;
  supla_esp_mqtt_vars->esp_conn.state = ESPCONN_NONE;

  supla_esp_mqtt_vars->esp_conn.proto.tcp->local_port = espconn_port();
  supla_esp_mqtt_vars->esp_conn.proto.tcp->remote_port = supla_esp_cfg.Port;
  os_memcpy(supla_esp_mqtt_vars->esp_conn.proto.tcp->remote_ip,
            &supla_esp_mqtt_vars->ip, 4);

  mqtt_reinit(&supla_esp_mqtt_vars->client, 0, supla_esp_mqtt_vars->sendbuf,
              sizeof(supla_esp_mqtt_vars->sendbuf),
              supla_esp_mqtt_vars->recvbuf,
              sizeof(supla_esp_mqtt_vars->recvbuf));

  espconn_regist_recvcb(&supla_esp_mqtt_vars->esp_conn,
                        supla_esp_mqtt_conn_recv_cb);
  espconn_regist_connectcb(&supla_esp_mqtt_vars->esp_conn,
                           supla_esp_mqtt_conn_on_connect);
  espconn_regist_disconcb(&supla_esp_mqtt_vars->esp_conn,
                          supla_esp_mqtt_conn_on_disconnect);

  supla_esp_mqtt_set_status(CONN_STATUS_CONNECTING);

  if (supla_esp_mqtt_espconn_connect() != 0) {
    supla_esp_mqtt_set_status(CONN_STATUS_DISCONNECTED);
    return;
  }
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_wants_publish(uint8 idx_from,
                                                    uint8 idx_to) {
  idx_from -= 1;
  idx_to -= 1;

  if (idx_from > idx_to) {
    return;
  }

  for (uint8 a = idx_from; a <= idx_to; a++) {
    supla_esp_mqtt_vars->publish_idx[a / 8] |= 1 << (a % 8);
  }
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_wants_subscribe(void) {
  supla_esp_mqtt_vars->subscribe_idx = 1;
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_dns__found(ip_addr_t *ip) {
  if (supla_esp_mqtt_vars == NULL) {
    return;
  }

  supla_esp_mqtt_vars->resolving_started = false;

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
  os_timer_arm(&supla_esp_mqtt_vars->iterate_timer, 50, 1);
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

  supla_esp_gpio_state_ipreceived();  // We go back to the state after
                                      // connecting to wifi, and before
                                      // connecting to the broker

  if (!supla_esp_mqtt_vars || supla_esp_mqtt_vars->resolving_started) {
    // Calling espconn_gethostbyname twice causes Fatal exception 29
    // (StoreProhibitedCause)
    supla_log(LOG_DEBUG, "Resolving already started!");
    return;
  }

  supla_esp_mqtt_vars->resolving_started = true;

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

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_server_connected(void) {
  return supla_esp_mqtt_vars &&
         supla_esp_mqtt_vars->status == CONN_STATUS_READY;
}

const char ICACHE_FLASH_ATTR *supla_esp_mqtt_topic_prefix(void) {
  return supla_esp_mqtt_vars->prefix;
}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_prepare__topic(char **topic_name_out,
                                                      const char *topic,
                                                      va_list va) {
  if (!topic_name_out || !topic) {
    return 0;
  }

  va_list vac;
  va_copy(vac, va);

  char c = 0;
  // ets_snprintf does not handle NULL when determining buffer size
  size_t size = ets_vsnprintf(&c, 1, topic, vac) + 1;
  va_end(vac);

  char *_topic = malloc(size);
  if (!_topic) {
    return 0;
  }

  char result = 0;
  ets_vsnprintf(_topic, size, topic, va);

  size = ets_snprintf(&c, 1, "%s/%s", supla_esp_mqtt_vars->prefix, _topic) + 1;

  *topic_name_out = malloc(size);

  if (*topic_name_out) {
    ets_snprintf(*topic_name_out, size, "%s/%s", supla_esp_mqtt_vars->prefix,
                 _topic);

    result = 1;
  }

  free(_topic);

  return result;
}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_prepare_topic(char **topic_name_out,
                                                     const char *topic, ...) {
  va_list va;
  va_start(va, topic);
  uint8 result = supla_esp_mqtt_prepare__topic(topic_name_out, topic, va);
  va_end(va);

  return result;
}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_prepare_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    const char *topic, const char *message, ...) {
  if (!topic_name_out || !message_out || !message_size_out || !topic ||
      !supla_esp_mqtt_vars->prefix || supla_esp_mqtt_vars->prefix[0] == 0) {
    return 0;
  }

  *topic_name_out = NULL;
  *message_out = NULL;
  *message_size_out = 0;

  uint8 result = 0;
  va_list va;
  va_start(va, message);

  if (supla_esp_mqtt_prepare__topic(topic_name_out, topic, va)) {
    if (message == NULL || message[0] == 0) {
      result = 1;
    } else {
      uint16_t size = strnlen(message, 8192);
      *message_out = malloc(size);
      if (*message_out) {
        memcpy(*message_out, message, size);
        *message_size_out = size;
        result = 1;
      }
    }
  }

  va_end(va);

  if (result == 0) {
    if (*topic_name_out) {
      free(*topic_name_out);
      *topic_name_out = NULL;
    }

    if (*message_out) {
      free(*message_out);
      *message_out = NULL;
    }
  }

  return result;
}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_prepare_channel_state_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    const char *topic, const char *message, uint8 channel_number) {
  return supla_esp_mqtt_prepare_message(
      topic_name_out, message_out, message_size_out, "channels/%i/state/%s",
      message, channel_number, topic);
}

int ICACHE_FLASH_ATTR supla_esp_mqtt_str2int(const char *str, uint16_t len,
                                             uint8 *err) {
  int result = 0;
  uint8 minus = 0;
  uint8 dot = 0;
  uint16_t a = 0;
  uint16_t _len = len;

  for (a = 0; a < len; a++) {
    char c = str[a];
    if (a == 0 && c == '-') {
      minus = 1;
      continue;
    } else if (a > 0 && !dot && c == '.') {
      dot = 1;
      _len = a;
    } else if (c < '0' || c > '9') {
      if (err) {
        *err = 1;
      }
      return 0;
    }
  }

  for (a = minus ? 1 : 0; a < _len; a++) {
    result += (str[a] - '0') * pow(10, _len - 1 - a);
  }

  if (minus) {
    result *= -1;
  }

  if (err) {
    *err = 0;
  }

  return result;
}

int ICACHE_FLASH_ATTR supla_esp_mqtt_parse_int_with_prefix(
    const char *prefix, uint16_t prefix_len, char **topic_name,
    uint16_t *topic_name_size, uint8 *err) {
  if (err) {
    *err = 1;
  }

  if (*topic_name_size < prefix_len ||
      memcmp(*topic_name, prefix, prefix_len) != 0) {
    return 0;
  }

  (*topic_name) += prefix_len;
  (*topic_name_size) -= prefix_len;

  for (size_t a = 0; a < *topic_name_size; a++) {
    if ((*topic_name)[a] == '/') {
      if (a == 0) {
        return 0;
      }

      int result = supla_esp_mqtt_str2int(*topic_name, a, err);

      if (err && *err) {
        return 0;
      }

      (*topic_name) += a + 1;
      (*topic_name_size) -= a + 1;

      if (err) {
        *err = 0;
      }

      return result;
    }
  }

  return 0;
}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_lc_equal(const char *str,
                                                const char *message,
                                                size_t message_size) {
  for (size_t a = 0; a < message_size; a++) {
    uint8 i1 = message[a];
    uint8 i2 = str[a];
    if (tolower(i1) != tolower(i2)) {
      return 0;
    }
  }

  return 1;
}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_parser_set_on(
    const void *topic_name, uint16_t topic_name_size, const char *message,
    size_t message_size, uint8 *channel_number, uint8 *on) {
  if (!topic_name || topic_name_size == 0 || !message || message_size == 0 ||
      !channel_number || !on || !supla_esp_mqtt_vars->prefix ||
      supla_esp_mqtt_vars->prefix[0] == 0 ||
      supla_esp_mqtt_vars->prefix_len + 1 >= topic_name_size) {
    return 0;
  }

  char *tn = (char *)topic_name;

  if (memcmp(tn, supla_esp_mqtt_vars->prefix,
             supla_esp_mqtt_vars->prefix_len) == 0) {
    tn += supla_esp_mqtt_vars->prefix_len + 1;
    topic_name_size -= supla_esp_mqtt_vars->prefix_len + 1;
  } else {
    return 0;
  }

  uint8 err = 1;
  *channel_number = supla_esp_mqtt_parse_int_with_prefix(
      "channels/", 9, &tn, &topic_name_size, &err);

  if (err) {
    return 0;
  }

  if (topic_name_size == 6 && memcmp(tn, "set/on", topic_name_size) == 0) {
    if ((message_size == 1 && message[0] == '1') ||
        (message_size == 3 &&
         supla_esp_mqtt_lc_equal("yes", message, message_size)) ||
        (message_size == 4 &&
         supla_esp_mqtt_lc_equal("true", message, message_size))) {
      *on = 1;
      return 1;
    } else if ((message_size == 1 && message[0] == '0') ||
               (message_size == 2 &&
                supla_esp_mqtt_lc_equal("no", message, message_size)) ||
               (message_size == 5 &&
                supla_esp_mqtt_lc_equal("false", message, message_size))) {
      *on = 0;
      return 1;
    }
  } else if (topic_name_size == 14 &&
             memcmp(tn, "execute_action", topic_name_size) == 0) {
    if (message_size == 7 &&
        supla_esp_mqtt_lc_equal("turn_on", message, message_size)) {
      *on = 1;
      return 1;
    } else if (message_size == 8 &&
               supla_esp_mqtt_lc_equal("turn_off", message, message_size)) {
      *on = 0;
      return 1;
    } else if (message_size == 6 &&
               supla_esp_mqtt_lc_equal("toggle", message, message_size)) {
      *on = 255;
      return 1;
    }
  }

  return 0;
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_prepare_val(char buffer[25],
                                                  uint8 is_unsigned,
                                                  _supla_int64_t value,
                                                  uint8 precision) {
  uint8 minus = 0;
  uint8 n = 0;
  uint8 m = 0;
  uint8 offset = 0;

  if (!is_unsigned && (_supla_int64_t)value < 0) {
    value = (_supla_int64_t)value * -1;
    minus = 1;
  }

  unsigned _supla_int64_t v = value;
  while (v != 0) {
    if (!m && precision && v % 10 == 0) {
      value /= 10;
      precision--;
    } else {
      m = 1;
      n++;
    }
    v /= 10;
  }

  if (minus) {
    buffer[0] = '-';
    offset++;
  }

  if (value == 0) {
    precision = 0;
    n++;
  } else if (precision > 0) {
    if (precision >= n) {
      buffer[offset++] = '0';
      buffer[offset++] = '.';
      for (m = 0; m < precision - n; m++) {
        buffer[offset++] = '0';
      }
    } else {
      offset++;
    }
    precision++;
  }

  buffer[n + offset] = 0;

  while (n > 0) {
    if (precision) {
      precision--;
      if (!precision) {
        buffer[n + offset - 1] = '.';
        offset--;
      }
    }
    buffer[n + offset - 1] = value % 10 + '0';
    value /= 10;
    n--;
  }
}

#ifdef ELECTRICITY_METER_COUNT

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_prepare_phase_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    const char *topic, const char *message, uint8 channel_number, uint8 phase) {
  return supla_esp_mqtt_prepare_message(
      topic_name_out, message_out, message_size_out,
      "channels/%i/state/phases/%i/%s", message, channel_number, phase + 1,
      topic);
}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_prepare_em_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    uint8 index, uint8 channel_number, _supla_int_t channel_flags) {
  if (index > 41) {
    return 0;
  }

  TElectricityMeter_ExtendedValue_V2 *em_ev =
      supla_esp_em_get_last_ev_ptr(channel_number);

  if (!em_ev ||
      ((channel_flags & SUPLA_CHANNEL_FLAG_PHASE1_UNSUPPORTED) && index >= 10 &&
       index <= 21) ||
      (((channel_flags & SUPLA_CHANNEL_FLAG_PHASE2_UNSUPPORTED) &&
        index >= 22 && index <= 33)) ||
      (((channel_flags & SUPLA_CHANNEL_FLAG_PHASE3_UNSUPPORTED) &&
        index >= 34 && index <= 45))) {
    return 0;
  }

  char value[50];
  value[0] = 0;

  short phase = (index - 6) / 12;

  // Indexes are not allowed to be changed

  switch (index) {
    case 1:
      ets_snprintf(value, sizeof(value), "%u", em_ev->measured_values);
      // This topic should be called "measured_values" but for compatibility
      // with the rest API it has been changed to "support"
      return supla_esp_mqtt_prepare_channel_state_message(
          topic_name_out, message_out, message_size_out, "support", value,
          channel_number);

    case 2:
      if (em_ev->measured_values & EM_VAR_FORWARD_ACTIVE_ENERGY) {
        supla_esp_mqtt_prepare_val(value, 1,
                                   em_ev->total_forward_active_energy[0] +
                                       em_ev->total_forward_active_energy[1] +
                                       em_ev->total_forward_active_energy[2],
                                   5);

        return supla_esp_mqtt_prepare_channel_state_message(
            topic_name_out, message_out, message_size_out,
            "total_forward_active_energy", value, channel_number);
      }
      break;

    case 3:
      if (em_ev->measured_values & EM_VAR_REVERSE_ACTIVE_ENERGY) {
        supla_esp_mqtt_prepare_val(value, 1,
                                   em_ev->total_reverse_active_energy[0] +
                                       em_ev->total_reverse_active_energy[1] +
                                       em_ev->total_reverse_active_energy[2],
                                   5);

        return supla_esp_mqtt_prepare_channel_state_message(
            topic_name_out, message_out, message_size_out,
            "total_reverse_active_energy", value, channel_number);
      }
      break;

    case 4:
      if (em_ev->measured_values & EM_VAR_FORWARD_ACTIVE_ENERGY_BALANCED) {
        supla_esp_mqtt_prepare_val(
            value, 1, em_ev->total_forward_active_energy_balanced, 5);

        return supla_esp_mqtt_prepare_channel_state_message(
            topic_name_out, message_out, message_size_out,
            "total_forward_active_energy_balanced", value, channel_number);
      }
      break;

    case 5:
      if (em_ev->measured_values & EM_VAR_REVERSE_ACTIVE_ENERGY_BALANCED) {
        supla_esp_mqtt_prepare_val(
            value, 1, em_ev->total_reverse_active_energy_balanced, 5);

        return supla_esp_mqtt_prepare_channel_state_message(
            topic_name_out, message_out, message_size_out,
            "total_reverse_active_energy_balanced", value, channel_number);
      }
      break;

    case 6:
    case 18:
    case 30:
      if (em_ev->measured_values & EM_VAR_FORWARD_ACTIVE_ENERGY) {
        supla_esp_mqtt_prepare_val(
            value, 1, em_ev->total_forward_active_energy[phase], 5);

        return supla_esp_mqtt_prepare_phase_message(
            topic_name_out, message_out, message_size_out,
            "total_forward_active_energy", value, channel_number, phase);
      }
      break;

    case 7:
    case 19:
    case 31:
      if (em_ev->measured_values & EM_VAR_REVERSE_ACTIVE_ENERGY) {
        supla_esp_mqtt_prepare_val(
            value, 1, em_ev->total_reverse_active_energy[phase], 5);

        return supla_esp_mqtt_prepare_phase_message(
            topic_name_out, message_out, message_size_out,
            "total_reverse_active_energy", value, channel_number, phase);
      }
      break;

    case 8:
    case 20:
    case 32:
      if (em_ev->measured_values & EM_VAR_FORWARD_REACTIVE_ENERGY) {
        supla_esp_mqtt_prepare_val(
            value, 1, em_ev->total_forward_reactive_energy[phase], 5);

        return supla_esp_mqtt_prepare_phase_message(
            topic_name_out, message_out, message_size_out,
            "total_forward_reactive_energy", value, channel_number, phase);
      }
      break;

    case 9:
    case 21:
    case 33:
      if (em_ev->measured_values & EM_VAR_REVERSE_REACTIVE_ENERGY) {
        supla_esp_mqtt_prepare_val(
            value, 1, em_ev->total_reverse_reactive_energy[phase], 5);

        return supla_esp_mqtt_prepare_phase_message(
            topic_name_out, message_out, message_size_out,
            "total_reverse_reactive_energy", value, channel_number, phase);
      }
      break;

    case 10:
    case 22:
    case 34:
      if (em_ev->measured_values & EM_VAR_FREQ) {
        supla_esp_mqtt_prepare_val(value, 1, em_ev->m[0].freq, 2);

        return supla_esp_mqtt_prepare_phase_message(
            topic_name_out, message_out, message_size_out, "frequency", value,
            channel_number, phase);
      }
      break;

    case 11:
    case 23:
    case 35:
      if (em_ev->measured_values & EM_VAR_VOLTAGE) {
        supla_esp_mqtt_prepare_val(value, 1, em_ev->m[0].voltage[phase], 2);

        return supla_esp_mqtt_prepare_phase_message(
            topic_name_out, message_out, message_size_out, "voltage", value,
            channel_number, phase);
      }
      break;

    case 12:
    case 24:
    case 36:
      if (em_ev->measured_values & EM_VAR_CURRENT ||
          em_ev->measured_values & EM_VAR_CURRENT_OVER_65A) {
        unsigned int current = em_ev->m[0].current[phase];

        if ((em_ev->measured_values & EM_VAR_CURRENT_OVER_65A) &&
            !(em_ev->measured_values & EM_VAR_CURRENT)) {
          current *= 10;
        }

        supla_esp_mqtt_prepare_val(value, 1, current, 3);

        return supla_esp_mqtt_prepare_phase_message(
            topic_name_out, message_out, message_size_out, "current", value,
            channel_number, phase);
      }
      break;

    case 13:
    case 25:
    case 37:
      if (em_ev->measured_values & EM_VAR_POWER_ACTIVE ||
          em_ev->measured_values & EM_VAR_POWER_ACTIVE_KW) {
        _supla_int64_t power_active = em_ev->m[0].power_active[phase];

        if ((em_ev->measured_values & EM_VAR_POWER_ACTIVE_KW) &&
            !(em_ev->measured_values & EM_VAR_POWER_ACTIVE)) {
          power_active *= 1000;
        }

        supla_esp_mqtt_prepare_val(value, 0, power_active, 5);

        return supla_esp_mqtt_prepare_phase_message(
            topic_name_out, message_out, message_size_out, "power_active",
            value, channel_number, phase);
      }
      break;

    case 14:
    case 26:
    case 38:
      if (em_ev->measured_values & EM_VAR_POWER_REACTIVE ||
          em_ev->measured_values & EM_VAR_POWER_REACTIVE_KVAR) {
        _supla_int64_t power_reactive = em_ev->m[0].power_reactive[phase];

        if ((em_ev->measured_values & EM_VAR_POWER_REACTIVE_KVAR) &&
            !(em_ev->measured_values & EM_VAR_POWER_REACTIVE)) {
          power_reactive *= 1000;
        }

        supla_esp_mqtt_prepare_val(value, 0, power_reactive, 5);

        return supla_esp_mqtt_prepare_phase_message(
            topic_name_out, message_out, message_size_out, "power_reactive",
            value, channel_number, phase);
      }
      break;

    case 15:
    case 27:
    case 39:
      if (em_ev->measured_values & EM_VAR_POWER_APPARENT ||
          em_ev->measured_values & EM_VAR_POWER_APPARENT_KVA) {
        _supla_int64_t power_apparent = em_ev->m[0].power_apparent[phase];

        if ((em_ev->measured_values & EM_VAR_POWER_APPARENT_KVA) &&
            !(em_ev->measured_values & EM_VAR_POWER_APPARENT)) {
          power_apparent *= 1000;
        }

        supla_esp_mqtt_prepare_val(value, 0, power_apparent, 5);

        return supla_esp_mqtt_prepare_phase_message(
            topic_name_out, message_out, message_size_out, "power_apparent",
            value, channel_number, phase);
      }
      break;

    case 16:
    case 28:
    case 40:
      if (em_ev->measured_values & EM_VAR_POWER_FACTOR) {
        supla_esp_mqtt_prepare_val(value, 0, em_ev->m[0].power_factor[phase],
                                   3);

        return supla_esp_mqtt_prepare_phase_message(
            topic_name_out, message_out, message_size_out, "power_factor",
            value, channel_number, phase);
      }
      break;

    case 17:
    case 29:
    case 41:
      if (em_ev->measured_values & EM_VAR_PHASE_ANGLE) {
        supla_esp_mqtt_prepare_val(value, 0, em_ev->m[0].phase_angle[phase], 1);

        return supla_esp_mqtt_prepare_phase_message(
            topic_name_out, message_out, message_size_out, "phase_angle", value,
            channel_number, phase);
      }
      break;
  }
  return 0;
}
#endif /*ELECTRICITY_METER_COUNT*/

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_prepare_ha_cfg_topic(
    const char *component, char **topic_name_out, uint8 channel_number,
    uint8 n) {
  const char topic[] =
      "homeassistant/%s/supla/%02x%02x%02x%02x%02x%02x_%i_%i/config";

  *topic_name_out = NULL;
  size_t buffer_size = 0;

  unsigned char mac[6] = {};
  wifi_get_macaddr(STATION_IF, mac);
  char c = 0;

  for (uint8 a = 0; a < 2; a++) {
    buffer_size = ets_snprintf(a ? *topic_name_out : &c, a ? buffer_size : 1,
                               topic, component, mac[0], mac[1], mac[2], mac[3],
                               mac[4], mac[5], channel_number, n) +
                  1;

    if (!a) {
      *topic_name_out = malloc(buffer_size);
      if (!*topic_name_out) {
        return 0;
      }
    }
  }

  return 1;
}

#ifdef MQTT_HA_RELAY_SUPPORT
uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_ha_relay_prepare_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    uint8 light, uint8 channel_number, const char *mfr) {
  if (!supla_esp_mqtt_prepare_ha_cfg_topic(light ? "light" : "switch",
                                           topic_name_out, channel_number, 0)) {
    return 0;
  }

  const char cfg[] =
      "{"
      "\"avty\":{"
        "\"topic\":\"%s/state/connected\","
        "\"payload_available\":\"true\","
        "\"payload_not_available\":\"false\""
      "},"
      "\"~\":\"%s/channels/%i\","
      "\"device\":{"
        "\"ids\":\"%s\","
        "\"mf\":\"%s\","
        "\"name\":\"%s\","
        "\"sw\":\"%s\""
      "},"
      "\"name\":\"#%i %s\","
      "\"uniq_id\":\"supla_%02x%02x%02x%02x%02x%02x_%i\","
      "\"qos\":0,"
      "\"ret\":false,"
      "\"opt\":false,"
      "\"stat_t\":\"~/state/on\","
      "\"cmd_t\":\"~/set/on\","
      "\"pl_on\":\"true\","
      "\"pl_off\":\"false\""
      "}";
  char c = 0;

  char device_name[SUPLA_DEVICE_NAME_MAXSIZE] = {};
  supla_esp_board_set_device_name(device_name, SUPLA_DEVICE_NAME_MAXSIZE);

  unsigned char mac[6] = {};
  wifi_get_macaddr(STATION_IF, mac);

  size_t buffer_size = 0;

  for (uint8 a = 0; a < 2; a++) {
    buffer_size =
        ets_snprintf(a ? *message_out : &c, a ? buffer_size : 1, cfg,
                     supla_esp_mqtt_vars->prefix, supla_esp_mqtt_vars->prefix,
                     channel_number, supla_esp_mqtt_vars->device_id, mfr,
                     device_name, SUPLA_ESP_SOFTVER, channel_number,
                     light ? "Light switch" : "Power switch", mac[0], mac[1],
                     mac[2], mac[3], mac[4], mac[5], channel_number) +
        1;

    if (!a) {
      *message_out = malloc(buffer_size);
      if (*message_out == NULL) {
        if (*topic_name_out) {
          free(*topic_name_out);
          *topic_name_out = NULL;
        }
        return 0;
      }
    }
  }

  *message_size_out = strnlen(*message_out, buffer_size);
  return 1;
}
#endif /*MQTT_HA_RELAY_SUPPORT*/

#ifdef MQTT_HA_EM_SUPPORT
uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_ha_em__prepare_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    uint8 channel_number, const char *mfr, const char *name, const char *unit,
    const char *stat_t, uint8 precision, int n, const char *val_tmpl,
    const char *device_class, bool total_increasing) {
  if (!supla_esp_mqtt_prepare_ha_cfg_topic("sensor", topic_name_out,
                                           channel_number, n)) {
    return 0;
  }

  const char cfg[] =
      "{"
      "\"avty\":{"
        "\"topic\":\"%s/state/connected\","
        "\"payload_available\":\"true\","
        "\"payload_not_available\":\"false\""
      "},"
      "\"~\":\"%s/channels/%i\","
      "\"device\":{"
        "\"ids\":\"%s\","
        "\"mf\":\"%s\","
        "\"name\":\"%s\","
        "\"sw\":\"%s\""
      "},"
      "\"name\":\"#%i Electricity Meter (%s)\","
      "\"uniq_id\":\"supla_%02x%02x%02x%02x%02x%02x_%i_%i\","
      "\"qos\":0,"
      "\"unit_of_meas\":\"%s\","
      "\"stat_t\":\"~/%s\","
      "\"val_tpl\":\"{{ %s | round(%i)}}\","
      "\"state_class\":\"%s\""
      "%s"
      "}";

  char c = 0;

  char device_name[SUPLA_DEVICE_NAME_MAXSIZE] = {};
  supla_esp_board_set_device_name(device_name, SUPLA_DEVICE_NAME_MAXSIZE);

  unsigned char mac[6] = {};
  wifi_get_macaddr(STATION_IF, mac);

  char *_device_class = NULL;
  int len = 0;

  if (device_class && (len = strnlen(device_class, 100) + 15)) {
    _device_class = malloc(len + 1);
    if (_device_class) {
      ets_snprintf(_device_class, len + 1, ",\"dev_cla\":\"%s\"", device_class);
    }
  }

  size_t buffer_size = 0;

  for (uint8 a = 0; a < 2; a++) {
    buffer_size =
        ets_snprintf(a ? *message_out : &c, a ? buffer_size : 1, cfg,
                     supla_esp_mqtt_vars->prefix, supla_esp_mqtt_vars->prefix,
                     channel_number, supla_esp_mqtt_vars->device_id, mfr,
                     device_name, SUPLA_ESP_SOFTVER, channel_number, name,
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                     channel_number, n, unit, stat_t,
                     val_tmpl ? val_tmpl : "value", precision,
                     total_increasing ? "total_increasing" : "measurement",
                     _device_class ? _device_class : "") +
        1;

    // ~/state/support in last_reset_topic was used only to keep the topic
    // active.

    if (!a) {
      *message_out = malloc(buffer_size);
      if (*message_out == NULL) {
        if (*topic_name_out) {
          free(*topic_name_out);
          *topic_name_out = NULL;
        }
        break;
      }
    }
  }

  if (_device_class) {
    free(_device_class);
    _device_class = NULL;
  }

  if (message_out) {
    *message_size_out = strnlen(*message_out, buffer_size);
    return 1;
  }

  return 0;
}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_ha_prepare_em_phase_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    uint8 channel_number, const char *mfr, const char *name, const char *unit,
    const char *stat_t, uint8 precision, uint8 phase, int n,
    const char *val_tmpl, const char *device_class, bool total_increasing) {
  size_t _stat_t_size = strlen(stat_t) + 20;
  char *_stat_t = malloc(_stat_t_size);
  if (!_stat_t) {
    return 0;
  }

  size_t _name_size = strlen(name) + 15;
  char *_name = malloc(_name_size);

  if (!_name) {
    free(_stat_t);
    return 0;
  }

  ets_snprintf(_stat_t, _stat_t_size, "state/phases/%i/%s", phase, stat_t);
  ets_snprintf(_name, _name_size, "%s - Phase %i", name, phase);

  uint8 result = supla_esp_mqtt_ha_em__prepare_message(
      topic_name_out, message_out, message_size_out, channel_number, mfr, _name,
      unit, _stat_t, precision, n, val_tmpl, device_class, total_increasing);

  free(_stat_t);
  free(_name);
  return result;
}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_ha_prepare_em_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    uint8 channel_number, const char *mfr, uint8 index) {
  _supla_int_t measured_values =
      supla_esp_board_em_get_all_possible_measured_values();

  uint8 phase = (index - 5) / 12 + 1;

  // Indexes are not allowed to be changed
  switch (index) {
    case 1:
      if (measured_values & EM_VAR_FORWARD_ACTIVE_ENERGY) {
        return supla_esp_mqtt_ha_em__prepare_message(
            topic_name_out, message_out, message_size_out, channel_number, mfr,
            "Total forward active energy", "kWh",
            "state/total_forward_active_energy", 5, index, NULL, "energy",
            true);
      }
      break;
    case 2:
      if (measured_values & EM_VAR_REVERSE_ACTIVE_ENERGY) {
        return supla_esp_mqtt_ha_em__prepare_message(
            topic_name_out, message_out, message_size_out, channel_number, mfr,
            "Total reverse active energy", "kWh",
            "state/total_reverse_active_energy", 5, index, NULL, "energy",
            true);
      }
      break;

    case 3:
      if (measured_values & EM_VAR_FORWARD_ACTIVE_ENERGY_BALANCED) {
        return supla_esp_mqtt_ha_em__prepare_message(
            topic_name_out, message_out, message_size_out, channel_number, mfr,
            "Total forward active energy - balanced", "kWh",
            "state/total_forward_active_energy_balanced", 5, index, NULL,
            "energy", true);
      }

      break;
    case 4:
      if (measured_values & EM_VAR_REVERSE_ACTIVE_ENERGY_BALANCED) {
        return supla_esp_mqtt_ha_em__prepare_message(
            topic_name_out, message_out, message_size_out, channel_number, mfr,
            "Total reverse active energy - balanced", "kWh",
            "state/total_reverse_active_energy_balanced", 5, index, NULL,
            "energy", true);
      }

      break;

      //
      //
      // ---

    case 5:
    case 17:
    case 29:
      if (measured_values & EM_VAR_FORWARD_ACTIVE_ENERGY) {
        return supla_esp_mqtt_ha_prepare_em_phase_message(
            topic_name_out, message_out, message_size_out, channel_number, mfr,
            "Total forward active energy", "kWh", "total_forward_active_energy",
            5, phase, index, NULL, "energy", true);
      }
      break;

    case 6:
    case 18:
    case 30:
      if (measured_values & EM_VAR_REVERSE_ACTIVE_ENERGY) {
        return supla_esp_mqtt_ha_prepare_em_phase_message(
            topic_name_out, message_out, message_size_out, channel_number, mfr,
            "Total reverse active energy", "kWh", "total_reverse_active_energy",
            5, phase, index, NULL, "energy", true);
      }
      break;

    case 7:
    case 19:
    case 31:
      if (measured_values & EM_VAR_FORWARD_REACTIVE_ENERGY) {
        return supla_esp_mqtt_ha_prepare_em_phase_message(
            topic_name_out, message_out, message_size_out, channel_number, mfr,
            "Total forward reactive energy", "kvarh",
            "total_forward_reactive_energy", 5, phase, index, NULL, NULL, true);
      }
      break;

    case 8:
    case 20:
    case 32:
      if (measured_values & EM_VAR_REVERSE_REACTIVE_ENERGY) {
        return supla_esp_mqtt_ha_prepare_em_phase_message(
            topic_name_out, message_out, message_size_out, channel_number, mfr,
            "Total reverse reactive energy", "kvarh",
            "total_reverse_reactive_energy", 5, phase, index, NULL, NULL, true);
      }
      break;

    case 9:
    case 21:
    case 33:
      if (measured_values & EM_VAR_FREQ) {
        return supla_esp_mqtt_ha_prepare_em_phase_message(
            topic_name_out, message_out, message_size_out, channel_number, mfr,
            "Frequency", "Hz", "frequency", 2, phase, index, NULL, NULL, false);
      }
      break;

    case 10:
    case 22:
    case 34:
      if (measured_values & EM_VAR_VOLTAGE) {
        return supla_esp_mqtt_ha_prepare_em_phase_message(
            topic_name_out, message_out, message_size_out, channel_number, mfr,
            "Voltage", "V", "voltage", 2, phase, index, NULL, "voltage", false);
      }
      break;

    case 11:
    case 23:
    case 35:
      if (measured_values & EM_VAR_CURRENT) {
        return supla_esp_mqtt_ha_prepare_em_phase_message(
            topic_name_out, message_out, message_size_out, channel_number, mfr,
            "Current", "A", "current", 3, phase, index, NULL, "current", false);
      }
      break;

    case 12:
    case 24:
    case 36:
      if (measured_values & EM_VAR_POWER_ACTIVE) {
        return supla_esp_mqtt_ha_prepare_em_phase_message(
            topic_name_out, message_out, message_size_out, channel_number, mfr,
            "Power active", "W", "power_active", 5, phase, index, NULL, "power",
            false);
      }
      break;

    case 13:
    case 25:
    case 37:
      if (measured_values & EM_VAR_POWER_REACTIVE) {
        return supla_esp_mqtt_ha_prepare_em_phase_message(
            topic_name_out, message_out, message_size_out, channel_number, mfr,
            "Power reactive", "var", "power_reactive", 5, phase, index, NULL,
            NULL, false);
      }
      break;

    case 14:
    case 26:
    case 38:
      if (measured_values & EM_VAR_POWER_APPARENT) {
        return supla_esp_mqtt_ha_prepare_em_phase_message(
            topic_name_out, message_out, message_size_out, channel_number, mfr,
            "Power apparent", "VA", "power_apparent", 5, phase, index, NULL,
            NULL, false);
      }
      break;

    case 15:
    case 27:
    case 39:
      if (measured_values & EM_VAR_POWER_FACTOR) {
        return supla_esp_mqtt_ha_prepare_em_phase_message(
            topic_name_out, message_out, message_size_out, channel_number, mfr,
            "Power factor", "%", "power_factor", 3, phase, index,
            "(float(value) * 100.0)", "power_factor", false);
      }
      break;

    case 16:
    case 28:
    case 40:
      if (measured_values & EM_VAR_PHASE_ANGLE) {
        return supla_esp_mqtt_ha_prepare_em_phase_message(
            topic_name_out, message_out, message_size_out, channel_number, mfr,
            "Phase angle", "°", "phase_angle", 1, phase, index, NULL, NULL,
            false);
      }
      break;
  }

  return 0;
}
#endif /*MQTT_HA_EM_SUPPORT*/

#ifdef MQTT_HA_ROLLERSHUTTER_SUPPORT
uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_ha_rs_prepare_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    uint8 channel_number, const char *mfr) {
  if (!supla_esp_mqtt_prepare_ha_cfg_topic("cover", topic_name_out,
                                           channel_number, 0)) {
    return 0;
  }

  const char cfg[] =
      "{"
      "\"~\":\"%s/channels/%i\","
      "\"device\":{"
        "\"ids\":\"%s\","
        "\"mf\":\"%s\","
        "\"name\":\"%s\","
        "\"sw\":\"%s\"},"
        "\"name\":\"#%i Roof window operation\","
        "\"uniq_id\":\"supla_%02x%02x%02x%02x%02x%02x_%i_%i\","
        "\"qos\":0,"
        "\"ret\":false,"
        "\"opt\":false,"
        "\"cmd_t\":\"~/execute_action\","
        "\"dev_cla\":\"shutter\","
        "\"pl_open\":\"REVEAL\","
        "\"pl_cls\":\"SHUT\","
        "\"pl_stop\":\"STOP\","
        "\"set_pos_t\":\"~/set/closing_percentage\","
        "\"pos_t\":\"~/state/shut\","
        "\"pos_open\":0,"
        "\"pos_clsd\":100,"
        "\"avty_t\":\"%s/state/connected\","
        "\"pl_avail\":\"true\","
        "\"pl_not_avail\":\"false\","
        "\"pos_tpl\":\""
          "{%% if value is defined %%}"
            "{%% if value | int < 0 %%}"
              "0"
            "{%% elif value | int > 100 %%}"
              "100"
            "{%% else %%}"
              "{{value | int}}"
            "{%% endif %%}"
          "{%% else %%}"
            "0"
          "{%% endif %%}\"}";

  char c = 0;

  char device_name[SUPLA_DEVICE_NAME_MAXSIZE] = {};
  supla_esp_board_set_device_name(device_name, SUPLA_DEVICE_NAME_MAXSIZE);

  unsigned char mac[6] = {};
  wifi_get_macaddr(STATION_IF, mac);

  size_t buffer_size = 0;

  for (uint8 a = 0; a < 2; a++) {
    buffer_size = ets_snprintf(a ? *message_out : &c, a ? buffer_size : 1, cfg,
                               supla_esp_mqtt_vars->prefix, channel_number,
                               supla_esp_mqtt_vars->device_id, mfr, device_name,
                               SUPLA_ESP_SOFTVER, channel_number, mac[0],
                               mac[1], mac[2], mac[3], mac[4], mac[5],
                               channel_number, 0, supla_esp_mqtt_vars->prefix) +
                  1;

    if (!a) {
      *message_out = malloc(buffer_size);
      if (*message_out == NULL) {
        if (*topic_name_out) {
          free(*topic_name_out);
          *topic_name_out = NULL;
        }
        return 0;
      }
    }
  }

  *message_size_out = strnlen(*message_out, buffer_size);
  return 1;
}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_parser_rs_action(
    const void *topic_name, uint16_t topic_name_size, const char *message,
    size_t message_size, uint8 *channel_number, uint8 *action,
    uint8 *percentage) {
  if (!topic_name || topic_name_size == 0 || !message || message_size == 0 ||
      !channel_number || !action || !supla_esp_mqtt_vars->prefix ||
      supla_esp_mqtt_vars->prefix[0] == 0 ||
      supla_esp_mqtt_vars->prefix_len + 1 >= topic_name_size) {
    return 0;
  }

  char *tn = (char *)topic_name;

  if (memcmp(tn, supla_esp_mqtt_vars->prefix,
             supla_esp_mqtt_vars->prefix_len) == 0) {
    tn += supla_esp_mqtt_vars->prefix_len + 1;
    topic_name_size -= supla_esp_mqtt_vars->prefix_len + 1;
  } else {
    return 0;
  }

  uint8 err = 1;
  *channel_number = supla_esp_mqtt_parse_int_with_prefix(
      "channels/", 9, &tn, &topic_name_size, &err);

  if (err) {
    return 0;
  }

  if (topic_name_size == 22 &&
      memcmp(tn, "set/closing_percentage", topic_name_size) == 0) {
    uint8 err = 1;
    int p = supla_esp_mqtt_str2int(message, message_size, &err);
    if (!err && p >= 0 && p <= 100) {
      *percentage = p;
      *action = MQTT_ACTION_SHUT_WITH_PERCENTAGE;
      return 1;
    }

  } else if (topic_name_size == 14 &&
             memcmp(tn, "execute_action", topic_name_size) == 0) {
    if (message_size == 4 &&
        supla_esp_mqtt_lc_equal("shut", message, message_size)) {
      *action = MQTT_ACTION_SHUT;
      return 1;
    } else if (message_size == 6 &&
               supla_esp_mqtt_lc_equal("reveal", message, message_size)) {
      *action = MQTT_ACTION_REVEAL;
      return 1;
    } else if (message_size == 4 &&
               supla_esp_mqtt_lc_equal("stop", message, message_size)) {
      *action = MQTT_ACTION_STOP;
      return 1;
    } else if (message_size == 11 &&
               supla_esp_mqtt_lc_equal("recalibrate", message, message_size)) {
      *action = MQTT_ACTION_RECALIBRATE;
      return 1;
    } else if (message_size == 9 &&
               supla_esp_mqtt_lc_equal("calibrate", message, message_size)) {
      *action = MQTT_ACTION_RECALIBRATE;
      return 1;
    }
  }

  return 0;
}

void ICACHE_FLASH_ATTR
supla_esp_mqtt_rs_recalibrate(unsigned char channelNumber) {
  for (int i = 0; i < RS_MAX_COUNT; i++) {
    if (supla_rs_cfg[i].up != NULL && supla_rs_cfg[i].down != NULL &&
        supla_rs_cfg[i].up->channel == channelNumber &&
        supla_rs_cfg[i].up->channel_flags &
            SUPLA_CHANNEL_FLAG_CALCFG_RECALIBRATE) {
      supla_rs_cfg[i].autoCal_step = 0;
      *(supla_rs_cfg[i].auto_opening_time) = 0;
      *(supla_rs_cfg[i].auto_closing_time) = 0;
      *(supla_rs_cfg[i].position) = 0;  // not calibrated

      supla_esp_gpio_rs_apply_new_times(i, supla_esp_cfg.Time2[i],
                                        supla_esp_cfg.Time1[i]);
      // trigger calibration by setting position to fully open
      supla_esp_gpio_rs_add_task(i, 0, 0);
    }
  }
}

#endif /*MQTT_HA_ROLLERSHUTTER_SUPPORT*/

#ifdef MQTT_DIMMER_SUPPORT
uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_ha_dimmer_prepare_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    uint8 channel_number, const char *mfr) {
  if (!supla_esp_mqtt_prepare_ha_cfg_topic("light", topic_name_out,
                                           channel_number, 0)) {
    return 0;
  }

  const char cfg[] =
      "{"
      "\"avty\":{"
        "\"topic\":\"%s/state/connected\","
        "\"payload_available\":\"true\","
        "\"payload_not_available\":\"false\""
      "},"
      "\"~\":\"%s/channels/%i\","
      "\"device\":{"
        "\"ids\":\"%s\","
        "\"mf\":\"%s\","
        "\"name\":\"%s\","
        "\"sw\":\"%s\""
      "},"
      "\"name\":\"#%i Light switch\","
      "\"uniq_id\":\"supla_%02x%02x%02x%02x%02x%02x_%i\","
      "\"qos\":0,"
      "\"ret\":false,"
      "\"cmd_t\":\"~/execute_action\","
      "\"pl_on\":\"TURN_ON\","
      "\"pl_off\":\"TURN_OFF\","
      "\"stat_t\":\"~/state/on\","
      "\"stat_val_tpl\":\"{"
        "%% if value == \\\"true\\\" %%}"
          "TURN_ON"
        "{%% else %%}"
          "TURN_OFF"
        "{%% endif %%}\","
      "\"on_cmd_type\":\"first\","
      "\"bri_cmd_t\":\"~/set/brightness\","
      "\"bri_scl\":100,"
      "\"bri_stat_t\":\"~/state/brightness\""
      "}";
  char c = 0;
  char device_name[SUPLA_DEVICE_NAME_MAXSIZE] = {};
  supla_esp_board_set_device_name(device_name, SUPLA_DEVICE_NAME_MAXSIZE);

  unsigned char mac[6] = {};
  wifi_get_macaddr(STATION_IF, mac);

  size_t buffer_size = 0;

  for (uint8 a = 0; a < 2; a++) {
    buffer_size =
        ets_snprintf(a ? *message_out : &c, a ? buffer_size : 1, cfg,
                     supla_esp_mqtt_vars->prefix, supla_esp_mqtt_vars->prefix,
                     channel_number, supla_esp_mqtt_vars->device_id, mfr,
                     device_name, SUPLA_ESP_SOFTVER, channel_number, mac[0],
                     mac[1], mac[2], mac[3], mac[4], mac[5], channel_number) +
        1;

    if (!a) {
      *message_out = malloc(buffer_size);
      if (*message_out == NULL) {
        if (*topic_name_out) {
          free(*topic_name_out);
          *topic_name_out = NULL;
        }
        return 0;
      }
    }
  }

  *message_size_out = strnlen(*message_out, buffer_size);
  return 1;
}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_parser_set_brightness(
    const void *topic_name, uint16_t topic_name_size, const char *message,
    size_t message_size, uint8 *channel_number, uint8 *brightness) {
  if (!topic_name || topic_name_size == 0 || !message || message_size == 0 ||
      !channel_number || !brightness || !supla_esp_mqtt_vars->prefix ||
      supla_esp_mqtt_vars->prefix[0] == 0 ||
      supla_esp_mqtt_vars->prefix_len + 1 >= topic_name_size) {
    return 0;
  }

  char *tn = (char *)topic_name;

  if (memcmp(tn, supla_esp_mqtt_vars->prefix,
             supla_esp_mqtt_vars->prefix_len) == 0) {
    tn += supla_esp_mqtt_vars->prefix_len + 1;
    topic_name_size -= supla_esp_mqtt_vars->prefix_len + 1;
  } else {
    return 0;
  }

  uint8 err = 1;
  *channel_number = supla_esp_mqtt_parse_int_with_prefix(
      "channels/", 9, &tn, &topic_name_size, &err);

  if (err) {
    return 0;
  }

  if (topic_name_size == 14 &&
      memcmp(tn, "set/brightness", topic_name_size) == 0) {
    uint8 err = 1;
    int p = supla_esp_mqtt_str2int(message, message_size, &err);
    if (!err && p >= 0 && p <= 100) {
      *brightness = p;
      return 1;
    }
  };

  return 0;
}

#endif /*MQTT_DIMMER_SUPPORT*/

#ifdef MQTT_HA_ACTION_TRIGGER_SUPPORT

const char *supla_esp_mqtt_ha_action_trigger_get_action_str(uint8 action_idx) {
  static const char long_press[] = "button_long_press";
  static const char button_short_press[] = "button_short_press";
  static const char button_double_press[] = "button_double_press";
  static const char button_triple_press[] = "button_triple_press";
  static const char button_quadruple_press[] = "button_quadruple_press";
  static const char button_quintuple_press[] = "button_quintuple_press";
  static const char button_turn_on[] = "button_turn_on";
  static const char button_turn_off[] = "button_turn_off";

  const char *selected_action = NULL;
  switch (action_idx) {
    case 0:
      selected_action = long_press;
      break;
    case 1:
      selected_action = button_short_press;
      break;
    case 2:
      selected_action = button_double_press;
      break;
    case 3:
      selected_action = button_triple_press;
      break;
    case 4:
      selected_action = button_quadruple_press;
      break;
    case 5:
      selected_action = button_quintuple_press;
      break;
    case 6:
      selected_action = button_turn_on;
      break;
    case 7:
      selected_action = button_turn_off;
      break;
  };

  return selected_action;
}

#define ACTION_CHECK(ACT_CAP) ((active_action_trigger_caps & (ACT_CAP)) != 0) 

bool ICACHE_FLASH_ATTR supla_esp_mqtt_ha_action_trigger_is_enabled(
    uint8 action_idx, _supla_int_t active_action_trigger_caps) {
  switch (action_idx) {
    case 0:
      return ACTION_CHECK(SUPLA_ACTION_CAP_HOLD);
      break;

    case 1:
      return ACTION_CHECK(SUPLA_ACTION_CAP_TOGGLE_x1) ||
        ACTION_CHECK(SUPLA_ACTION_CAP_SHORT_PRESS_x1);
      break;

    case 2:
      return ACTION_CHECK(SUPLA_ACTION_CAP_TOGGLE_x2) ||
        ACTION_CHECK(SUPLA_ACTION_CAP_SHORT_PRESS_x2);
      break;

    case 3:
      return ACTION_CHECK(SUPLA_ACTION_CAP_TOGGLE_x3) ||
        ACTION_CHECK(SUPLA_ACTION_CAP_SHORT_PRESS_x3);
      break;

    case 4:
      return ACTION_CHECK(SUPLA_ACTION_CAP_TOGGLE_x4) ||
        ACTION_CHECK(SUPLA_ACTION_CAP_SHORT_PRESS_x4);
      break;

    case 5:
      return ACTION_CHECK(SUPLA_ACTION_CAP_TOGGLE_x5) ||
        ACTION_CHECK(SUPLA_ACTION_CAP_SHORT_PRESS_x5);
      break;

    case 6:
      return ACTION_CHECK(SUPLA_ACTION_CAP_TURN_ON);
      break;

    case 7:
      return ACTION_CHECK(SUPLA_ACTION_CAP_TURN_OFF);
      break;
  };
  return false;
}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_ha_action_trigger_prepare_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    const char *mfr, uint8 channel_number, uint8 action_idx,
    uint8 button_number, _supla_int_t active_action_trigger_caps) {
  if (!supla_esp_mqtt_prepare_ha_cfg_topic("device_automation", topic_name_out,
        channel_number, action_idx)) {
    return 0;
  }

  const char *selected_action =
    supla_esp_mqtt_ha_action_trigger_get_action_str(action_idx);

  if (!selected_action) {
    return 0;
  }

  bool enabled_trigger = supla_esp_mqtt_ha_action_trigger_is_enabled(
      action_idx, active_action_trigger_caps);

  //supla_log(LOG_DEBUG, "mqtt is enabled check %d, action %d, active %d", enabled_trigger, action_idx, active_action_trigger_caps);

  const char cfg[] =
      "{"
      "\"device\":{"
        "\"ids\":\"%s\","
        "\"mf\":\"%s\","
        "\"name\":\"%s\","
        "\"sw\":\"%s\""
      "},"
      "\"automation_type\":\"trigger\","
      "\"topic\":\"%s/channels/%i/%s\","  // prefix + channel + type
      "\"type\":\"%s\","      // type (button_short_press, etc)
      "\"subtype\":\"button_%i\"," // subtype (button_1, etc.)
      "\"payload\":\"%s\"," // type
      "\"qos\":0"
      "}";
  char c = 0;

  char device_name[SUPLA_DEVICE_NAME_MAXSIZE] = {};
  supla_esp_board_set_device_name(device_name, SUPLA_DEVICE_NAME_MAXSIZE);

  unsigned char mac[6] = {};
  wifi_get_macaddr(STATION_IF, mac);

  size_t buffer_size = 0;

  for (uint8 a = 0; a < 2; a++) {
    buffer_size =
        ets_snprintf(a ? *message_out : &c, a ? buffer_size : 1, cfg,
                     supla_esp_mqtt_vars->device_id,
                     mfr,
                     device_name,
                     SUPLA_ESP_SOFTVER,
                     supla_esp_mqtt_vars->prefix, channel_number, selected_action,
                     selected_action,
                     button_number,
                     selected_action
                     ) +
        1;

    if (!a) {
      *message_out = malloc(buffer_size);
      if (*message_out == NULL) {
        if (*topic_name_out) {
          free(*topic_name_out);
          *topic_name_out = NULL;
        }
        return 0;
      }
    }
  }
  if (!enabled_trigger) {
    // if trigger is disabled, send empty payload
    *(char *)(*message_out) = '\0';
  }

  *message_size_out = strnlen(*message_out, buffer_size);
  return 1;
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_send_action_trigger(uint8 channel, 
    int action) {
  if (!(supla_esp_cfg.Flags & CFG_FLAG_MQTT_ENABLED)) {
    return;
  }

  int offset = MQTT_BOARD_ACTION_TRIGGER_IDX_OFFSET;

  int actionIdx = -1;
  switch (action) {
    case SUPLA_ACTION_CAP_HOLD:
      actionIdx = 0;
      break;
    case SUPLA_ACTION_CAP_TOGGLE_x1:
    case SUPLA_ACTION_CAP_SHORT_PRESS_x1:
      actionIdx = 1;
      break;
    case SUPLA_ACTION_CAP_TOGGLE_x2:
    case SUPLA_ACTION_CAP_SHORT_PRESS_x2:
      actionIdx = 2;
      break;
    case SUPLA_ACTION_CAP_TOGGLE_x3:
    case SUPLA_ACTION_CAP_SHORT_PRESS_x3:
      actionIdx = 3;
      break;
    case SUPLA_ACTION_CAP_TOGGLE_x4:
    case SUPLA_ACTION_CAP_SHORT_PRESS_x4:
      actionIdx = 4;
      break;
    case SUPLA_ACTION_CAP_TOGGLE_x5:
    case SUPLA_ACTION_CAP_SHORT_PRESS_x5:
      actionIdx = 5;
      break;
    case SUPLA_ACTION_CAP_TURN_ON:
      actionIdx = 6;
      break;
    case SUPLA_ACTION_CAP_TURN_OFF:
      actionIdx = 7;
      break;
  };

  if (actionIdx >= 0) {
    int idx = offset +
      ((channel - MQTT_BOARD_ACTION_TRIGGER_FIRST_CHANNEL_ID) *
       MQTT_ACTION_TRIGGER_MAX_COUNT) +
      actionIdx;
    supla_esp_mqtt_wants_publish(idx, idx);
  }
}

uint8 ICACHE_FLASH_ATTR
supla_esp_mqtt_get_action_trigger_message_for_publication(char **topic_name,
    void **message,
    size_t *message_size,
    uint8 index) {
  int offset = MQTT_BOARD_ACTION_TRIGGER_IDX_OFFSET;

  int channel = (index - offset) / MQTT_ACTION_TRIGGER_MAX_COUNT +
    MQTT_BOARD_ACTION_TRIGGER_FIRST_CHANNEL_ID;
  int action_idx = (index - offset) % MQTT_ACTION_TRIGGER_MAX_COUNT;

  const char *selected_action =
    supla_esp_mqtt_ha_action_trigger_get_action_str(action_idx);

  if (!selected_action) {
    return 0;
  }

  return supla_esp_mqtt_prepare_message(
      topic_name, message, message_size, "channels/%i/%s",
        selected_action,
        channel, selected_action);

}
#endif /*MQTT_HA_ACTION_TRIGGER_SUPPORT*/

#ifndef MQTT_DEVICE_STATE_SUPPORT_DISABLED
uint8 ICACHE_FLASH_ATTR
supla_esp_mqtt_device_state_message(char **topic_name_out, void **message_out,
                                    size_t *message_size_out, uint8 index) {
  char value[50] = {};

  switch (index) {
    case 1: {
      struct ip_info ipconfig;
      if (wifi_get_ip_info(STATION_IF, &ipconfig) && ipconfig.ip.addr != 0) {
        ets_snprintf(value, sizeof(value), "%i.%i.%i.%i",
                     (((unsigned int)ipconfig.ip.addr) & 0xff),
                     (((unsigned int)ipconfig.ip.addr) >> 8 & 0xff),
                     (((unsigned int)ipconfig.ip.addr) >> 16 & 0xff),
                     (((unsigned int)ipconfig.ip.addr) >> 24 & 0xff));

        return supla_esp_mqtt_prepare_message(
            topic_name_out, message_out, message_size_out, "state/ip", value);
      }
    }

    case 2: {
      unsigned char mac[6] = {};
      if (wifi_get_macaddr(STATION_IF, mac)) {
        ets_snprintf(value, sizeof(value), "%02X:%02X:%02X:%02X:%02X:%02X",
                     (unsigned char)mac[0], mac[1], (unsigned char)mac[2],
                     mac[3], (unsigned char)mac[4], mac[5]);
        return supla_esp_mqtt_prepare_message(
            topic_name_out, message_out, message_size_out, "state/mac", value);
      }
    }
    case 3:

      ets_snprintf(value, sizeof(value), "%i", uptime_sec());

      return supla_esp_mqtt_prepare_message(
          topic_name_out, message_out, message_size_out, "state/uptime", value);

    case 4:
      if (supla_esp_mqtt_vars->connected_at_sec) {
        ets_snprintf(value, sizeof(value), "%i",
                     uptime_sec() - supla_esp_mqtt_vars->connected_at_sec);

        return supla_esp_mqtt_prepare_message(topic_name_out, message_out,
                                              message_size_out,
                                              "state/connection_uptime", value);
      }

    case 5:
      ets_snprintf(value, sizeof(value), "%i", system_get_free_heap_size());

      return supla_esp_mqtt_prepare_message(topic_name_out, message_out,
                                            message_size_out,
                                            "state/free_heap_size", value);

    case 6:
    case 7: {
      sint8 rssi = wifi_station_get_rssi();
      if (rssi < 10) {
        if (index == 6) {
          ets_snprintf(value, sizeof(value), "%i", rssi);
          return supla_esp_mqtt_prepare_message(topic_name_out, message_out,
                                                message_size_out, "state/rssi",
                                                value);
        } else {
          if (rssi > -50) {
            rssi = 100;
          } else if (rssi <= -100) {
            rssi = 0;
          } else {
            rssi = 2 * (rssi + 100);
          }

          ets_snprintf(value, sizeof(value), "%i", rssi);

          return supla_esp_mqtt_prepare_message(
              topic_name_out, message_out, message_size_out,
              "state/wifi_signal_strength", value);
        }
      }
    }
  }

  return 0;
}
#endif /*MQTT_DEVICE_STATE_SUPPORT_DISABLED*/

#ifdef MQTT_HA_RELAY_SUPPORT
char ICACHE_FLASH_ATTR supla_esp_mqtt_channel_set_value(int port, char v,
                                                        int channel_number) {
  char _v = v == 1 ? HI_VALUE : LO_VALUE;

  supla_esp_gpio_relay_hi(port, _v);

  _v = supla_esp_gpio_relay_is_hi(port);

  supla_esp_board_mqtt_on_relay_state_changed(channel_number);

  return (v == 1 ? HI_VALUE : LO_VALUE) == _v;
}

void ICACHE_FLASH_ATTR supla_esp_mqtt_on_countdown_timer_finish(
    uint8 gpio_id, uint8 channel_number,
    char target_value[SUPLA_CHANNELVALUE_SIZE]) {
  supla_esp_mqtt_channel_set_value(
      gpio_id, target_value[0] == 0 ? LO_VALUE : HI_VALUE, channel_number);
}

#endif /*MQTT_HA_RELAY_SUPPORT*/

#ifdef MQTT_RGB_SUPPORT
uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_ha_rgb_prepare_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    uint8 channel_number, const char *mfr) {
  if (!supla_esp_mqtt_prepare_ha_cfg_topic("light", topic_name_out,
                                           channel_number, 0)) {
    return 0;
  }

  const char cfg[] =
      "{"
      "\"avty\":{"
        "\"topic\":\"%s/state/connected\","
        "\"payload_available\":\"true\","
        "\"payload_not_available\":\"false\""
      "},"
      "\"~\":\"%s/channels/%i\","
      "\"device\":{"
        "\"ids\":\"%s\","
        "\"mf\":\"%s\","
        "\"name\":\"%s\","
        "\"sw\":\"%s\""
      "},"
      "\"name\":\"RGB Lighting\","
      "\"uniq_id\":\"supla_%02x%02x%02x%02x%02x%02x_%i\","
      "\"qos\":0,"
      "\"ret\":false,"
      "\"cmd_t\":\"~/execute_action\","
      "\"pl_on\":\"TURN_ON\","
      "\"pl_off\":\"TURN_OFF\","
      "\"stat_t\":\"~/state/on\","
      "\"stat_val_tpl\":\"{"
        "%% if value == \\\"true\\\" %%}"
          "TURN_ON"
        "{%% else %%}"
          "TURN_OFF"
        "{%% endif %%}\","
      "\"on_cmd_type\":\"last\","
      "\"bri_cmd_t\":\"~/set/color_brightness\","
      "\"bri_scl\":100,"
      "\"bri_stat_t\":\"~/state/color_brightness\","
      "\"rgb_cmd_t\":\"~/set/color\","
      "\"rgb_state_topic\":\"~/state/normalized_rgb\""
      "}";
  char c = 0;
  char device_name[SUPLA_DEVICE_NAME_MAXSIZE] = {};
  supla_esp_board_set_device_name(device_name, SUPLA_DEVICE_NAME_MAXSIZE);

  unsigned char mac[6] = {};
  wifi_get_macaddr(STATION_IF, mac);

  size_t buffer_size = 0;

  for (uint8 a = 0; a < 2; a++) {
    buffer_size =
        ets_snprintf(a ? *message_out : &c, a ? buffer_size : 1, cfg,
                     supla_esp_mqtt_vars->prefix, supla_esp_mqtt_vars->prefix,
                     channel_number, supla_esp_mqtt_vars->device_id, mfr,
                     device_name, SUPLA_ESP_SOFTVER, mac[0],
                     mac[1], mac[2], mac[3], mac[4], mac[5], channel_number) +
        1;

    if (!a) {
      *message_out = malloc(buffer_size);
      if (*message_out == NULL) {
        if (*topic_name_out) {
          free(*topic_name_out);
          *topic_name_out = NULL;
        }
        return 0;
      }
    }
  }

  *message_size_out = strnlen(*message_out, buffer_size);
  return 1;
}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_parser_set_color_brightness(
    const void *topic_name, uint16_t topic_name_size, const char *message,
    size_t message_size, uint8 *channel_number, uint8 *brightness) {
  if (!topic_name || topic_name_size == 0 || !message || message_size == 0 ||
      !channel_number || !brightness || !supla_esp_mqtt_vars->prefix ||
      supla_esp_mqtt_vars->prefix[0] == 0 ||
      supla_esp_mqtt_vars->prefix_len + 1 >= topic_name_size) {
    return 0;
  }

  char *tn = (char *)topic_name;

  if (memcmp(tn, supla_esp_mqtt_vars->prefix,
             supla_esp_mqtt_vars->prefix_len) == 0) {
    tn += supla_esp_mqtt_vars->prefix_len + 1;
    topic_name_size -= supla_esp_mqtt_vars->prefix_len + 1;
  } else {
    return 0;
  }

  uint8 err = 1;
  *channel_number = supla_esp_mqtt_parse_int_with_prefix(
      "channels/", 9, &tn, &topic_name_size, &err);

  if (err) {
    return 0;
  }

  if (topic_name_size == 20 &&
      memcmp(tn, "set/color_brightness", topic_name_size) == 0) {
    uint8 err = 1;
    int p = supla_esp_mqtt_str2int(message, message_size, &err);
    if (!err && p >= 0 && p <= 100) {
      *brightness = p;
      return 1;
    }
  };

  return 0;
}

int ICACHE_FLASH_ATTR supla_esp_mqtt_str2rgb(const char *str, int len,
    uint8 *red, uint8 *green, uint8 *blue) {

  uint8 err = 0;
  uint16_t a = 0;

  *red = *green = *blue = 0;

  uint8 colors[3] = {};

  for (int i = 0; i < 3; i++) {
    if (len <= 0) {
      return 0;
    }

    for (a = 0; a < len && str[a] != ','; a++);
    colors[i] = supla_esp_mqtt_str2int(str, a, &err);
    if (err) {
      return 0;
    }

    str += a + 1;
    len -= a + 1;
  }
  
  *red = colors[0];
  *green = colors[1];
  *blue = colors[2];
  return 1;
}

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_parser_set_color(
    const void *topic_name, uint16_t topic_name_size, const char *message,
    size_t message_size, uint8 *channel_number, uint8 *red, uint8 *green,
    uint8 *blue) {
  if (!topic_name || topic_name_size == 0 || !message || message_size == 0 ||
      !channel_number || !red || !green || !blue ||
      !supla_esp_mqtt_vars->prefix || supla_esp_mqtt_vars->prefix[0] == 0 ||
      supla_esp_mqtt_vars->prefix_len + 1 >= topic_name_size) {
    return 0;
  }

  char *tn = (char *)topic_name;

  if (memcmp(tn, supla_esp_mqtt_vars->prefix,
             supla_esp_mqtt_vars->prefix_len) == 0) {
    tn += supla_esp_mqtt_vars->prefix_len + 1;
    topic_name_size -= supla_esp_mqtt_vars->prefix_len + 1;
  } else {
    return 0;
  }

  uint8 err = 1;
  *channel_number = supla_esp_mqtt_parse_int_with_prefix(
      "channels/", 9, &tn, &topic_name_size, &err);

  if (err) {
    return 0;
  }

  if (topic_name_size == 9 &&
      memcmp(tn, "set/color", topic_name_size) == 0) {

    if (supla_esp_mqtt_str2rgb(message, message_size, red, green, blue)) {
      return 1;
    }
  };

  return 0;
}

#endif /*MQTT_RGB_SUPPORT*/

#ifdef MQTT_IMPULSE_COUNTER_SUPPORT
uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_ha_impulse_counter_prepare_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    uint8 channel_number, const char *mfr, int device_class) {
  if (!supla_esp_mqtt_prepare_ha_cfg_topic("sensor", topic_name_out,
                                           channel_number, 0)) {
    return 0;
  }

  const char cfg[] =
      "{"
      "\"avty\":{"
        "\"topic\":\"%s/state/connected\","
        "\"payload_available\":\"true\","
        "\"payload_not_available\":\"false\""
      "},"
      "\"~\":\"%s/channels/%i\","
      "\"device\":{"
        "\"ids\":\"%s\","
        "\"mf\":\"%s\","
        "\"name\":\"%s\","
        "\"sw\":\"%s\""
      "},"
      "\"name\":\"#%i %s\","
      "\"uniq_id\":\"supla_%02x%02x%02x%02x%02x%02x_%i\","
      "\"qos\":0,"
      "\"stat_t\":\"~/state/value\","
      "\"state_class\":\"total_increasing\""
      "%s"
      "}";
  char c = 0;
  char device_name[SUPLA_DEVICE_NAME_MAXSIZE] = {};
  supla_esp_board_set_device_name(device_name, SUPLA_DEVICE_NAME_MAXSIZE);

  unsigned char mac[6] = {};
  wifi_get_macaddr(STATION_IF, mac);

  size_t buffer_size = 0;

  char mqtt_append[100] = {};
  char mqtt_name[100] = "Impulse counter";
  if (device_class != MQTT_DEVICE_CLASS_NONE) {

    switch (device_class) {
      case MQTT_DEVICE_CLASS_ENERGY_WH:
        strncpy(mqtt_append, ",\"dev_cla\":\"energy\",\"unit_of_meas\":\"Wh\"",
            100);
        strncpy(mqtt_name, "Total energy", 100);
        break;
      case MQTT_DEVICE_CLASS_ENERGY_KWH:
        strncpy(mqtt_append, ",\"dev_cla\":\"energy\",\"unit_of_meas\":\"kWh\"",
            100);
        strncpy(mqtt_name, "Total energy", 100);
        break;
      case MQTT_DEVICE_CLASS_GAS_FT3:
        strncpy(mqtt_append, ",\"dev_cla\":\"gas\",\"unit_of_meas\":\"ft³\"",
            100);
        strncpy(mqtt_name, "Total gas", 100);
        break;
      case MQTT_DEVICE_CLASS_GAS_M3:
        strncpy(mqtt_append, ",\"dev_cla\":\"gas\",\"unit_of_meas\":\"m³\"",
            100);
        strncpy(mqtt_name, "Total gas", 100);
        break;
      default:
        break;
    }
  }

  for (uint8 a = 0; a < 2; a++) {
    buffer_size =
        ets_snprintf(a ? *message_out : &c, a ? buffer_size : 1, cfg,
                     supla_esp_mqtt_vars->prefix, supla_esp_mqtt_vars->prefix,
                     channel_number, supla_esp_mqtt_vars->device_id, mfr,
                     device_name, SUPLA_ESP_SOFTVER, channel_number, mqtt_name,
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                     channel_number, mqtt_append) + 1;
    if (!a) {
      *message_out = malloc(buffer_size);
      if (*message_out == NULL) {
        if (*topic_name_out) {
          free(*topic_name_out);
          *topic_name_out = NULL;
        }
        return 0;
      }
    }
  }

  *message_size_out = strnlen(*message_out, buffer_size);
  return 1;
}

#endif /*MQTT_IMPULSE_COUNTER_SUPPORT*/

#endif /*MQTT_SUPPORT_ENABLED*/
