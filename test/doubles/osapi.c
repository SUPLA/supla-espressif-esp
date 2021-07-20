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

#include <os_type.h>
#include <osapi.h>
#include <stdio.h>

os_timer_func_t *lastTimerCb = NULL;

void ets_timer_arm_new(os_timer_t *ptimer, uint32_t time, bool repeat_flag,
                       bool ms_flag){};
void os_timer_disarm(os_timer_t *ptimer){};
void os_timer_setfn(os_timer_t *ptimer, os_timer_func_t *pfunction,
                    void *parg) {
  lastTimerCb = pfunction;
};
void os_delay_us(uint32_t us){};
int os_get_random(unsigned char *buf, size_t len) { return 0; }

int ets_snprintf(char *str, unsigned int size, const char *format, ...) {};
