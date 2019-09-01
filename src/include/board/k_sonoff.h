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

#ifndef K_SONOFF_H_
#define K_SONOFF_H_

#define ESP8266_SUPLA_PROTO_VERSION 7

#define SUPLA_ESP_SOFTVER "2.7.4.1"

#define RASING_EDGE

//#define LED_INVERT

#ifdef __BOARD_k_sonoff_ds18b20
	#define DS18B20
	#define TEMPERATURE_CHANNEL 1
	#define AP_SSID "SONOFF_DS18B20"

	// Sonoff TH (v2)
    	#define W1_GPIO14

#endif

#ifdef __BOARD_k_sonoff_DHT22
	#define DHTSENSOR
   	#define TEMPERATURE_HUMIDITY_CHANNEL 1
	#define AP_SSID "SONOFF_DHT22"

    // Sonoff TH (v1)
	//#define W1_GPIO3

    // Sonoff TH (v2)
       #define W1_GPIO14

#endif

#ifdef __BOARD_k_sonoff
	#define AP_SSID "SONOFF"
#endif

#define LED_RED_PORT     13
#define LED_GREEN_PORT	  2
#define B_RELAY1_PORT    12
#define B_CFG_PORT        0

#define BOARD_ESP_STARTED {GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_RED_PORT),  1);};

#define BOARD_GPIO_OUTPUT_SET_HI if (supla_last_state == STATE_CONNECTED) {if (port == LED_RED_PORT) {hi =!supla_esp_gpio_output_is_hi(B_RELAY1_PORT);\
 } else if (port==B_RELAY1_PORT) {\
 supla_esp_gpio_set_led(hi, 1, 1); }\
 else if (port == 20) { \
 	supla_log(LOG_DEBUG, "update, port = %i", port);\
	supla_esp_cfg.FirmwareUpdate = 1;\
	supla_esp_cfg_save(&supla_esp_cfg);\
	supla_esp_devconn_system_restart(); };\
}; 

void supla_esp_board_send_channel_values_with_delay(void *srpc);

#endif
