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
  _supla_int_t sender_id;
  unsigned _supla_int64_t last_time;
  unsigned int time_left_ms;
  uint8 gpio_id;
  uint8 channel_number;
  char target_value[SUPLA_CHANNELVALUE_SIZE];

} _t_countdown_timer_item;

typedef struct {
  unsigned int delay_ms;
  ETSTimer timer;
  _t_countdown_timer_item items[RELAY_MAX_COUNT];

#ifdef BOARD_COUNTDOWN_TIMER_SAVE_DELAY_MS
  unsigned _supla_int64_t last_save_time;
#endif /*BOARD_COUNTDOWN_TIMER_SAVE_DELAY_MS*/

  _countdown_timer_finish_cb finish_cb;
  _countdown_timer_on_disarm on_disarm_cb;

} _t_countdown_timer_vars;

_t_countdown_timer_vars countdown_timer_vars;

void CDT_ICACHE_FLASH_ATTR
supla_esp_countdown_set_finish_cb(_countdown_timer_finish_cb finish_cb) {
  countdown_timer_vars.finish_cb = finish_cb;
}

void CDT_ICACHE_FLASH_ATTR
supla_esp_countdown_set_on_disarm_cb(_countdown_timer_on_disarm on_disarm_cb) {
  countdown_timer_vars.on_disarm_cb = on_disarm_cb;
}

void CDT_ICACHE_FLASH_ATTR supla_esp_countdown_timer_init(void) {
  memset(&countdown_timer_vars, 0, sizeof(_t_countdown_timer_vars));
  for (uint8 a = 0; a < RELAY_MAX_COUNT; a++) {
    countdown_timer_vars.items[a].channel_number = 255;
  }
}

void CDT_ICACHE_FLASH_ATTR supla_esp_countdown_timer_startstop(void);

void CDT_ICACHE_FLASH_ATTR supla_esp_countdown_timer_cb(void *ptr) {
#ifdef BOARD_COUNTDOWN_TIMER_SAVE_DELAY_MS
  uint8 save = uptime_msec() > countdown_timer_vars.last_save_time +
                                   BOARD_COUNTDOWN_TIMER_SAVE_DELAY_MS;
#endif /*BOARD_COUNTDOWN_TIMER_SAVE_DELAY_MS*/

  for (uint8 a = 0; a < RELAY_MAX_COUNT; a++) {
    _t_countdown_timer_item *i = &countdown_timer_vars.items[a];
    if (i->channel_number != 255 && i->time_left_ms > 0) {
      unsigned _supla_int64_t now_ms = uptime_msec();
      unsigned _supla_int64_t time_diff = now_ms - i->last_time;
      if (time_diff >= i->time_left_ms) {
        i->time_left_ms = 0;
        if (countdown_timer_vars.finish_cb) {
          countdown_timer_vars.finish_cb(i->gpio_id, i->channel_number,
                                         i->target_value);
        }
#ifdef BOARD_COUNTDOWN_TIMER_SAVE_DELAY_MS
        save = 1;
#endif /*BOARD_COUNTDOWN_TIMER_SAVE_DELAY_M*/
      } else {
        i->time_left_ms -= time_diff;
      }
      i->last_time = now_ms;

#ifdef BOARD_COUNTDOWN_TIMER_SAVE_DELAY_MS
      if (save) {
        supla_esp_board_save_countdown_timer_state(
            i->time_left_ms, i->gpio_id, i->channel_number, i->target_value,
            i->sender_id, 0);
      }
#endif /*BOARD_COUNTDOWN_TIMER_SAVE_DELAY_MS*/

      if (i->time_left_ms == 0) {
        i->channel_number = 255;
      }
    }
  }

#ifdef BOARD_COUNTDOWN_TIMER_SAVE_DELAY_MS
  if (save) {
    supla_esp_board_save_countdown_timer_state(0, 0, 0, NULL, 0, 1);
    countdown_timer_vars.last_save_time = uptime_msec();
  }
#endif /*BOARD_COUNTDOWN_TIMER_SAVE_DELAY_MS*/

  supla_esp_countdown_timer_startstop();
}

void CDT_ICACHE_FLASH_ATTR supla_esp_countdown_timer_startstop(void) {
  unsigned int delay_ms = 0;

  for (uint8 a = 0; a < RELAY_MAX_COUNT; a++) {
    if (countdown_timer_vars.items[a].channel_number != 255 &&
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
    char target_value[SUPLA_CHANNELVALUE_SIZE], _supla_int_t sender_id) {
#ifdef BOARD_ON_COUNTDOWN_START
  BOARD_ON_COUNTDOWN_START;
#endif /*BOARD_ON_COUNTDOWN_START*/

  _t_countdown_timer_item *i = NULL;
  uint8 a;

  for (a = 0; a < RELAY_MAX_COUNT; a++) {
    if (countdown_timer_vars.items[a].channel_number == channel_number) {
      i = &countdown_timer_vars.items[a];
      break;
    }
  }

  if (i == NULL) {
    for (a = 0; a < RELAY_MAX_COUNT; a++) {
      if (countdown_timer_vars.items[a].channel_number == 255) {
        i = &countdown_timer_vars.items[a];
        break;
      }
    }
  }

  if (i == NULL) {
    return 0;
  }

  i->sender_id = sender_id;
  i->time_left_ms = time_ms;
  i->channel_number = channel_number;
  i->last_time = uptime_msec();
  i->gpio_id = gpio_id;
  memcpy(i->target_value, target_value, sizeof(SUPLA_CHANNELVALUE_SIZE));

  supla_esp_countdown_timer_startstop();

  return 1;
}

uint8 CDT_ICACHE_FLASH_ATTR
supla_esp_countdown_timer_disarm(uint8 channel_number) {
  uint8 result = 0;
  for (uint8 a = 0; a < RELAY_MAX_COUNT; a++) {
    if (countdown_timer_vars.items[a].channel_number == channel_number) {
      countdown_timer_vars.items[a].channel_number = 255;

      if (countdown_timer_vars.items[a].time_left_ms > 0) {
        countdown_timer_vars.items[a].time_left_ms = 0;
        result = 1;

        if (countdown_timer_vars.on_disarm_cb) {
          countdown_timer_vars.on_disarm_cb(channel_number);
        }
      }

      break;
    }
  }

  return result;
}

void CDT_ICACHE_FLASH_ATTR supla_esp_countdown_get_state(
    uint8 channel_number, TTimerState_ExtendedValue *state) {
  if (state == NULL) {
    return;
  }

  memset(state, 0, sizeof(TTimerState_ExtendedValue));

  for (uint8 a = 0; a < RELAY_MAX_COUNT; a++) {
    if (countdown_timer_vars.items[a].channel_number == channel_number) {
      state->RemainingTimeMs = countdown_timer_vars.items[a].time_left_ms;
      state->SenderID = countdown_timer_vars.items[a].sender_id;
      memcpy(state->TargetValue, countdown_timer_vars.items[a].target_value,
             SUPLA_CHANNELVALUE_SIZE);
    }
  }
}

void CDT_ICACHE_FLASH_ATTR supla_esp_countdown_get_state_ev(
    uint8 channel_number, TSuplaChannelExtendedValue *ev) {
  if (ev == NULL) {
    return;
  }

  memset(ev, 0, sizeof(TSuplaChannelExtendedValue));
  ev->type = EV_TYPE_TIMER_STATE_V1;
  ev->size = sizeof(TTimerState_ExtendedValue);

  supla_esp_countdown_get_state(channel_number,
                                (TTimerState_ExtendedValue *)ev->value);
}

#endif /*COUNTDOWN_TIMER_DISABLED*/
