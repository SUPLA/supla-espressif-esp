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

#ifndef COUNTDOWN_TIMER_DISABLED

#include "supla_esp_countdown_timer.h"
#include <osapi.h>
#include "supla-dev/log.h"

typedef struct {
  _countdown_timer_finish_cb finish_cb;
  _supla_int_t sender_id;
  unsigned _supla_int64_t last_time;
  unsigned int time_left_ms;
  uint8 gpio_id;
  uint8 channel_number;

} _t_countdown_timer_item;

typedef struct {
  unsigned int delay_ms;
  ETSTimer timer;
  _t_countdown_timer_item items[RELAY_MAX_COUNT];

#ifdef BOARD_COUNTDOWN_TIMER_STATE_SAVERESTORE
  unsigned _supla_int64_t last_save_time;
#endif /*BOARD_COUNTDOWN_TIMER_STATE_SAVERESTORE*/

} _t_countdown_timer_vars;

_t_countdown_timer_vars countdown_timer_vars;

void CDT_ICACHE_FLASH_ATTR supla_esp_countdown_timer_init(void) {
  memset(&countdown_timer_vars, 0, sizeof(_t_countdown_timer_vars));
  for (uint8 a = 0; a < RELAY_MAX_COUNT; a++) {
    countdown_timer_vars.items[a].gpio_id = 255;
  }
}

void CDT_ICACHE_FLASH_ATTR supla_esp_countdown_timer_startstop(void);

void CDT_ICACHE_FLASH_ATTR supla_esp_countdown_timer_cb(void *ptr) {
#ifdef BOARD_COUNTDOWN_TIMER_STATE_SAVERESTORE
  uint8 save = uptime_msec() > countdown_timer_vars.last_save_time +
                                   BOARD_COUNTDOWN_TIMER_SAVE_DELAY_MS;
#endif /*BOARD_COUNTDOWN_TIMER_STATE_SAVERESTORE*/

  for (uint8 a = 0; a < RELAY_MAX_COUNT; a++) {
    _t_countdown_timer_item *i = &countdown_timer_vars.items[a];
    if (i->gpio_id != 255 && i->time_left_ms > 0) {
      unsigned _supla_int64_t now_ms = uptime_msec();
      unsigned _supla_int64_t time_diff = now_ms - i->last_time;
      if (time_diff >= i->time_left_ms) {
        i->time_left_ms = 0;
        if (i->finish_cb) {
          i->finish_cb(i->gpio_id, i->channel_number);
        }
        i->gpio_id = 255;
      } else {
        i->time_left_ms -= time_diff;
      }
      i->last_time = now_ms;

#ifdef BOARD_COUNTDOWN_TIMER_STATE_SAVERESTORE
      if (save) {
        supla_esp_board_save_countdown_timer_state(
            countdown_timer_vars.time_ms, countdown_timer_vars.gpio_id,
            countdown_timer_vars.channel_number, countdown_timer_vars.sender_id,
            0);
      }
#endif /*BOARD_COUNTDOWN_TIMER_STATE_SAVERESTORE*/
    }
  }

#ifdef BOARD_COUNTDOWN_TIMER_STATE_SAVERESTORE
  if (save) {
    supla_esp_board_save_countdown_timer_state(0, 0, 0, 0, 1);
  }
#endif /*BOARD_COUNTDOWN_TIMER_STATE_SAVERESTORE*/

  supla_esp_countdown_timer_startstop();
}

void CDT_ICACHE_FLASH_ATTR supla_esp_countdown_timer_startstop(void) {
  unsigned int delay_ms = 0;

  for (uint8 a = 0; a < RELAY_MAX_COUNT; a++) {
    if (countdown_timer_vars.items[a].gpio_id != 255 &&
        countdown_timer_vars.items[a].time_left_ms > 0) {
      unsigned int dms = countdown_timer_vars.items[a].time_left_ms / 10;
      if (dms < 50) {
        dms = 50;
      } else if (dms > 1000) {
        dms = 1000;
      }

      if (delay_ms == 0 || dms < delay_ms) {
        delay_ms = dms;
      }
    }
  }

  if (delay_ms == 0 || delay_ms != countdown_timer_vars.delay_ms) {
    os_timer_disarm(&countdown_timer_vars.timer);

    countdown_timer_vars.delay_ms = delay_ms;

    if (countdown_timer_vars.delay_ms > 0) {
      os_timer_setfn(&countdown_timer_vars.timer,
                     (os_timer_func_t *)supla_esp_countdown_timer_cb, NULL);
      os_timer_arm(&countdown_timer_vars.timer, countdown_timer_vars.delay_ms,
                   1);
    }
  }
}

uint8 CDT_ICACHE_FLASH_ATTR supla_esp_countdown_timer_countdown(
    unsigned int time_ms, uint8 gpio_id, uint8 channel_number,
    _supla_int_t sender_id, _countdown_timer_finish_cb finish_cb) {
#ifdef BOARD_ON_COUNTDOWN_START
  BOARD_ON_COUNTDOWN_START;
#endif /*BOARD_ON_COUNTDOWN_START*/

  _t_countdown_timer_item *i = NULL;
  uint8 a;

  for (a = 0; a < RELAY_MAX_COUNT; a++) {
    if (countdown_timer_vars.items[a].gpio_id == gpio_id) {
      i = &countdown_timer_vars.items[a];
      break;
    }
  }

  if (i == NULL) {
    for (a = 0; a < RELAY_MAX_COUNT; a++) {
      if (countdown_timer_vars.items[a].gpio_id == 255) {
        i = &countdown_timer_vars.items[a];
        break;
      }
    }
  }

  if (i == NULL) {
    return 0;
  }

  i->finish_cb = finish_cb;
  i->sender_id = sender_id;
  i->time_left_ms = time_ms;
  i->channel_number = channel_number;
  i->last_time = uptime_msec();
  i->gpio_id = gpio_id;

  supla_esp_countdown_timer_startstop();

  return 1;
}

#endif /*COUNTDOWN_TIMER_DISABLED*/
