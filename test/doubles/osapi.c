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
#include <assert.h>
#include <user_interface.h>

os_timer_func_t *lastTimerCb = NULL;

struct _ETSTIMER_ *timer_first = NULL;

void executeTimers() {
  uint32_t currentTime = system_get_time() / 1000;
  struct _ETSTIMER_ *curPtr = timer_first;
  while (curPtr) {
    if (curPtr->timer_expire > 0 && currentTime >= curPtr->timer_expire) {
      if (curPtr->timer_period) {
        curPtr->timer_expire = currentTime + curPtr->timer_period;
      } else {
        curPtr->timer_expire = 0;
      }
      (curPtr->timer_func)(curPtr->timer_arg);
    }
    curPtr = curPtr->timer_next;
  }

}

void cleanupTimers() {
  timer_first = NULL;
}

void ets_timer_arm_new(os_timer_t *ptimer, uint32_t time, bool repeat_flag,
                       bool ms_flag) {
  assert(ms_flag == 1);
  assert(ptimer);
  assert(time > 0);
  assert(ptimer->timer_func);
  if (repeat_flag) {
    ptimer->timer_period = time;
  } else {
    ptimer->timer_period = 0;
  }

  ptimer->timer_expire = time + system_get_time() / 1000;

  struct _ETSTIMER_ *curPtr = timer_first;
  struct _ETSTIMER_ *lastPtr = curPtr;
  while (curPtr) {
    if (curPtr == ptimer) {
      // timer is already on a list
      return;
    }
    lastPtr = curPtr;
    curPtr = curPtr->timer_next;
  }
  // timer is not yet added to the list
  ptimer->timer_next = NULL;
  if (timer_first == NULL) {
    timer_first = ptimer;
    return;
  } 
  lastPtr->timer_next = ptimer;
}

void os_timer_disarm(os_timer_t *ptimer) {
  assert(ptimer);
  ptimer->timer_period = 0;
  ptimer->timer_expire = 0;
};

void os_timer_setfn(os_timer_t *ptimer, os_timer_func_t *pfunction,
                    void *parg) {
  assert(ptimer);
  assert(pfunction);
  ptimer->timer_func = pfunction;
  ptimer->timer_arg = parg;

  lastTimerCb = pfunction;
};

void os_delay_us(uint32_t us){};
int os_get_random(unsigned char *buf, size_t len) { return 0; }

int ets_snprintf(char *str, unsigned int size, const char *format, ...) {};
