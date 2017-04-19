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

#ifndef SONOFF_H_
#define SONOFF_H_

#ifdef __BOARD_sonoff_ds18b20
	#define DS18B20
	#define TEMPERATURE_CHANNEL 1

    // Sonoff TH (v1)
	//#define W1_GPIO3

    // Sonoff TH (v2)
    #define W1_GPIO14

#endif

#define LED_RED_PORT    13

void supla_esp_board_send_channel_values_with_delay(void *srpc);

#endif
