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

#ifndef SUPLA_DS18B20_H_
#define SUPLA_DS18B20_H_

#include "supla_esp.h"
#include "supla-dev/proto.h"

#if defined DS18B20 || defined TEMPERATURE_PORT_CHANNEL

#define NUMBER_OF_ERRORS    4
#define MAX_ERROR_SIZE      10
#define MIN_MEASURING_RANGE_DS18B20     -55
#define MAX_MEASURING_RANGE_DS18B20     125

extern int supla_ds18b20_pin;

extern ETSTimer supla_ds18b20_timer1;
extern ETSTimer supla_ds18b20_timer2;

void ICACHE_FLASH_ATTR supla_ds18b20_init(void);
void supla_get_temperature(char value[SUPLA_CHANNELVALUE_SIZE]);
void supla_ds18b20_start(void);
#endif

#endif
