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

#ifndef SUPLA_GATE_MODULE_H_
#define SUPLA_GATE_MODULE_H_


#if defined(__BOARD_gate_module_dht11) \
    || defined(__BOARD_gate_module_dht22)
   #define DHTSENSOR
   #define TEMPERATURE_HUMIDITY_CHANNEL 4
#else
   #define DS18B20
   #define TEMPERATURE_CHANNEL 4
#endif


#define LED_GREEN_PORT  12
#define LED_BLUE_PORT   14
#define LED_REINIT

void ICACHE_FLASH_ATTR supla_esp_board_send_channel_values_with_delay(void *srpc);

#endif
