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

#ifndef SONOFF_POW2_H_
#define SONOFF_POW2_H_

#define ESP8266_SUPLA_PROTO_VERSION 10
#define MANUFACTURER_ID 0
#define PRODUCT_ID 0

#define POWSENSOR2
#define LED_RED_PORT    13
#define ELECTRICITIMETER

void supla_esp_board_send_channel_values_with_delay(void *srpc);

#endif
