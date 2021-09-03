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

#ifndef SUPLA_ESP_INPUT_H_
#define SUPLA_ESP_INPUT_H_

#include "supla_esp.h"

#define INPUT_STATE_ACTIVE 1
#define INPUT_STATE_INACTIVE 0

#define CFG_BTN_PRESS_COUNT     10

typedef struct {
  uint8 gpio_id;  // input GPIO
  uint8 flags;    // configuration flags INPUT_FLAG_
  uint8 type;     // input type INPUT_TYPE_
  uint8 last_state;
  uint8 step;     // used internally to determine which action should 
                  // be triggered
  uint8 debounce_step;
  uint8 cycle_counter;  // used to determine how long input is active
                        // or inactive
  uint16 cfg_counter;   // -> click_counter
  uint8 relay_gpio_id;  // GPIO of relay which is controlled by this
                        // input. It can also controll RS
  uint8 channel;        // used for INPUT_TYPE_SENSOR and ACTION_TRIGGER
  uint8 debounce_value;

  ETSTimer debounce_timer;
  ETSTimer timer;
  unsigned int last_active; // timestamp of last active state
} supla_input_cfg_t;

extern supla_input_cfg_t supla_input_cfg[INPUT_MAX_COUNT];

// This method should be called if board use custom input method
// other than simple HIGH/LOW state directly on GPIO.
// It will make sure that all reactions associasted with buttons
// will be executed properly (i.e. on hold, on multiclick, enter 
// cfg mode, etc.)
void GPIO_ICACHE_FLASH supla_esp_input_notify_state_change(
    supla_input_cfg_t *input_cfg, int new_state);

void supla_esp_input_start_debounce_timer(supla_input_cfg_t *input_cfg);

#endif /*SUPLA_ESP_INPUT_H_*/
