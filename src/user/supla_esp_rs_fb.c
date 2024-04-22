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

#include <stdlib.h>
#include <string.h>

#include <ets_sys.h>
#include <gpio.h>
#include <mem.h>
#include <os_type.h>
#include <osapi.h>
#include <user_interface.h>

#include "supla_esp.h"
#include "supla_esp_cfg.h"
#include "supla_esp_gpio.h"
#include "supla_esp_rs_fb.h"
#include "supla_esp_devconn.h"

#include "supla-dev/log.h"

#define RS_STATE_STOP 0
#define RS_STATE_DOWN 1
#define RS_STATE_UP 2

supla_roller_shutter_cfg_t supla_rs_cfg[RS_MAX_COUNT];

#ifdef _ROLLERSHUTTER_SUPPORT

static uint8 rs_time_margin = 110;

void GPIO_ICACHE_FLASH supla_esp_gpio_rs_set_time_margin(int value) {
  supla_log(LOG_DEBUG, "supla_esp_gpio_rs_set_time_margin(%d)", value);
  if (value >= 0 && value <= 100) {
    rs_time_margin = value;
  } else {
    rs_time_margin = 110;
  }
}

void GPIO_ICACHE_FLASH supla_esp_gpio_rs_set_flag(
    supla_roller_shutter_cfg_t *rsCfg, unsigned _supla_int16_t flag) {
  if (rsCfg) {
    rsCfg->flags |= flag;
  }
}

void GPIO_ICACHE_FLASH supla_esp_gpio_rs_clear_flag(
    supla_roller_shutter_cfg_t *rsCfg, unsigned _supla_int16_t flag) {
  if (rsCfg) {
    rsCfg->flags &= ~flag;
  }
}

#ifdef RS_AUTOCALIBRATION_SUPPORTED
// Caution: this method may be called every 10 ms for each roller shutter.
// In some cases it may be too often (ESP may crash), so consider implementing
// some caching mechanism.
bool GPIO_ICACHE_FLASH
supla_esp_board_is_rs_in_move(supla_roller_shutter_cfg_t *rs_cfg);
#endif /*RS_AUTOCALIBRATION_SUPPORTED*/

bool GPIO_ICACHE_FLASH
supla_esp_gpio_is_rs_in_move(supla_roller_shutter_cfg_t *rs_cfg) {
#ifdef RS_AUTOCALIBRATION_SUPPORTED
  return supla_esp_board_is_rs_in_move(rs_cfg);
#else
  return false;
#endif /*RS_AUTOCALIBRATION_SUPPORTED*/
}

int GPIO_ICACHE_FLASH
supla_esp_gpio_rs_get_idx_by_ptr(supla_roller_shutter_cfg_t *rs_cfg) {
  for (int i = 0; i < RS_MAX_COUNT; i++) {
    if (&supla_rs_cfg[i] == rs_cfg) {
      return i;
    }
  }
  return -1;
}

bool GPIO_ICACHE_FLASH supla_esp_gpio_rs_is_autocal_enabled(int idx) {
  if (idx < 0 || idx > RS_MAX_COUNT) {
    return false;
  }
  return supla_esp_cfg.Time1[idx] == 0 && supla_esp_cfg.Time2[idx] == 0 &&
    (supla_rs_cfg[idx].up->channel_flags &
     SUPLA_CHANNEL_FLAG_RS_AUTO_CALIBRATION);
}

bool GPIO_ICACHE_FLASH supla_esp_gpio_rs_is_autocal_done(int idx) {
  if (idx < 0 || idx > RS_MAX_COUNT) {
    return false;
  }
  return supla_esp_cfg.AutoCalOpenTime[idx] > 0 &&
    supla_esp_cfg.AutoCalCloseTime[idx] > 0;
}

void GPIO_ICACHE_FLASH supla_esp_gpio_rs_check_if_autocal_is_needed(
    supla_roller_shutter_cfg_t *rs_cfg) {
  if (rs_cfg == NULL) {
    return;
  }
  int idx = supla_esp_gpio_rs_get_idx_by_ptr(rs_cfg);

  if (supla_esp_gpio_rs_is_autocal_enabled(idx) &&
      !supla_esp_gpio_rs_is_autocal_done(idx) && rs_cfg->autoCal_step == 0) {
    rs_cfg->performAutoCalibration = true;
  }
}

sint8 GPIO_ICACHE_FLASH
supla_esp_gpio_rs_get_current_position(supla_roller_shutter_cfg_t *rs_cfg) {
  sint8 result = -1;
  if (rs_cfg && *rs_cfg->position >= 100 && *rs_cfg->position <= 10100) {
    // we add 50 here in order to round position to closest integer
    // instead of rounding down, which could translate 1.99% to 1%
    result = (*rs_cfg->position - 100 + 50) / 100;
  }
  return result;
}

sint8 GPIO_ICACHE_FLASH
supla_esp_gpio_rs_get_current_tilt(supla_roller_shutter_cfg_t *rs_cfg) {
  sint8 result = 0;
  if (rs_cfg && *rs_cfg->tilt >= 100 && *rs_cfg->tilt <= 10100) {
    // we add 50 here in order to round tilt to closest integer
    // instead of rounding down, which could translate 1.99% to 1%
    result = (*rs_cfg->tilt - 100 + 50) / 100;
  }
  return result;
}

bool GPIO_ICACHE_FLASH
supla_esp_gpio_rs_is_tilt_set(supla_roller_shutter_cfg_t *rs_cfg) {
  bool result = true;
  if (rs_cfg && (*rs_cfg->tilt < 100 || *rs_cfg->tilt > 10100)) {
    result = false;
  }
  return result;
}

// calibration with manually provided times
void GPIO_ICACHE_FLASH supla_esp_gpio_rs_calibrate(
    supla_roller_shutter_cfg_t *rs_cfg, unsigned int full_time,
    unsigned int time, int pos) {

  if ((*rs_cfg->position < 100 || *rs_cfg->position > 10100) && full_time > 0) {

    supla_esp_gpio_rs_set_flag(rs_cfg, RS_VALUE_FLAG_CALIBRATION_IN_PROGRESS);
    full_time *= 1.1;  // 10% margin
    *rs_cfg->position = 0;
    *rs_cfg->tilt = 0;
    supla_esp_gpio_rs_clear_flag(rs_cfg, RS_VALUE_FLAG_TILT_IS_SET);

    if ((time / 1000) >= full_time) {
      *rs_cfg->position = pos;
      if (supla_esp_gpio_rs_is_tilt_supported(rs_cfg)) {
        *rs_cfg->tilt = pos;
      } else {
        *rs_cfg->tilt = 0;
        supla_esp_gpio_rs_clear_flag(rs_cfg, RS_VALUE_FLAG_TILT_IS_SET);
      }
      supla_esp_save_state(RS_SAVE_STATE_DELAY);
    }
  }
}

void GPIO_ICACHE_FLASH supla_esp_gpio_rs_check_motor(
    supla_roller_shutter_cfg_t *rs_cfg, bool moveUp, bool isRsInMove) {
  if (!rs_cfg) {
    return;
  }

  // ignore first RS_AUTOCAL_FILTERING_TIME_MS after movement started
  unsigned int t = system_get_time();
  if (t - rs_cfg->start_time < RS_AUTOCAL_FILTERING_TIME_MS * 1000) {
    return;
  }

  int idx = supla_esp_gpio_rs_get_idx_by_ptr(rs_cfg);
  if (idx >= 0) {
    if (supla_esp_gpio_rs_is_autocal_done(idx)) {
      if (!isRsInMove) {
        // we don't check if motor is working properly close to fully
        // open/closed positions
        if ((moveUp && supla_esp_gpio_rs_get_current_position(rs_cfg) > 5) ||
            (!moveUp && supla_esp_gpio_rs_get_current_position(rs_cfg) < 95)) {
          supla_esp_gpio_rs_set_flag(rs_cfg, RS_VALUE_FLAG_MOTOR_PROBLEM);
        }
      }
    }
  }
}

#define RS_DIRECTION_NONE 0
#define RS_DIRECTION_UP 2
#define RS_DIRECTION_DOWN 1

void GPIO_ICACHE_FLASH supla_esp_gpio_rs_set_relay_delayed(void *timer_arg) {
  supla_roller_shutter_cfg_t *rs_cfg =
    ((supla_roller_shutter_cfg_t *)timer_arg);
  if (rs_cfg->delayed_trigger.autoCal_request) {
    rs_cfg->autoCal_button_request = true;
    rs_cfg->delayed_trigger.autoCal_request = false;
  }
  supla_esp_gpio_rs_set_relay(rs_cfg, rs_cfg->delayed_trigger.value, 0, 0);

  // TODO reset delayed_trigger.value to 0
}

void GPIO_ICACHE_FLASH
supla_esp_gpio_rs_set_relay(supla_roller_shutter_cfg_t *rs_cfg, uint8 value,
    uint8 cancel_task, uint8 stop_delay) {
  if (rs_cfg == NULL) {
    return;
  }

  if (!rs_cfg->autoCal_button_request) {
    if (rs_cfg->autoCal_step > 0) {
      // abort autocalibration
      rs_cfg->autoCal_step = 0;
      *rs_cfg->full_opening_time = 0;
      *rs_cfg->full_closing_time = 0;
      *rs_cfg->auto_opening_time = 0;
      *rs_cfg->auto_closing_time = 0;
      *rs_cfg->position = 0;  // not calibrated
      *rs_cfg->tilt = 0;      // not calibrated or not used
      supla_esp_gpio_rs_clear_flag(rs_cfg, RS_VALUE_FLAG_TILT_IS_SET);
    }
  }

  unsigned int t = system_get_time();
  unsigned int delay_time = 0;

  if (cancel_task) {
    supla_esp_gpio_rs_cancel_task(rs_cfg);
  }

  os_timer_disarm(&rs_cfg->delayed_trigger.timer);

  if (value == RS_RELAY_OFF) {
    if (RS_STOP_DELAY && stop_delay == 1 && rs_cfg->start_time > 0 &&
        rs_cfg->stop_time == 0 && t >= rs_cfg->start_time &&
        (t - rs_cfg->start_time) / 1000 < RS_STOP_DELAY) {
      delay_time = RS_STOP_DELAY - (t - rs_cfg->start_time) / 1000 + 1;
    }

  } else {
    // keep last direction for SBS handling in devconn
    rs_cfg->last_direction = value;

    supla_esp_gpio_rs_clear_flag(rs_cfg, RS_VALUE_FLAG_CALIBRATION_FAILED);
    supla_esp_gpio_rs_clear_flag(rs_cfg, RS_VALUE_FLAG_MOTOR_PROBLEM);
    supla_esp_gpio_rs_clear_flag(rs_cfg, RS_VALUE_FLAG_CALIBRATION_LOST);

    supla_relay_cfg_t *rel = value == RS_RELAY_UP ? rs_cfg->down : rs_cfg->up;

    if (__supla_esp_gpio_relay_is_hi(rel)) {
      supla_esp_gpio_relay_hi(rel->gpio_id, 0);
      os_delay_us(10000);

      t = system_get_time();
    }

    if (RS_START_DELAY && rs_cfg->start_time == 0 && rs_cfg->stop_time > 0 &&
        t >= rs_cfg->stop_time &&
        (t - rs_cfg->stop_time) / 1000 < RS_START_DELAY) {
      delay_time = RS_START_DELAY - (t - rs_cfg->stop_time) / 1000 + 1;
    }
  }

  // supla_log(LOG_DEBUG, "VALUE: %i", value);

  if (delay_time > 100) {
    rs_cfg->delayed_trigger.autoCal_request = rs_cfg->autoCal_button_request;
    rs_cfg->delayed_trigger.value = value;

    // supla_log(LOG_DEBUG, "Delay: %i", delay_time);

    os_timer_setfn(&rs_cfg->delayed_trigger.timer,
        supla_esp_gpio_rs_set_relay_delayed, rs_cfg);
    os_timer_arm(&rs_cfg->delayed_trigger.timer, delay_time, 0);

    rs_cfg->autoCal_button_request = false;
    return;
  }

  if (value == RS_RELAY_UP) {
    supla_esp_gpio_relay_hi(rs_cfg->up->gpio_id, 1);
  } else if (value == RS_RELAY_DOWN) {
    supla_esp_gpio_relay_hi(rs_cfg->down->gpio_id, 1);
  } else {
    supla_esp_gpio_relay_hi(rs_cfg->up->gpio_id, 0);
    supla_esp_gpio_relay_hi(rs_cfg->down->gpio_id, 0);
  }
  rs_cfg->autoCal_button_request = false;
}

uint8 GPIO_ICACHE_FLASH
supla_esp_gpio_rs_get_value(supla_roller_shutter_cfg_t *rs_cfg) {
  if (rs_cfg != NULL) {
    if (1 == __supla_esp_gpio_relay_is_hi(rs_cfg->up)) {
      return RS_RELAY_UP;
    } else if (1 == __supla_esp_gpio_relay_is_hi(rs_cfg->down)) {
      return RS_RELAY_DOWN;
    }
  }

  return RS_RELAY_OFF;
}

void GPIO_ICACHE_FLASH supla_esp_gpio_rs_move_position(
    supla_roller_shutter_cfg_t *rs_cfg, unsigned int full_time_ms,
    unsigned int *time, uint8 up, bool isRsInMove) {

  if ((*rs_cfg->position) < 100 || (*rs_cfg->position) > 10100 ||
      full_time_ms == 0)
    return;

  int last_pos = *rs_cfg->position;
  int last_tilt = *rs_cfg->tilt;

  unsigned int full_time = full_time_ms * 1000;
  unsigned int full_tilting_time = *rs_cfg->tilt_change_time * 1000;
  unsigned int full_position_time = full_time;

  switch (*rs_cfg->tilt_type) {
    case FB_TILT_TYPE_KEEP_POSITION_WHILE_TILTING:
    case FB_TILT_TYPE_TILTING_ONLY_AT_FULLY_CLOSED:
      full_position_time -= full_tilting_time;
      break;
    case FB_TILT_TYPE_CHANGE_POSITION_WHILE_TILTING:
      break;
  }

  // remaining tilt in 0.01% units, so 10000 is 100%
  unsigned int remaining_tilt =
    up ? *rs_cfg->tilt - 100 : 10100 - *rs_cfg->tilt;
  unsigned int remaining_tilt_time =
    (1.0 * remaining_tilt / 10000.0) * full_tilting_time;

  unsigned int remaining_position =
    up ? *rs_cfg->position - 100 : 10100 - *rs_cfg->position;
  unsigned int remaining_position_time =
    (1.0 * remaining_position / 10000.0) * full_position_time;

  unsigned int time_delta = 0;

  if (*rs_cfg->tilt_type == FB_TILT_TYPE_TILTING_ONLY_AT_FULLY_CLOSED &&
      *rs_cfg->position < 10100) {
    remaining_tilt_time = 0;
  }

  // adjust tilt change
  if (remaining_tilt_time > 0) {
    int tilt_delta = 0;
    if (remaining_tilt_time <= *time) {
      tilt_delta = up ? *rs_cfg->tilt - 100 : 10100 - *rs_cfg->tilt;
      *rs_cfg->tilt = up ? 100 : 10100;
    } else {
      tilt_delta = 10000.0 * *time / full_tilting_time;
      *rs_cfg->tilt += up ? -tilt_delta : tilt_delta;
    }
    time_delta = (1.0 * tilt_delta) * full_tilting_time / 10000.0;
  }

  // skip position change when position change is not happening during tilting
  if (time_delta > 0 &&
      (*rs_cfg->tilt_type == FB_TILT_TYPE_KEEP_POSITION_WHILE_TILTING ||
       *rs_cfg->tilt_type == FB_TILT_TYPE_TILTING_ONLY_AT_FULLY_CLOSED)) {
    remaining_position_time = 0;
  }

  // adjust position change
  if (remaining_position_time > 0) {
    int position_delta = 0;
    if (remaining_position_time <= *time) {
      position_delta = up ? *rs_cfg->position - 100 : 10100 - *rs_cfg->position;
      *rs_cfg->position = up ? 100 : 10100;
    } else {
      position_delta = 10000.0 * *time / full_position_time;
      *rs_cfg->position += up ? -position_delta : position_delta;
    }
    time_delta = (1.0 * position_delta) * full_position_time / 10000.0;
  }

  // make sure that remaining ms of time are not lost, but will be used on next
  // iteration
  if (time_delta > *time) {
    *time = 0;
  } else {
    *time -= time_delta;
  }

  // State saving if position or tilt has changed
  if (last_pos != *rs_cfg->position || last_tilt != *rs_cfg->tilt) {
    supla_esp_save_state(RS_SAVE_STATE_DELAY);
  }

  // Handling of extreem motor positions (fully open/closed) with time margin
  if (((*rs_cfg->position) == 100 && up == 1 &&
       (*rs_cfg->tilt == 0 || *rs_cfg->tilt == -1 || *rs_cfg->tilt == 100)) ||
      ((*rs_cfg->position) == 10100 && up == 0 &&
       (*rs_cfg->tilt == 0 || *rs_cfg->tilt == -1 || *rs_cfg->tilt == 10100))) {
    // This time margin starts counting time when RS reach either 0 or 100%. So
    // it adds "full_time * 110%" of enabled relay after it was expected to
    // reach fully open or closed RS.
    // This part of code is executed only for "MOVE UP/DOWN" actions (either
    // from physical buttons or in app). It is not executed when new position
    // is given in % value.
    if ((*time) / 1000 >=
        (int)(full_time_ms * (1.0 * rs_time_margin / 100.0))) {
      int idx = supla_esp_gpio_rs_get_idx_by_ptr(rs_cfg);
      if (idx >= 0) {
        if (supla_esp_gpio_rs_is_autocal_done(idx)) {
          // when RS is fully closed/opened, we add 110% of time to finilize
          // operation. In such case we expect that motor will shut down
          // within that 110% time margin. If it doesn't, it means that we
          // lost RS calibration (it is still in move, while it should already
          // stop.
          if (isRsInMove) {
            supla_esp_gpio_rs_set_flag(rs_cfg, RS_VALUE_FLAG_CALIBRATION_LOST);
          }
        }
      }
      supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_OFF, 0, 0);
      supla_log(LOG_DEBUG, "Timeout full_time * %d%", rs_time_margin);
    }

    return;
  }
}

uint8 GPIO_ICACHE_FLASH supla_esp_gpio_rs_time_margin(
    supla_roller_shutter_cfg_t *rs_cfg, unsigned int full_time,
    unsigned int time, uint8 m) {
  return (full_time > 0 && ((time / 10) / full_time) < m) ? 1 : 0;
}

void GPIO_ICACHE_FLASH supla_esp_gpio_rs_task_processing(
    supla_roller_shutter_cfg_t *rs_cfg, bool isRsInMove,
    unsigned int full_opening_time, unsigned int full_closing_time) {
  if (rs_cfg->task.state == RS_TASK_INACTIVE || rs_cfg->autoCal_step > 0) {
    return;
  }

  if (rs_cfg->performAutoCalibration) {
    supla_esp_gpio_rs_start_autoCal(rs_cfg);
    return;
  }

  // Start calibration is needed
  if (*rs_cfg->position < 100 || *rs_cfg->position > 10100) {
    if (0 == __supla_esp_gpio_relay_is_hi(rs_cfg->down) &&
        0 == __supla_esp_gpio_relay_is_hi(rs_cfg->up) &&
        full_opening_time > 0 && full_closing_time > 0) {
      if (rs_cfg->task.position < 50) {
        supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_UP, 0, 0);
      } else {
        supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_DOWN, 0, 0);
      }
    }
    return;
  }

  int raw_position = (*rs_cfg->position) - 100;
  int raw_tilt = (*rs_cfg->tilt) - 100;
  if (raw_tilt < 0) {
    raw_tilt = 0;
  }
  int task_position = (int)rs_cfg->task.position * 100;
  int task_tilt = (int)rs_cfg->task.tilt * 100;

  // delta_pos_ variables defines how much position should be reduced/increased
  // in order to match requested position after tilting
  int delta_pos_up = 0;
  int delta_pos_down = 0;

  // we calculate what will be actual position after setting
  // the requested tilt. Then if that position is higher than task position
  // we should start movement down, if lower, then move up, if ~equal, then
  // just proceed with tilting.
  // Positioning should happen until we reach task_position corrected by
  // delta_pos_up/down

  int raw_position_after_pre_tilt = raw_position;

  if (task_tilt >= 0) {
    switch (*rs_cfg->tilt_type) {
      case FB_TILT_TYPE_CHANGE_POSITION_WHILE_TILTING: {
        // adjust position so it will match requested value after tilting
        if (rs_cfg->task.state == RS_TASK_SETTING_POSITION
            || (task_position != -100 && task_tilt != -100)) {

          int tilt_direction =
            (task_tilt > raw_tilt) ? RS_DIRECTION_DOWN : RS_DIRECTION_UP;
          int delta_tilt =
            (task_tilt > raw_tilt) ?
            (task_tilt - raw_tilt) : (raw_tilt - task_tilt);
          int tilting_time =
            1.0 * (*rs_cfg->tilt_change_time) * delta_tilt / 10000.0;


          raw_position_after_pre_tilt =
            (tilt_direction == RS_DIRECTION_DOWN) ?
            raw_position + 10000.0 * tilting_time / full_closing_time :
            raw_position - 10000.0 * tilting_time / full_opening_time;

          int required_correction_up = 10000 - task_tilt;
          unsigned int tilt_correction_time_up =
            (1.0 * required_correction_up) * (*rs_cfg->tilt_change_time);
          delta_pos_down = tilt_correction_time_up / full_opening_time;
          if (raw_tilt < 10000) {

          }

          if (task_position + delta_pos_down > 10000) {
            delta_pos_down = 10000 - task_position;
          }

          int required_tilt_movement_down = task_tilt;
          unsigned int tilt_time_down =
            (1.0 * required_tilt_movement_down) * (*rs_cfg->tilt_change_time);
          delta_pos_up = tilt_time_down / full_closing_time;

          if (task_position < delta_pos_up) {
            delta_pos_up = task_position;
          }
        }
        break;
      }
      case FB_TILT_TYPE_TILTING_ONLY_AT_FULLY_CLOSED: {
        // ignore tilt when position <100
        break;
      }
    }
  }

  // As a first step we will start movement in order to reach desired position
  if (rs_cfg->task.state == RS_TASK_ACTIVE) {
    rs_cfg->task.state = RS_TASK_SETTING_POSITION;

    if (task_position != -100) {
      if (raw_position_after_pre_tilt > task_position) {
        rs_cfg->task.direction = RS_DIRECTION_UP;
        supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_UP, 0, 0);
#ifdef SUPLA_DEBUG
        supla_log(LOG_DEBUG, "task go UP %i, %i, %i, %i", raw_position,
            task_position, raw_tilt, task_tilt);
#endif /*SUPLA_DEBUG*/
      } else if (raw_position_after_pre_tilt < task_position) {
        rs_cfg->task.direction = RS_DIRECTION_DOWN;
        supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_DOWN, 0, 0);
#ifdef SUPLA_DEBUG
        supla_log(LOG_DEBUG, "task go DOWN %i, %i, %i, %i", raw_position,
            task_position, raw_tilt, task_tilt);
#endif /*SUPLA_DEBUG*/
      }
    }
  }

  // If postion change is not needed or we already reached desired position
  // we'll start tilting
  if (rs_cfg->task.state == RS_TASK_SETTING_POSITION &&
      rs_cfg->task.direction == RS_DIRECTION_NONE) {
    rs_cfg->task.state = RS_TASK_SETTING_TILT;
    if (raw_tilt > task_tilt && task_tilt != -100) {
      rs_cfg->task.direction = RS_DIRECTION_UP;
      supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_UP, 0, 0);
#ifdef SUPLA_DEBUG
      supla_log(LOG_DEBUG, "task go UP %i, %i, %i, %i", raw_position,
          task_position, raw_tilt, task_tilt);
#endif /*SUPLA_DEBUG*/
    } else if (raw_tilt < task_tilt && task_tilt != -100) {
      rs_cfg->task.direction = RS_DIRECTION_DOWN;
      supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_DOWN, 0, 0);
#ifdef SUPLA_DEBUG
      supla_log(LOG_DEBUG, "task go DOWN %i, %i, %i, %i", raw_position,
          task_position, raw_tilt, task_tilt);
#endif /*SUPLA_DEBUG*/
    } else {
#ifdef SUPLA_DEBUG
      supla_log(LOG_DEBUG, "task finished, position and tilt already set");
#endif /*SUPLA_DEBUG*/
      rs_cfg->task.state = RS_TASK_INACTIVE;
      rs_cfg->task.direction = RS_DIRECTION_NONE;
      supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_OFF, 0, 0);
    }
  }

  if (rs_cfg->task.state == RS_TASK_SETTING_POSITION) {
    // Task was already started, so we are in the middle of processing
    if ((rs_cfg->task.direction == RS_DIRECTION_UP &&
          raw_position <= task_position - delta_pos_up) ||
        (rs_cfg->task.direction == RS_DIRECTION_DOWN &&
         raw_position >= task_position + delta_pos_down)) {

      uint8 time_margin = 5;  // default 5% margin
      // default rs_time_margin is set to 1.1 -> 110% for button movement
      // If it is changed to other value, then we apply it also as margin for
      // percantage control
      if (rs_time_margin < 110) {
        time_margin = rs_time_margin;
        if (isRsInMove && time_margin < 50) {
          time_margin = 50;
        }
      }

      // time margin is added when RS/FB reach fully open or fully closed
      // position. In case of some FB type "fully open/close" means that
      // tilt also has to be set to 100%
      if (raw_position == 0 &&
          1 == supla_esp_gpio_rs_time_margin(rs_cfg, full_opening_time,
            rs_cfg->up_time, time_margin)) {
      } else if ((raw_position == 10000
            && (*rs_cfg->tilt_type == FB_TILT_TYPE_CHANGE_POSITION_WHILE_TILTING
              || *rs_cfg->tilt_type == FB_TILT_TYPE_NOT_SUPPORTED
              || raw_tilt == 10000))
          && 1 == supla_esp_gpio_rs_time_margin(rs_cfg, full_closing_time,
            rs_cfg->down_time, time_margin)
          ) {
      } else {
        int idx = supla_esp_gpio_rs_get_idx_by_ptr(rs_cfg);
        if (idx >= 0 &&
            (rs_cfg->task.position == 0 ||
             (rs_cfg->task.position == 100
              && (*rs_cfg->tilt_type == FB_TILT_TYPE_CHANGE_POSITION_WHILE_TILTING
              || *rs_cfg->tilt_type == FB_TILT_TYPE_NOT_SUPPORTED
              || rs_cfg->task.tilt == 100))
              )) {
          if (supla_esp_gpio_rs_is_autocal_done(idx)) {
            // when RS is fully closed/opened, we add 10% of time to finilize
            // operation. In such case we expect that motor will shut down
            // within that 10% time margin. If it doesn't, it means that we
            // lost RS calibration (it is still in move, while it should already
            // stop.
            if (isRsInMove) {
              supla_esp_gpio_rs_set_flag(rs_cfg,
                  RS_VALUE_FLAG_CALIBRATION_LOST);
            }
          }
        }

        rs_cfg->task.direction = RS_DIRECTION_NONE;
        supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_OFF, 0, 0);

#ifdef SUPLA_DEBUG
        supla_log(LOG_DEBUG, "task position setting completed");
#endif /*SUPLA_DEBUG*/
      }
    }
  }

  if (rs_cfg->task.state == RS_TASK_SETTING_TILT) {
    if ((rs_cfg->task.direction == RS_DIRECTION_UP &&
          raw_tilt <= task_tilt) ||
        (rs_cfg->task.direction == RS_DIRECTION_DOWN &&
         raw_tilt >= task_tilt)) {
      rs_cfg->task.state = RS_TASK_INACTIVE;
      rs_cfg->task.direction = RS_DIRECTION_NONE;
      supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_OFF, 0, 0);
#ifdef SUPLA_DEBUG
      supla_log(LOG_DEBUG, "task tilt setting completed");
#endif /*SUPLA_DEBUG*/
    }
  }
}

void GPIO_ICACHE_FLASH
supla_esp_gpio_rs_start_autoCal(supla_roller_shutter_cfg_t *rs_cfg) {
  // Automatic calibration steps:
  // 1. Move RS up
  // 2. Move RS down and store down_time
  // 3. Move RS up and store up_time
  // 4. Finish auto calibration
  // 5. Execute last stored task

  rs_cfg->performAutoCalibration = false;
  rs_cfg->autoCal_step = 1;
  rs_cfg->autoCal_button_request = true;
  supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_UP, 0, 0);
}
void GPIO_ICACHE_FLASH
supla_esp_gpio_rs_calibration_failed(supla_roller_shutter_cfg_t *rs_cfg) {

  rs_cfg->autoCal_step = 0;
  *rs_cfg->auto_opening_time = 0;
  *rs_cfg->auto_closing_time = 0;
  *rs_cfg->position = 0;
  *rs_cfg->tilt = 0;
  supla_esp_gpio_rs_set_flag(rs_cfg, RS_VALUE_FLAG_CALIBRATION_FAILED);
  supla_esp_gpio_rs_clear_flag(rs_cfg, RS_VALUE_FLAG_CALIBRATION_IN_PROGRESS);

  // off with cancel task
  supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_OFF, 1, 0);
}

void GPIO_ICACHE_FLASH supla_esp_gpio_rs_autocalibrate(
    supla_roller_shutter_cfg_t *rs_cfg, bool isRsInMove) {
  if (rs_cfg == NULL || rs_cfg->autoCal_step == 0) {
    supla_esp_gpio_rs_clear_flag(rs_cfg, RS_VALUE_FLAG_CALIBRATION_IN_PROGRESS);
    return;
  }
  supla_esp_gpio_rs_set_flag(rs_cfg, RS_VALUE_FLAG_CALIBRATION_IN_PROGRESS);

  if (rs_cfg->up_time < RS_AUTOCAL_FILTERING_TIME_MS * 1000 &&
      rs_cfg->down_time < RS_AUTOCAL_FILTERING_TIME_MS * 1000) {
    // we ignore first RS_AUTOCAL_FILTERING_TIME_MS ms during
    // autocalibration
    return;
  }

  switch (rs_cfg->autoCal_step) {
    case 1: {
              if (!isRsInMove) {
                rs_cfg->autoCal_step = 2;
                rs_cfg->autoCal_button_request = true;
                supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_DOWN, 0, 0);
              } else if (rs_cfg->up_time > RS_AUTOCAL_MAX_TIME_MS * 1000) {
                supla_esp_gpio_rs_calibration_failed(rs_cfg);
              }
              break;
            }
    case 2: {
              if (!isRsInMove) {
                if (rs_cfg->down_time < RS_AUTOCAL_MIN_TIME_MS * 1000) {
                  // calibration failed
                  supla_esp_gpio_rs_calibration_failed(rs_cfg);
                } else {
                  rs_cfg->autoCal_step = 3;
                  *rs_cfg->auto_closing_time = rs_cfg->down_time / 1000;
                  rs_cfg->autoCal_button_request = true;
                  supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_UP, 0, 0);
                }
              } else if (rs_cfg->down_time > RS_AUTOCAL_MAX_TIME_MS * 1000) {
                supla_esp_gpio_rs_calibration_failed(rs_cfg);
              }
              break;
            }
    case 3: {
              if (!isRsInMove) {
                if (rs_cfg->up_time < RS_AUTOCAL_MIN_TIME_MS * 1000) {
                  // calibration failed
                  supla_esp_gpio_rs_calibration_failed(rs_cfg);
                } else {
                  rs_cfg->autoCal_step = 0;
                  *rs_cfg->auto_opening_time = rs_cfg->up_time / 1000;

                  *rs_cfg->position = 100;  // fully open and calibrated
                  if (supla_esp_gpio_rs_is_tilt_supported(rs_cfg)) {
                    *rs_cfg->tilt = 100;
                  } else {
                    *rs_cfg->tilt = 0;
                    supla_esp_gpio_rs_clear_flag(rs_cfg,
                                                 RS_VALUE_FLAG_TILT_IS_SET);
                  }
                  supla_esp_gpio_rs_clear_flag(rs_cfg,
                      RS_VALUE_FLAG_CALIBRATION_IN_PROGRESS);
                  supla_esp_save_state(RS_SAVE_STATE_DELAY);
                  supla_esp_cfg_save(&supla_esp_cfg);
                  rs_cfg->autoCal_button_request = true;
                  supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_OFF, 0, 0);
                }
              } else if (rs_cfg->up_time > RS_AUTOCAL_MAX_TIME_MS * 1000) {
                supla_esp_gpio_rs_calibration_failed(rs_cfg);
              }
              break;
            }
  }
}

void GPIO_ICACHE_FLASH supla_esp_gpio_rs_timer_cb(void *timer_arg) {
  supla_roller_shutter_cfg_t *rs_cfg = (supla_roller_shutter_cfg_t *)timer_arg;

  if (supla_esp_gpio_init_time == 0)
    return;

  unsigned int t = system_get_time();
  bool isRsInMove = supla_esp_gpio_is_rs_in_move(rs_cfg);

  int idx = supla_esp_gpio_rs_get_idx_by_ptr(rs_cfg);
  unsigned int full_opening_time = 0;
  unsigned int full_closing_time = 0;
  if (supla_esp_gpio_rs_is_autocal_enabled(idx)) {
    full_opening_time = *rs_cfg->auto_opening_time;
    full_closing_time = *rs_cfg->auto_closing_time;
    if (full_opening_time == 0 && full_closing_time == 0) {
      *rs_cfg->position = 0;
      *rs_cfg->tilt = 0;
      supla_esp_gpio_rs_clear_flag(rs_cfg, RS_VALUE_FLAG_TILT_IS_SET);
    }
  } else {
    full_opening_time = *rs_cfg->full_opening_time;
    full_closing_time = *rs_cfg->full_closing_time;
    if (*rs_cfg->auto_closing_time != 0 || *rs_cfg->auto_opening_time != 0 ||
        rs_cfg->autoCal_step != 0) {
      *rs_cfg->auto_opening_time = 0;
      *rs_cfg->auto_closing_time = 0;
      *rs_cfg->position = 0;
      *rs_cfg->tilt = 0;
      supla_esp_gpio_rs_clear_flag(rs_cfg, RS_VALUE_FLAG_TILT_IS_SET);
      rs_cfg->autoCal_step = 0;
    }
  }

  if (1 == __supla_esp_gpio_relay_is_hi(rs_cfg->up) ||
      1 == __supla_esp_gpio_relay_is_hi(rs_cfg->down)) {
    if (supla_esp_gpio_rs_is_autocal_enabled(idx)) {
      if (!rs_cfg->detectedPowerConsumption) {
        rs_cfg->detectedPowerConsumption = isRsInMove;
      }
      if (!rs_cfg->detectedPowerConsumption) {
        if (t - rs_cfg->start_time < 2000 * 1000) {  // < 2s
          rs_cfg->last_time = t;
        }
      }
    }
  } else {
    rs_cfg->detectedPowerConsumption = false;
  }

  if (1 == __supla_esp_gpio_relay_is_hi(rs_cfg->up)) {
    rs_cfg->down_time = 0;
    rs_cfg->up_time += (t - rs_cfg->last_time);

    // supla_log(LOG_DEBUG, "isRsInMove %d, up %d, down %d, cur_pos %d",
    // isRsInMove,
    //     rs_cfg->up_time, rs_cfg->down_time, *rs_cfg->position - 100);

    if (rs_cfg->up_time > 0) {
      supla_esp_gpio_rs_check_motor(rs_cfg, true, isRsInMove);  // true  = up
    }
    supla_esp_gpio_rs_autocalibrate(rs_cfg, isRsInMove);
    supla_esp_gpio_rs_calibrate(rs_cfg, full_opening_time, rs_cfg->up_time,
        100);
    supla_esp_gpio_rs_move_position(rs_cfg, full_opening_time, &rs_cfg->up_time,
        1, isRsInMove);

  } else if (1 == __supla_esp_gpio_relay_is_hi(rs_cfg->down)) {

    rs_cfg->down_time += (t - rs_cfg->last_time);
    rs_cfg->up_time = 0;

    if (rs_cfg->down_time > 0) {
      supla_esp_gpio_rs_check_motor(rs_cfg, false, isRsInMove);  // false = down
    }
    supla_esp_gpio_rs_autocalibrate(rs_cfg, isRsInMove);
    supla_esp_gpio_rs_calibrate(rs_cfg, full_closing_time, rs_cfg->down_time,
        10100);
    supla_esp_gpio_rs_move_position(rs_cfg, full_closing_time,
        &rs_cfg->down_time, 0, isRsInMove);
  } else {
    // if relays are off and we are not during autocal, then reset "calibration
    // in progress" flag
    if (rs_cfg->autoCal_step == 0) {
      supla_esp_gpio_rs_clear_flag(rs_cfg,
          RS_VALUE_FLAG_CALIBRATION_IN_PROGRESS);
    }

    rs_cfg->up_time = 0;
    rs_cfg->down_time = 0;
  }

  supla_esp_gpio_rs_check_if_autocal_is_needed(rs_cfg);
  supla_esp_gpio_rs_task_processing(rs_cfg, isRsInMove, full_opening_time,
      full_closing_time);

  if (t - rs_cfg->last_comm_time >= 200000) {  // 500 ms.

    if (rs_cfg->last_position != *rs_cfg->position ||
        rs_cfg->last_flags != rs_cfg->flags ||
        rs_cfg->last_tilt != *rs_cfg->tilt) {

      supla_log(LOG_DEBUG,
          "New RS value: pos %d (last %d), tilt %d (last %d),"
          "flags %4x (last %4x)",
          *rs_cfg->position, rs_cfg->last_position, *rs_cfg->tilt,
          rs_cfg->last_tilt, rs_cfg->flags, rs_cfg->last_flags);

      rs_cfg->last_position = *rs_cfg->position;
      rs_cfg->last_tilt = *rs_cfg->tilt;
      rs_cfg->last_flags = rs_cfg->flags;

      char value[SUPLA_CHANNELVALUE_SIZE] = {};
      sint8 pos = -1;
      sint8 tilt = -1;
      if (*rs_cfg->tilt_type == FB_TILT_TYPE_NOT_SUPPORTED) {
        // Roller shutter
        pos = supla_esp_gpio_rs_get_current_position(rs_cfg);
        TDSC_RollerShutterValue *rsValue = (TDSC_RollerShutterValue *)value;
        rsValue->position = pos;
        rsValue->reserved1 = 0;
        supla_esp_gpio_rs_clear_flag(rs_cfg, RS_VALUE_FLAG_TILT_IS_SET);
        rsValue->flags = rs_cfg->flags;
      } else if (*rs_cfg->tilt_type != FB_TILT_TYPE_NOT_SUPPORTED) {
        // Facade blind
        pos = supla_esp_gpio_rs_get_current_position(rs_cfg);
        tilt = supla_esp_gpio_rs_get_current_tilt(rs_cfg);
        if (supla_esp_gpio_rs_is_tilt_set(rs_cfg)) {
          supla_esp_gpio_rs_set_flag(rs_cfg, RS_VALUE_FLAG_TILT_IS_SET);
        } else {
          supla_esp_gpio_rs_clear_flag(rs_cfg, RS_VALUE_FLAG_TILT_IS_SET);
        }
        TDSC_FacadeBlindValue *fbValue = (TDSC_FacadeBlindValue *)value;
        fbValue->position = pos;
        fbValue->tilt = tilt;
        fbValue->flags = rs_cfg->flags;
      }

      supla_esp_channel_value__changed(rs_cfg->up->channel, value);

#ifdef BOARD_ON_ROLLERSHUTTER_POSITION_CHANGED
      supla_esp_board_on_rollershutter_position_changed(rs_cfg->up->channel,
          pos, tilt);
#endif /*BOARD_ON_ROLLERSHUTTER_POSITION_CHANGED*/
    }

    // TODO: move timeout based relay switch off out of current "if"
    // TODO: reset up/down_time to 0 just after stop
    // 10 min. - timeout
    if (rs_cfg->up_time > 600 * 1000 * 1000 ||
        rs_cfg->down_time > 600 * 1000 * 1000) {
      supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_OFF, 0, 0);
    }

    // supla_log(LOG_DEBUG, "UT: %i, DT: %i, FOT: %i, FCT: %i, pos: %i",
    // rs_cfg->up_time, rs_cfg->down_time, *rs_cfg->full_opening_time,
    //*rs_cfg->full_closing_time, *rs_cfg->position);
    rs_cfg->last_comm_time = t;
  }
  rs_cfg->last_time = t;
}

void GPIO_ICACHE_FLASH
supla_esp_gpio_rs_cancel_task(supla_roller_shutter_cfg_t *rs_cfg) {

  if (rs_cfg == NULL) {
    return;
  }

  // supla_log(LOG_DEBUG, "Task canceled");

  rs_cfg->task.state = RS_TASK_INACTIVE;
  rs_cfg->task.position = 0;
  rs_cfg->task.tilt = 0;
  rs_cfg->task.direction = RS_DIRECTION_NONE;

  supla_esp_save_state(RS_SAVE_STATE_DELAY);
}

void GPIO_ICACHE_FLASH supla_esp_gpio_rs_add_task(int idx, sint8 position,
    sint8 tilt) {

  if (idx < 0 || idx >= RS_MAX_COUNT) {
    return;
  }

  if (position > 100) {
    position = 100;
  }
  if (tilt > 100) {
    tilt = 100;
  }

  sint8 current_position =
    supla_esp_gpio_rs_get_current_position(&supla_rs_cfg[idx]);
  sint8 current_tilt = supla_esp_gpio_rs_get_current_tilt(&supla_rs_cfg[idx]);

  if (current_position == position || position == -1) {
    if (!supla_esp_gpio_rs_is_tilt_supported(&supla_rs_cfg[idx]) ||
        current_tilt == tilt || tilt == -1) {
      return;
    }
  }

  // If we are in the middle of processing other task and only tilt
  // was changed in new task, then we keep other's task position
  if (supla_rs_cfg[idx].task.state != RS_TASK_INACTIVE) {
    if (supla_rs_cfg[idx].task.state != RS_TASK_SETTING_TILT
      && position == -1) {
      position = supla_rs_cfg[idx].task.position;
    }
    if (tilt == -1) {
      tilt = supla_rs_cfg[idx].task.tilt;
    }
  }

//  if (tilt == -1) {
//    tilt = current_tilt;
//  }

  if (*supla_rs_cfg[idx].tilt_type ==
      FB_TILT_TYPE_TILTING_ONLY_AT_FULLY_CLOSED) {
    if (position != 100) {
      if (!(position == -1 && *supla_rs_cfg[idx].position == 10100)) {
        tilt = 0;
      }
    }
  }

  supla_rs_cfg[idx].task.position = position;
  supla_rs_cfg[idx].task.tilt = tilt;
  supla_rs_cfg[idx].task.direction = RS_DIRECTION_NONE;
  supla_rs_cfg[idx].task.state = RS_TASK_ACTIVE;

  supla_esp_save_state(RS_SAVE_STATE_DELAY);
}

bool GPIO_ICACHE_FLASH supla_esp_gpio_rs_apply_new__times(int idx,
                                                          int close_time_ms,
                                                          int open_time_ms,
                                                          int tilt_time_ms,
                                                          bool save) {
  supla_log(LOG_DEBUG, "Apply new times[%d]: ct %i, ot %i, tt %i", idx,
            close_time_ms, open_time_ms, tilt_time_ms);
  bool resetRsConfig = false;
  bool result = false;
  if (idx >= RS_MAX_COUNT || supla_rs_cfg[idx].up == NULL ||
      supla_rs_cfg[idx].down == NULL) {
    return false;
  }

  if (supla_rs_cfg[idx].up->channel_flags &
      SUPLA_CHANNEL_FLAG_RS_AUTO_CALIBRATION) {
    // Handling of time when auto calibration is supported
    if (close_time_ms != 0 || open_time_ms != 0) {
      if (supla_esp_gpio_rs_is_autocal_enabled(idx)) {
        supla_esp_cfg.AutoCalOpenTime[idx] = 0;
        supla_esp_cfg.AutoCalCloseTime[idx] = 0;
        resetRsConfig = true;
      } else {
        if (close_time_ms != supla_esp_cfg.Time2[idx] ||
            open_time_ms != supla_esp_cfg.Time1[idx]) {
          resetRsConfig = true;
        }
      }
    } else {
      // ct_ms == 0 && ot_ms == 0
      if (!supla_esp_gpio_rs_is_autocal_enabled(idx)) {
        // enable auto calibration
        resetRsConfig = true;
      }
    }
  } else {
    // Default behavior when auto calibration is not supported
    if (close_time_ms != supla_esp_cfg.Time2[idx] ||
        open_time_ms != supla_esp_cfg.Time1[idx]) {
      resetRsConfig = true;
    }
  }
  if (resetRsConfig) {
    supla_esp_cfg.Time2[idx] = close_time_ms;
    supla_esp_cfg.Time1[idx] = open_time_ms;
    supla_esp_cfg.AutoCalOpenTime[idx] = 0;
    supla_esp_cfg.AutoCalCloseTime[idx] = 0;

    // Reset position to 0. It means that RS is not calibrated
    supla_esp_state.rs_position[idx] = 0;
    supla_esp_state.tilt[idx] = 0;
    supla_esp_gpio_rs_clear_flag(&supla_rs_cfg[idx], RS_VALUE_FLAG_TILT_IS_SET);

    result = true;
  }

  if (tilt_time_ms != -1 && tilt_time_ms != supla_esp_cfg.Time3[idx]) {
    supla_esp_cfg.Time3[idx] = tilt_time_ms;
  }

  if (save) {
    supla_esp_save_state(0);
    supla_esp_cfg_save(&supla_esp_cfg);
  }

  // calibration is not needed if times are already set
  if (supla_rs_cfg[idx].performAutoCalibration &&
      (supla_esp_cfg.Time1[idx] > 0 || supla_esp_cfg.Time2[idx] > 0)) {
    supla_rs_cfg[idx].performAutoCalibration = false;
  }

  return result;
}

void GPIO_ICACHE_FLASH supla_esp_gpio_rs_apply_new_times(int idx,
                                                         int close_time_ms,
                                                         int open_time_ms,
                                                         int tilt_time_ms) {
  supla_esp_gpio_rs_apply_new__times(idx, close_time_ms, open_time_ms,
                                     tilt_time_ms, true);
}

bool GPIO_ICACHE_FLASH
supla_esp_gpio_rs_is_tilt_supported(supla_roller_shutter_cfg_t *rs_cfg) {
  if (rs_cfg == NULL) {
    return false;
  }

  if (*rs_cfg->tilt_change_time == 0) {
    return false;
  }

  return true;
}

bool GPIO_ICACHE_FLASH
supla_esp_gpio_is_rs(supla_roller_shutter_cfg_t *rs_cfg) {
  if (rs_cfg && *rs_cfg->tilt_type == 0) {
    return true;
  }
  return false;
}

bool GPIO_ICACHE_FLASH
supla_esp_gpio_is_fb(supla_roller_shutter_cfg_t *rs_cfg) {
  if (rs_cfg && *rs_cfg->tilt_type > 0) {
    return true;
  }
  return false;
}
#endif /*_ROLLERSHUTTER_SUPPORT*/

supla_roller_shutter_cfg_t *GPIO_ICACHE_FLASH
supla_esp_gpio_get_rs_cfg(supla_relay_cfg_t *rel_cfg) {

  if (rel_cfg == NULL)
    return NULL;

  int a;

  for (a = 0; a < RS_MAX_COUNT; a++)
    if (supla_rs_cfg[a].up == rel_cfg || supla_rs_cfg[a].down == rel_cfg) {

      return &supla_rs_cfg[a];
    }

  return NULL;
}

supla_roller_shutter_cfg_t *GPIO_ICACHE_FLASH
supla_esp_gpio_get_rs__cfg(int port) {

  int a;

  if (port == 255)
    return NULL;

  for (a = 0; a < RELAY_MAX_COUNT; a++)
    if (supla_relay_cfg[a].gpio_id == port) {
      return supla_esp_gpio_get_rs_cfg(&supla_relay_cfg[a]);
    }

  return NULL;
}

void GPIO_ICACHE_FLASH supla_esp_gpio_rs_apply_new_config(
    int channel_number, TChannelConfig_RollerShutter *rsConfig) {
  if (channel_number < 0 || channel_number >= RS_MAX_COUNT) {
    return;
  }
  bool saveConfig = false;
  if (rsConfig->MotorUpsideDown > 0 && rsConfig->MotorUpsideDown < 3) {
    bool newMotorUpsideDown = (rsConfig->MotorUpsideDown == 2);
    bool oldMotorUpsideDown =
      (supla_esp_cfg.MotorUpsideDown & (1 << channel_number)) != 0;
    if (newMotorUpsideDown != oldMotorUpsideDown) {
      if (newMotorUpsideDown) {
        supla_esp_cfg.MotorUpsideDown |= (1 << channel_number);
      } else {
        supla_esp_cfg.MotorUpsideDown &= ~(1 << channel_number);
      }
      supla_relay_cfg_t *newUp = supla_rs_cfg[channel_number].down;
      supla_relay_cfg_t *newDown = supla_rs_cfg[channel_number].up;
      supla_rs_cfg[channel_number].down = newDown;
      supla_rs_cfg[channel_number].up = newUp;
      supla_log(LOG_DEBUG, "RS[%d] motor upside down %d", channel_number,
                newMotorUpsideDown);
      saveConfig = true;
    }
  }

  if (rsConfig->ButtonsUpsideDown > 0 && rsConfig->ButtonsUpsideDown < 3) {
    bool newButtonsUpsideDown = (rsConfig->ButtonsUpsideDown == 2);
    bool oldButtonsUpsideDown = (supla_esp_cfg.ButtonsUpsideDown) != 0;
    if (newButtonsUpsideDown != oldButtonsUpsideDown) {
      supla_esp_cfg.ButtonsUpsideDown = (newButtonsUpsideDown ? 1 : 0);
      int newUpButtonGpio = supla_input_cfg[(2 * channel_number) + 1].gpio_id;
      int newDownButtonGpio = supla_input_cfg[(2 * channel_number)].gpio_id;
      supla_input_cfg[(2 * channel_number)].gpio_id = newUpButtonGpio;
      supla_input_cfg[(2 * channel_number) + 1].gpio_id = newDownButtonGpio;
      supla_log(LOG_DEBUG, "RS[%d] buttons upside down %d", channel_number,
                newButtonsUpsideDown);
      saveConfig = true;
    }
  }

  int timeMargin = rsConfig->TimeMargin;
  if (timeMargin == -1 || (timeMargin >= 1 && timeMargin <= 101)) {
    if (timeMargin > 0) {
      timeMargin -= 1;
    }
    if (timeMargin != supla_esp_cfg.AdditionalTimeMargin) {
      supla_esp_cfg.AdditionalTimeMargin = timeMargin;
      supla_esp_gpio_rs_set_time_margin(supla_esp_cfg.AdditionalTimeMargin);
      saveConfig = true;
    }
  }

  supla_esp_gpio_rs_apply_new_times(channel_number,
                                    rsConfig->ClosingTimeMS,
                                    rsConfig->OpeningTimeMS, 0);

  if (saveConfig) {
    supla_esp_cfg_save(&supla_esp_cfg);
  }
}

void GPIO_ICACHE_FLASH supla_esp_gpio_fb_apply_new_config(
    int channel_number, TChannelConfig_FacadeBlind *fbConfig) {
  if (channel_number < 0 || channel_number >= RS_MAX_COUNT) {
    return;
  }
  supla_log(LOG_DEBUG, "FB[%d] new config: mud %d, bud %d, ot %d, ct %d, "
      "tt %d, tm %d, type %d", channel_number, fbConfig->MotorUpsideDown,
      fbConfig->ButtonsUpsideDown, fbConfig->OpeningTimeMS,
      fbConfig->ClosingTimeMS, fbConfig->TiltingTimeMS,
      fbConfig->TimeMargin, fbConfig->FacadeBlindType);

  bool saveConfig = false;
  if (fbConfig->MotorUpsideDown > 0 && fbConfig->MotorUpsideDown < 3) {
    bool newMotorUpsideDown = (fbConfig->MotorUpsideDown == 2);
    bool oldMotorUpsideDown =
      (supla_esp_cfg.MotorUpsideDown & (1 << channel_number)) != 0;
    if (newMotorUpsideDown != oldMotorUpsideDown) {
      if (newMotorUpsideDown) {
        supla_esp_cfg.MotorUpsideDown |= (1 << channel_number);
      } else {
        supla_esp_cfg.MotorUpsideDown &= ~(1 << channel_number);
      }
      supla_relay_cfg_t *newUp = supla_rs_cfg[channel_number].down;
      supla_relay_cfg_t *newDown = supla_rs_cfg[channel_number].up;
      supla_rs_cfg[channel_number].down = newDown;
      supla_rs_cfg[channel_number].up = newUp;
      supla_log(LOG_DEBUG, "FB[%d] motor upside down %d", channel_number,
          newMotorUpsideDown);
      saveConfig = true;
    }
  }

  if (fbConfig->ButtonsUpsideDown > 0 && fbConfig->ButtonsUpsideDown < 3) {
    bool newButtonsUpsideDown = (fbConfig->ButtonsUpsideDown == 2);
    bool oldButtonsUpsideDown = (supla_esp_cfg.ButtonsUpsideDown) != 0;
    if (newButtonsUpsideDown != oldButtonsUpsideDown) {
      supla_esp_cfg.ButtonsUpsideDown = (newButtonsUpsideDown ? 1 : 0);
      int newUpButtonGpio = supla_input_cfg[(2 * channel_number) + 1].gpio_id;
      int newDownButtonGpio = supla_input_cfg[(2 * channel_number)].gpio_id;
      supla_input_cfg[(2 * channel_number)].gpio_id = newUpButtonGpio;
      supla_input_cfg[(2 * channel_number) + 1].gpio_id = newDownButtonGpio;
      supla_log(LOG_DEBUG, "FB[%d] buttons upside down %d", channel_number,
          newButtonsUpsideDown);
      saveConfig = true;
    }
  }

  int timeMargin = fbConfig->TimeMargin;
  if (timeMargin == -1 || (timeMargin >= 1 && timeMargin <= 101)) {
    if (timeMargin > 0) {
      timeMargin -= 1;
    }
    if (timeMargin != supla_esp_cfg.AdditionalTimeMargin) {
      supla_esp_cfg.AdditionalTimeMargin = timeMargin;
      supla_esp_gpio_rs_set_time_margin(supla_esp_cfg.AdditionalTimeMargin);
      saveConfig = true;
    }
  }

  if (fbConfig->FacadeBlindType !=
      supla_esp_cfg.FacadeBlindType[channel_number]) {
    supla_esp_cfg.FacadeBlindType[channel_number] = fbConfig->FacadeBlindType;
    supla_log(LOG_DEBUG, "FB[%d] blind type %d", channel_number,
              fbConfig->FacadeBlindType);
    saveConfig = true;
  }

  supla_esp_gpio_rs_apply_new_times(
      channel_number, fbConfig->ClosingTimeMS,
      fbConfig->OpeningTimeMS, fbConfig->TiltingTimeMS);

  if (saveConfig) {
    supla_log(LOG_DEBUG, "SAVE CFG");
    supla_esp_cfg_save(&supla_esp_cfg);
  }
}
