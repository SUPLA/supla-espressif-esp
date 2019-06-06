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

#ifndef SONOFF_TOUCH_H_
#define SONOFF_TOUCH_H_

#define ESP8285
#define BOARD_ON_CONNECT
#define BOARD_CFG_HTML_TEMPLATE

#define USE_GPIO9

//#define GPIO_INIT_DELAY

#define USE_GPIO14

#define LED_INVERT

#define _RASING_EDGE

#define ESP8266_SUPLA_PROTO_VERSION 7

#define SUPLA_ESP_SOFTVER "2.7.9.0"

#ifdef __BOARD_sonoff_touch
	#define AP_SSID "SONOFF-TOUCH"
#endif

#ifdef __BOARD_sonoff_touch_dual
	#define AP_SSID "TOUCH-DUAL"
#endif

#ifdef __BOARD_sonoff_touch_triple
	#define AP_SSID "TOUCH-TRIPLE"
#endif

#define LED_RED_PORT    13

#define BOARD_GPIO_OUTPUT_SET_HI if (supla_last_state == STATE_CONNECTED) {if (port == 20) { \
 	supla_log(LOG_DEBUG, "update, port = %i", port); \
	supla_esp_devconn_stop(); \
	os_delay_us(200); \
	system_restart(); };  }; 


char *ICACHE_FLASH_ATTR supla_esp_board_cfg_html_template(
    char dev_name[25], const char mac[6], const char data_saved);
void ICACHE_FLASH_ATTR supla_esp_board_on_connect(void);
void ICACHE_FLASH_ATTR
supla_esp_board_send_channel_values_with_delay(void *srpc);

#endif
