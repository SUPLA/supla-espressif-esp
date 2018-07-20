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

#ifndef SUPLA_ELECTRICITY_METER_H_
#define SUPLA_ELECTRICITY_METER_H_

#include "supla_esp.h"

#ifdef ELECTRICITY_METER

void ICACHE_FLASH_ATTR supla_esp_em_init(void);
void ICACHE_FLASH_ATTR supla_esp_em_start(void);
void ICACHE_FLASH_ATTR supla_esp_em_device_registered(void);
void ICACHE_FLASH_ATTR supla_esp_em_get_value(unsigned char channel_number, char value[SUPLA_CHANNELVALUE_SIZE]);

#endif /*ELECTRICITY_METER*/

#endif