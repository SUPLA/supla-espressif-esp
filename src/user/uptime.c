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

#include "uptime.h"

#include <string.h>
#include <osapi.h>
#include <user_interface.h>

typedef struct {
  uint32 cycles;
  uint32 last_system_time;
  ETSTimer timer;
} _t_usermain_uptime;

_t_usermain_uptime usermain_uptime;

unsigned _supla_int64_t MAIN_ICACHE_FLASH uptime_usec(void) {
  uint32 time = system_get_time();
  if (time < usermain_uptime.last_system_time) {
    usermain_uptime.cycles++;
  }
  usermain_uptime.last_system_time = time;
  return usermain_uptime.cycles * (unsigned _supla_int64_t)0xffffffff +
         (unsigned _supla_int64_t)time;
}

unsigned _supla_int64_t MAIN_ICACHE_FLASH uptime_msec(void) {
  return uptime_usec() / (unsigned _supla_int64_t)1000;
}

uint32 MAIN_ICACHE_FLASH uptime_sec(void) {
  return uptime_msec() / (unsigned _supla_int64_t)1000;
}

void MAIN_ICACHE_FLASH supla_esp_uptime_counter_watchdog_cb(void *ptr) {
  uptime_usec();
}

void MAIN_ICACHE_FLASH supla_esp_uptime_init(void) {
  memset(&usermain_uptime, 0, sizeof(_t_usermain_uptime));

  os_timer_disarm(&usermain_uptime.timer);
  os_timer_setfn(&usermain_uptime.timer,
                 (os_timer_func_t *)supla_esp_uptime_counter_watchdog_cb, NULL);
  os_timer_arm(&usermain_uptime.timer, 10000, 1);
}
