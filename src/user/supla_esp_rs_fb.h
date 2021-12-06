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

#ifndef _SUPLA_ESP_RS_FB_H
#define _SUPLA_ESP_RS_FB_H

#include "supla_esp.h"
#include "supla_esp_gpio.h"

#define RS_RELAY_OFF 0
#define RS_RELAY_UP 2
#define RS_RELAY_DOWN 1

#define RS_TASK_INACTIVE 0
#define RS_TASK_ACTIVE 1
#define RS_TASK_SETTING_POSITION 2
#define RS_TASK_SETTING_TILT 3

typedef struct {
  uint8 position;
  uint8 tilt;
  uint8 direction;
  uint8 state;  // RS_TASK_*
} rs_task_t;

typedef struct {

  ETSTimer timer;
  uint8 value;
  bool autoCal_request;

} supla_rs_delayed_trigger;

#define FB_TILT_TYPE_KEEP_POSITION_WHILE_TILTING 0
#define FB_TILT_TYPE_CHANGE_POSITION_WHILE_TILTING 1
#define FB_TILT_TYPE_TILTING_ONLY_AT_FULLY_CLOSED 2

typedef struct {

  supla_relay_cfg_t *up;
  supla_relay_cfg_t *down;

  int *position;
  int *tilt;  // tilt position use the same format as "position", so
              // it is in 10-110 range, multiplied by 100
              // tilt 10 * 100 corresponds with fully open
              // tilt 110 * 100 corresponds with fully closed
  signed char tilt_type;  // FB_TILT_TYPE_*

  unsigned int *full_opening_time;
  unsigned int *full_closing_time;
  unsigned int *auto_opening_time;
  unsigned int *auto_closing_time;
  unsigned int *tilt_change_time;

  unsigned int last_comm_time;

  unsigned int up_time;
  unsigned int down_time;
  unsigned int last_time;
  unsigned int stop_time;
  unsigned int start_time;

  unsigned int autoCal_step;

  ETSTimer timer;
  supla_rs_delayed_trigger delayed_trigger;
  rs_task_t task;

  int last_position;
  int last_tilt;
  bool performAutoCalibration;
  bool autoCal_button_request;

  unsigned _supla_int16_t flags;       // RS_VALUE_FLAG_*
  unsigned _supla_int16_t last_flags;  // last send RS_VALUE_FLAG_*
  bool detectedPowerConsumption;
} supla_roller_shutter_cfg_t;

extern supla_roller_shutter_cfg_t supla_rs_cfg[RS_MAX_COUNT];

#ifdef _ROLLERSHUTTER_SUPPORT
void GPIO_ICACHE_FLASH supla_esp_gpio_rs_set_time_margin(uint8 value);

void GPIO_ICACHE_FLASH supla_esp_gpio_rs_apply_new_times(int idx, int ct_ms,
    int ot_ms);
bool GPIO_ICACHE_FLASH supla_esp_gpio_rs_apply_new__times(int idx, int ct_ms,
    int ot_ms, bool save);
void supla_esp_gpio_rs_set_relay(supla_roller_shutter_cfg_t *rs_cfg,
    uint8 value, uint8 cancel_task,
    uint8 stop_delay);
uint8 GPIO_ICACHE_FLASH
supla_esp_gpio_rs_get_value(supla_roller_shutter_cfg_t *rs_cfg);
void GPIO_ICACHE_FLASH
supla_esp_gpio_rs_cancel_task(supla_roller_shutter_cfg_t *rs_cfg);
void GPIO_ICACHE_FLASH supla_esp_gpio_rs_add_task(int idx, sint8 position,
    sint8 tilt);

sint8 supla_esp_gpio_rs_get_current_position(
    supla_roller_shutter_cfg_t *rs_cfg);
sint8 GPIO_ICACHE_FLASH
supla_esp_gpio_rs_get_current_tilt(supla_roller_shutter_cfg_t *rs_cfg);
void supla_esp_gpio_rs_start_autoCal(supla_roller_shutter_cfg_t *rs_cfg);
bool GPIO_ICACHE_FLASH
supla_esp_gpio_rs_is_tilt_supported(supla_roller_shutter_cfg_t *rs_cfg);
#endif /*_ROLLERSHUTTER_SUPPORT*/

supla_roller_shutter_cfg_t *supla_esp_gpio_get_rs__cfg(int port);
supla_roller_shutter_cfg_t *GPIO_ICACHE_FLASH
supla_esp_gpio_get_rs_cfg(supla_relay_cfg_t *rel_cfg);
void GPIO_ICACHE_FLASH supla_esp_gpio_rs_timer_cb(void *timer_arg);

#endif /*_SUPLA_ESP_RS_FB_H*/
