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

#ifndef SUPLA_ESP_H_
#define SUPLA_ESP_H_

#include "board/supla_esp_board.h"
#include "espmissingincludes.h"

#define SUPLA_ESP_SOFTVER "2.8.5"

#define STATE_UNKNOWN 0
#define STATE_DISCONNECTED 1
#define STATE_IPRECEIVED 2
#define STATE_CONNECTED 4
#define STATE_CFGMODE 5
#define STATE_UPDATE 6

#ifndef ESP8266_SUPLA_PROTO_VERSION
#define ESP8266_SUPLA_PROTO_VERSION SUPLA_PROTO_VERSION
#endif /* ESP8266_SUPLA_PROTO_VERSION */

#ifndef STATE_SECTOR_OFFSET
#define STATE_SECTOR_OFFSET 1
#endif /*STATE_SECTOR_OFFSET*/

#define LO_VALUE 0
#define HI_VALUE 1

#define RELAY_INIT_VALUE LO_VALUE

#ifndef SAVE_STATE_DELAY
#define SAVE_STATE_DELAY 1000
#endif /*SAVE_STATE_DELAY*/

#ifndef SEND_BUFFER_SIZE
#define SEND_BUFFER_SIZE 500
#endif /*SEND_BUFFER_SIZE*/

#ifndef CFG_BTN_PRESS_TIME
#define CFG_BTN_PRESS_TIME 5000
#endif /*CFG_BTN_PRESS_TIME*/

#ifndef GET_CFG_PRESS_TIME
#define GET_CFG_PRESS_TIME supla_esp_gpio_get_cfg_press_time
#endif /*GET_CFG_PRESS_TIME*/

#ifndef INPUT_MAX_COUNT
#define INPUT_MAX_COUNT 7
#endif /*INPUT_MAX_COUNT*/

#ifndef RELAY_MAX_COUNT
#define RELAY_MAX_COUNT 4
#endif /*RELAY_MAX_COUNT*/

#ifndef RS_MAX_COUNT
#define RS_MAX_COUNT 8
#endif /*RS_MAX_COUNT*/

#ifndef RS_SAVE_STATE_DELAY
#define RS_SAVE_STATE_DELAY 0
#endif /*RS_SAVE_STATE_DELAY*/

#ifndef CFG_TIME1_COUNT
#define CFG_TIME1_COUNT 8
#endif /*CFG_TIME1_COUNT*/

#ifndef CFG_TIME2_COUNT
#define CFG_TIME2_COUNT 8
#endif /*CFG_TIME2_COUNT*/

#ifndef SMOOTH_MAX_COUNT
#define SMOOTH_MAX_COUNT 1
#endif /*SMOOTH_MAX_COUNT*/

#define INPUT_FLAG_PULLUP 0x01
#define INPUT_FLAG_CFG_BTN 0x02
#define INPUT_FLAG_FACTORY_RESET 0x04
#define INPUT_FLAG_DISABLE_INTR 0x08

#define INPUT_TYPE_SENSOR 1
#define INPUT_TYPE_BTN_MONOSTABLE 2
#define INPUT_TYPE_BTN_MONOSTABLE_RS 3
#define INPUT_TYPE_BTN_BISTABLE 4
#define INPUT_TYPE_BTN_BISTABLE_RS 5
#define INPUT_TYPE_CUSTOM 200

#ifndef INPUT_MIN_CYCLE_COUNT
#define INPUT_MIN_CYCLE_COUNT 5
#endif /*INPUT_MIN_CYCLE_COUNT*/

#ifndef INPUT_CYCLE_TIME
#define INPUT_CYCLE_TIME 20
#endif /*INPUT_CYCLE_TIME*/

// milliseconds
#ifndef RS_START_DELAY
#define RS_START_DELAY 1000
#endif

// milliseconds
#ifndef RS_STOP_DELAY
#define RS_STOP_DELAY 500
#endif

// microseconds
#ifndef RELAY_MIN_DELAY
#define RELAY_MIN_DELAY 100000
#endif

#ifndef MAIN_ICACHE_FLASH
#define MAIN_ICACHE_FLASH ICACHE_FLASH_ATTR
#endif

#ifndef GPIO_ICACHE_FLASH
#define GPIO_ICACHE_FLASH ICACHE_FLASH_ATTR
#endif

#ifndef DEVCONN_ICACHE_FLASH
#define DEVCONN_ICACHE_FLASH ICACHE_FLASH_ATTR
#endif

#ifndef CFG_ICACHE_FLASH_ATTR
#define CFG_ICACHE_FLASH_ATTR ICACHE_FLASH_ATTR
#endif

#ifndef DHT_ICACHE_FLASH
#define DHT_ICACHE_FLASH ICACHE_FLASH_ATTR
#endif

#ifndef CDT_ICACHE_FLASH_ATTR
#define CDT_ICACHE_FLASH_ATTR ICACHE_FLASH_ATTR
#endif

#ifndef DNS_ICACHE_FLASH_ATTR
#define DNS_ICACHE_FLASH_ATTR ICACHE_FLASH_ATTR
#endif

#ifndef BTN1_DEFAULT
#define BTN1_DEFAULT BTN_TYPE_MONOSTABLE
#endif

#ifndef BTN2_DEFAULT
#define BTN2_DEFAULT BTN_TYPE_BISTABLE
#endif

#ifndef MANUFACTURER_ID
#define MANUFACTURER_ID 0
#endif

#ifndef PRODUCT_ID
#define PRODUCT_ID 0
#endif

#ifndef DEVICE_FLAGS
#define DEVICE_FLAGS 0
#endif

#ifndef MQTT_PREFIX_SIZE
#define MQTT_PREFIX_SIZE 50
#endif /*MQTT_PREFIX_SIZE*/

#ifndef MQTT_RECVBUF_SIZE
#define MQTT_RECVBUF_SIZE 1024
#endif /*MQTT_RECVBUF_SIZE*/

#ifndef MQTT_SENDBUF_SIZE
#define MQTT_SENDBUF_SIZE 4096
#endif /*MQTT_SENDBUF_SIZE*/

#ifndef MQTT_DEVICE_NAME
#define MQTT_DEVICE_NAME AP_SSID
#endif /*MQTT_DEVICE_NAME*/

#define CFG_FLAG_MQTT_ENABLED 0x01
#define CFG_FLAG_MQTT_NO_RETAIN 0x02
#define CFG_FLAG_MQTT_TLS 0x04
#define CFG_FLAG_MQTT_NO_AUTH 0x08

void supla_esp_board_set_device_name(char *buffer, uint8 buffer_size);
#if ESP8266_SUPLA_PROTO_VERSION >= 10
void supla_esp_board_set_channels(TDS_SuplaDeviceChannel_C *channels,
                                  unsigned char *channel_count);
#else
void supla_esp_board_set_channels(TDS_SuplaDeviceChannel_B *channels,
                                  unsigned char *channel_count);
#endif /*ESP8266_SUPLA_PROTO_VERSION >= 10*/
void supla_esp_board_relay_before_change_state(void);
void supla_esp_board_relay_after_change_state(void);
void supla_esp_board_gpio_init(void);
void ICACHE_FLASH_ATTR supla_system_restart(void);
void ICACHE_FLASH_ATTR supla_system_restart_with_delay(uint32 delay_ms);

#ifdef __FOTA

#ifndef UPDATE_TIMEOUT
// 120 sec.
#define UPDATE_TIMEOUT 120000000
#endif /* UPDATE_TIMEOUT */

#define RSA_NUM_BYTES 512
#define RSA_PUBLIC_EXPONENT 65537

extern const uint8_t rsa_public_key_bytes[RSA_NUM_BYTES];

#ifndef UPDATE_PARAM3
#define UPDATE_PARAM3 0
#endif /*UPDATE_PARAM3*/

#ifndef UPDATE_PARAM4
#define UPDATE_PARAM4 0
#endif /*UPDATE_PARAM4*/

#endif /*__FOTA*/

#ifndef INTR_CLEAR_MASK
#define INTR_CLEAR_MASK 0xFF
#endif

#ifndef GPIO_PORT_INIT
#define GPIO_PORT_INIT                                 \
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0); \
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4); \
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5); \
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12); \
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13); \
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14)
#endif

// PWM ----------------------------------

#ifndef PWM_0_OUT_IO_MUX
#define PWM_0_OUT_IO_MUX PERIPHS_IO_MUX_GPIO4_U
#define PWM_0_OUT_IO_NUM 4
#define PWM_0_OUT_IO_FUNC FUNC_GPIO4
#endif

#ifndef PWM_1_OUT_IO_MUX
#define PWM_1_OUT_IO_MUX PERIPHS_IO_MUX_GPIO5_U
#define PWM_1_OUT_IO_NUM 5
#define PWM_1_OUT_IO_FUNC FUNC_GPIO5
#endif

#ifndef PWM_2_OUT_IO_MUX
#define PWM_2_OUT_IO_MUX PERIPHS_IO_MUX_MTDI_U
#define PWM_2_OUT_IO_NUM 12
#define PWM_2_OUT_IO_FUNC FUNC_GPIO12
#endif

#ifndef PWM_3_OUT_IO_MUX
#define PWM_3_OUT_IO_MUX PERIPHS_IO_MUX_MTCK_U
#define PWM_3_OUT_IO_NUM 13
#define PWM_3_OUT_IO_FUNC FUNC_GPIO13
#endif

#ifndef PWM_4_OUT_IO_MUX
#define PWM_4_OUT_IO_MUX PERIPHS_IO_MUX_MTMS_U
#define PWM_4_OUT_IO_NUM 14
#define PWM_4_OUT_IO_FUNC FUNC_GPIO14
#endif

#ifndef PWM_PERIOD
#define PWM_PERIOD 1000
#endif

// --------------------------------------

#ifndef AP_SSID
#ifdef ESP8285
#define AP_SSID "SUPLA-ESP8285"
#else
#define AP_SSID "SUPLA-ESP8266"
#endif
#endif

#define SPI_FLASH_SEC_SIZE 4096
#define SERVER_MAXSIZE 100
#define WIFI_SSID_MAXSIZE 32
#define WIFI_PWD_MAXSIZE 64

#define STATE_MAXSIZE 300

#define RECVBUFF_MAXSIZE 1024

#define ACTIVITY_TIMEOUT 10

#ifdef WATCHDOG_TIMEOUT
#error "WATCHDOG_TIMEOUT is deprecated use WATCHDOG_TIMEOUT_SEC"
#endif /*WATCHDOG_TIMEOUT*/

#ifdef WATCHDOG_SOFT_TIMEOUT
#error "WATCHDOG_SOFT_TIMEOUT is deprecated use WATCHDOG_SOFT_TIMEOUT_SEC"
#endif /*WATCHDOG_SOFT_TIMEOUT*/

#ifndef WATCHDOG_TIMEOUT_SEC
#define WATCHDOG_TIMEOUT_SEC 60
#endif /*WATCHDOG_TIMEOUT*/

#ifndef WATCHDOG_SOFT_TIMEOUT_SEC
// WATCHDOG_SOFT_TIMEOUT_SEC > WATCHDOG_TIMEOUT == WATCHDOG_TIMEOUT inactive
#define WATCHDOG_SOFT_TIMEOUT_SEC 65
#endif /*WATCHDOG_SOFT_TIMEOUT_SEC*/

#ifndef RECONNECT_DELAY_MSEC
#define RECONNECT_DELAY_MSEC 2000
#endif /*RECONNECT_DELAY_MSEC*/

#ifndef RELAY_DOUBLE_TRY
#define RELAY_DOUBLE_TRY 10000
#endif

#ifndef RS_MAX_COUNT
#define RS_MAX_COUNT 4
#endif

#ifndef RGBW_CHANNEl_CMP
#define RGBW_CHANNEl_CMP
#endif

#ifndef RGBW_CHANNEL_LIMIT
#define RGBW_CHANNEL_LIMIT \
  if (ChannelNumber >= 2) return;
#endif

#ifdef DONT_SAVE_STATE
#define DEVICE_STATE_INACTIVE
#endif

unsigned _supla_int64_t MAIN_ICACHE_FLASH uptime_usec(void);
unsigned _supla_int64_t MAIN_ICACHE_FLASH uptime_msec(void);
uint32 MAIN_ICACHE_FLASH uptime_sec(void);

#endif /* SUPLA_ESP_H_ */
