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

#ifdef ELECTRICITY_METER_COUNT

#ifndef ELECTRICITY_METER_CHANNEL_OFFSET
#define ELECTRICITY_METER_CHANNEL_OFFSET 0
#endif /*ELECTRICITY_METER_CHANNEL_OFFSET*/

void ICACHE_FLASH_ATTR supla_esp_em_init(void);
void ICACHE_FLASH_ATTR supla_esp_em_start(void);
void ICACHE_FLASH_ATTR supla_esp_em_stop(void);
void ICACHE_FLASH_ATTR supla_esp_em_device_registered(void);
void ICACHE_FLASH_ATTR supla_esp_em_get_value(
    unsigned char channel_number, char value[SUPLA_CHANNELVALUE_SIZE]);
void ICACHE_FLASH_ATTR supla_esp_em_send_base_value_enabled(char enabled);
void ICACHE_FLASH_ATTR supla_esp_em_set_measurement_frequency(int freq);
TElectricityMeter_ExtendedValue_V2* ICACHE_FLASH_ATTR
supla_esp_em_get_last_ev_ptr(uint8 channel_number);
_supla_int_t supla_esp_board_em_get_all_possible_measured_values(void);
#endif /*ELECTRICITY_METER_COUNT*/

#endif
