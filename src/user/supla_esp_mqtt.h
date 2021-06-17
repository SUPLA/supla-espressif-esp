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

#ifndef SUPLA_MQTT_H_
#define SUPLA_MQTT_H_

#include "supla_esp.h"

#ifdef MQTT_SUPPORT_ENABLED

#define MQTT_ACTION_NONE 0
#define MQTT_ACTION_TURN_ON 1
#define MQTT_ACTION_TURN_OFF 2
#define MQTT_ACTION_TOGGLE 3
#define MQTT_ACTION_SHUT 4
#define MQTT_ACTION_SHUT_WITH_PERCENTAGE 5
#define MQTT_ACTION_REVEAL 6
#define MQTT_ACTION_STOP 7

void ICACHE_FLASH_ATTR supla_esp_mqtt_init(void);
void ICACHE_FLASH_ATTR supla_esp_mqtt_before_system_restart(void);
void ICACHE_FLASH_ATTR supla_esp_mqtt_client_start(void);
void ICACHE_FLASH_ATTR supla_esp_mqtt_client_stop(void);
uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_server_connected(void);
const char ICACHE_FLASH_ATTR *supla_esp_mqtt_topic_prefix(void);
uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_prepare_topic(char **topic_name_out,
                                                     const char *topic, ...);
uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_prepare_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    const char *topic, const char *message, ...);
uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_prepare_channel_state_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    const char *topic, const char *message, uint8 channel_number);

#ifdef ELECTRICITY_METER_COUNT
uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_prepare_em_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    uint8 index, uint8 channel_number, _supla_int_t channel_flags);
#endif /*ELECTRICITY_METER_COUNT*/

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_prepare__message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    const char *topic, const char *message, uint8 channel_number);

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_parser_set_on(
    const void *topic_name, uint16_t topic_name_size, const char *message,
    size_t message_size, uint8 *channel_number, uint8 *on);

void ICACHE_FLASH_ATTR supla_esp_mqtt_wants_publish(uint8 idx_from,
                                                    uint8 idx_to);
void ICACHE_FLASH_ATTR supla_esp_mqtt_wants_subscribe(void);

uint8 ICACHE_FLASH_ATTR
supla_esp_board_mqtt_get_subscription_topic(char **topic_name, uint8 index);
uint8 ICACHE_FLASH_ATTR supla_esp_board_mqtt_get_message_for_publication(
    char **topic_name, void **message, size_t *message_size, uint8 index);

void ICACHE_FLASH_ATTR supla_esp_board_mqtt_on_message_received(
    uint8_t dup_flag, uint8_t qos_level, uint8_t retain_flag,
    const void *topic_name, uint16_t topic_name_size, const char *message,
    size_t message_size);

#ifdef MQTT_HA_RELAY_SUPPORT
uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_ha_relay_prepare_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    uint8 light, uint8 channel_number, const char *mfr);
#endif /*MQTT_HA_RELAY_SUPPORT*/

#ifdef MQTT_HA_EM_SUPPORT
uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_ha_prepare_em_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    uint8 channel_number, const char *mfr, uint8 index);
#endif

#ifdef MQTT_HA_ROLLERSHUTTER_SUPPORT
uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_ha_relay_prepare_message(
    char **topic_name_out, void **message_out, size_t *message_size_out,
    uint8 channel_number, const char *mfr);

uint8 ICACHE_FLASH_ATTR supla_esp_mqtt_parser_rs_action(
    const void *topic_name, uint16_t topic_name_size, const char *message,
    size_t message_size, uint8 *channel_number, uint8 *action,
    uint8 *percentage);
#endif /*MQTT_HA_ROLLERSHUTTER_SUPPORT*/

#endif /*MQTT_SUPPORT_ENABLED*/

#endif
