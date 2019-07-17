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

#ifndef INCAN_H_
#define INCAN_H_

#define BOARD_CFG_HTML_TEMPLATE
#define ESP8266_SUPLA_PROTO_VERSION 7

#define CHECK_GPIO_FOR_UART

#define LED_RED_PORT 2
#define USE_GPIO16_INPUT

#if defined(__BOARD_inCan_DS) || defined(__BOARD_inCanRS_DS)

#define DS18B20
#define TEMPERATURE_CHANNEL 1

#elif defined(__BOARD_inCan_DHT11) || defined(__BOARD_inCanRS_DHT11)

#define DHT
#define DHTSENSOR
#define SENSOR_DHT11
#define TEMPERATURE_HUMIDITY_CHANNEL 1

#elif defined(__BOARD_inCan_DHT22) || defined(__BOARD_inCanRS_DHT22)

#define DHT
#define DHTSENSOR
#define SENSOR_DHT22
#define TEMPERATURE_HUMIDITY_CHANNEL 1

#endif

#if defined(__BOARD_inCanRS_DS) || defined(__BOARD_inCanRS_DHT11) || defined(__BOARDRS_inCanRS_DHT22)

#define _ROLLERSHUTTER_SUPPORT
#define WATCHDOG_TIMEOUT 90000000
#define RELAY_MIN_DELAY 50
#endif

char* ICACHE_FLASH_ATTR supla_esp_board_cfg_html_template(
    char dev_name[25], const char mac[6], const char data_saved);
void ICACHE_FLASH_ATTR
supla_esp_board_send_channel_values_with_delay(void* srpc);

#define BOARD_ON_INPUT_ACTIVE                        \
    supla_esp_board_gpio_on_input_active(input_cfg); \
    return;
void ICACHE_FLASH_ATTR supla_esp_board_gpio_on_input_active(void* _input_cfg);

#define BOARD_ON_INPUT_INACTIVE                        \
    supla_esp_board_gpio_on_input_inactive(input_cfg); \
    return;
void ICACHE_FLASH_ATTR supla_esp_board_gpio_on_input_inactive(void* _input_cfg);
#endif
