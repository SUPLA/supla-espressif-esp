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

#ifndef IMPULSE_COUNTER_H_
#define IMPULSE_COUNTER_H_

#define IMPULSE_COUNTER

#define COUNTER_COUNT 3
#define IMPULSE_PORT1 13  // (NodeMCU D7)
#define IMPULSE_PORT2 12  // (NodeMCU D6)
#define IMPULSE_PORT3 5   // (NodeMCU D1)

#define LED_RED_PORT 14  // (NodeMCU D5)
#define REF_LED_PORT 4   // (NodeMCU D2)
#define B_CFG_PORT 0     // (NodeMCU flash btn)
#define IMPULSE_TRIGGER_VALUE 0
#define ESP8266_SUPLA_PROTO_VERSION 10
#define BOARD_ON_CONNECT

#define BOARD_ESP_STARTING supla_esp_board_starting();
#define BOARD_INTR_HANDLER \
  if (supla_esp_board_intr_handler(gpio_status)) return

void supla_esp_board_send_channel_values_with_delay(void *srpc);
void ICACHE_FLASH_ATTR supla_esp_board_starting(void);
uint8 ICACHE_FLASH_ATTR supla_esp_board_get_impulse_counter(
    unsigned char channel_number, TDS_ImpulseCounter_Value *icv);
uint8 supla_esp_board_intr_handler(uint32 gpio_status);
void ICACHE_FLASH_ATTR supla_esp_board_on_connect(void);

#endif
