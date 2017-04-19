/*
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

#ifndef LIGHTSWITCH_H_
#define LIGHTSWITCH_H_

#define B_RELAY2_PORT	13
#define B_BTN2_PORT	12

#define CFGBTN_TYPE_SELECTION
#define LED_RED_PORT    14

#if defined(__BOARD_lightswitch_x2_54) \
	|| defined(__BOARD_lightswitch_x2_54_DHT11) \
	|| defined(__BOARD_lightswitch_x2_54_DHT22)

	#define B_RELAY1_PORT	5
	#define B_CFG_PORT	4
#else
	#define B_RELAY1_PORT	4
	#define B_CFG_PORT	5
#endif


#if defined(__BOARD_lightswitch_x2) || defined(__BOARD_lightswitch_x2_54)

	#define DS18B20
        #define TEMPERATURE_CHANNEL 2

 #elif defined(__BOARD_lightswitch_x2_DHT11) || defined(__BOARD_lightswitch_x2_54_DHT11)

	#define DHTSENSOR
	#define SENSOR_DHT11
        #define TEMPERATURE_HUMIDITY_CHANNEL 2

 #elif defined(__BOARD_lightswitch_x2_DHT22) || defined(__BOARD_lightswitch_x2_54_DHT22)

	#define DHTSENSOR
	#define SENSOR_DHT22
        #define TEMPERATURE_HUMIDITY_CHANNEL 2

#endif

void supla_esp_board_send_channel_values_with_delay(void *srpc);

#endif // LIGHTSWITCH_H_

