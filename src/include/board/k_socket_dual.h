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

#ifndef K_SOCKET_DUAL_H_
#define K_SOCKET_DUAL_H_

#define ESP8266_SUPLA_PROTO_VERSION 7

#define SUPLA_ESP_SOFTVER "2.7.16.0"

#define BOARD_CFG_HTML_TEMPLATE

#define BOARD_ON_CONNECT

#ifdef __BOARD_k_socket_dual_ds18b20
	#define DS18B20
	#define TEMPERATURE_CHANNEL 2
	#define AP_SSID "SOCKET_DUAL_DS18B20"

    	#define W1_GPIO14

#endif

#ifdef __BOARD_k_socket_dual_DHT22
	#define DHTSENSOR
   	#define TEMPERATURE_HUMIDITY_CHANNEL 2
	#define AP_SSID "SOCKET_DUAL_DHT22"

        #define W1_GPIO14

#endif

#ifdef __BOARD_k_socket_dual
	#define AP_SSID "SOCKET_DUAL"
#endif

#define LED_RED_PORT      2
#define B_RELAY1_PORT     4
#define B_RELAY2_PORT    12
#define B_CFG_PORT        0

#define BOARD_GPIO_OUTPUT_SET_HI if (supla_last_state == STATE_CONNECTED) {if (port == 20) { \
 	supla_log(LOG_DEBUG, "update, port = %i", port); \
	supla_esp_cfg.FirmwareUpdate = 1;\
	supla_esp_cfg_save(&supla_esp_cfg);\
	os_delay_us(200); \
	supla_system_restart(); };\
};
	
char *ICACHE_FLASH_ATTR supla_esp_board_cfg_html_template(
    char dev_name[25], const char mac[6], const char data_saved);
	
void ICACHE_FLASH_ATTR supla_esp_board_on_connect(void);

void supla_esp_board_send_channel_values_with_delay(void *srpc);

#endif
