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

#include "supla_esp.h"

#ifndef SUPLA_ESP_COUNTDOWN_TIMER_H_
#define SUPLA_ESP_COUNTDOWN_TIMER_H_

#ifndef COUNTDOWN_TIMER_DISABLED

typedef void (*_countdown_timer_finish_cb)(
    uint8 gpio_id, uint8 channel_number,
    char target_value[SUPLA_CHANNELVALUE_SIZE]);
typedef void (*_countdown_timer_on_disarm)(uint8 channel_number);

void CDT_ICACHE_FLASH_ATTR
supla_esp_countdown_set_finish_cb(_countdown_timer_finish_cb finish_cb);
void CDT_ICACHE_FLASH_ATTR
supla_esp_countdown_set_on_disarm_cb(_countdown_timer_on_disarm on_disarm_cb);

void CDT_ICACHE_FLASH_ATTR supla_esp_countdown_timer_init(void);

uint8 CDT_ICACHE_FLASH_ATTR supla_esp_countdown_timer_countdown(
    unsigned int time_ms, uint8 gpio_id, uint8 channel_number,
    char target_value[SUPLA_CHANNELVALUE_SIZE], _supla_int_t sender_id);

uint8 CDT_ICACHE_FLASH_ATTR
supla_esp_countdown_timer_disarm(uint8 channel_number);

void CDT_ICACHE_FLASH_ATTR supla_esp_countdown_get_state(
    uint8 channel_number, TTimerState_ExtendedValue *state);
void CDT_ICACHE_FLASH_ATTR supla_esp_countdown_get_state_ev(
    uint8 channel_number, TSuplaChannelExtendedValue *ev);

#endif /*COUNTDOWN_TIMER_ENABLED*/

#endif /*SUPLA_ESP_COUNTDOWN_TIMER_H_*/
