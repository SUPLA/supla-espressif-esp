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

#ifndef LIGHTSWITCH_AT_H_
#define LIGHTSWITCH_AT_H_

#define ESP8266_SUPLA_PROTO_VERSION 16
#define B_RELAY1_PORT	13
#define B_BTN1_PORT	12
#define RETREIVE_CHANNEL_CONFIG 0b10

#define CFGBTN_TYPE_SELECTION
#define LED_RED_PORT    14

void supla_esp_board_send_channel_values_with_delay(void *srpc);

#endif // LIGHTSWITCH_AT_H_

