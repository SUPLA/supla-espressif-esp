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

#include "supla_esp_input.h"
#include "supla_esp_gpio.h"
#include "supla_esp_cfgmode.h"
#include "supla_esp_cfg.h"
#include "supla_esp_mqtt.h"
#include "supla_esp_devconn.h"
#include "supla-dev/log.h"

#include <osapi.h>
#include <user_interface.h>

///////////////////////////////////////////////////////////////////////////////
// Variable declarations
///////////////////////////////////////////////////////////////////////////////

supla_input_cfg_t supla_input_cfg[INPUT_MAX_COUNT];

///////////////////////////////////////////////////////////////////////////////
// Forward declarations of local .c methods
///////////////////////////////////////////////////////////////////////////////

LOCAL void supla_esp_input_debounce_timer_cb(void *timer_arg);
LOCAL void supla_esp_input_timer_cb(void *timer_arg);
void GPIO_ICACHE_FLASH supla_esp_input_legacy_state_change_handling(
    supla_input_cfg_t *input_cfg, int new_state);
void GPIO_ICACHE_FLASH supla_esp_input_legacy_timer_handling(
    supla_input_cfg_t *input_cfg, int state);
void GPIO_ICACHE_FLASH supla_esp_input_start_cfg_mode(void);
uint32 GPIO_ICACHE_FLASH
supla_esp_input_get_cfg_press_time(supla_input_cfg_t *input_cfg);

///////////////////////////////////////////////////////////////////////////////
// Method definitions
///////////////////////////////////////////////////////////////////////////////

// Starts debounce timer for standard input handling
// This method is not used for INPUT_TYPE_CUSTOM
void supla_esp_input_start_debounce_timer(supla_input_cfg_t *input_cfg) {
  if ( input_cfg->type == INPUT_TYPE_CUSTOM) {
    return;
  }
  if (input_cfg->debounce_step == 0) {
    input_cfg->debounce_step = 1;

    os_timer_disarm(&input_cfg->debounce_timer);
    os_timer_setfn(&input_cfg->debounce_timer, 
        supla_esp_input_debounce_timer_cb, input_cfg);
    os_timer_arm(&input_cfg->debounce_timer, INPUT_CYCLE_TIME, true);
  }
}

// Debounce timer for standard input handlign
// This method is not used for INPUT_TYPE_CUSTOM
LOCAL void supla_esp_input_debounce_timer_cb(void *timer_arg) {
	supla_input_cfg_t *input_cfg = (supla_input_cfg_t *)timer_arg;

	uint8 current_value = gpio__input_get(input_cfg->gpio_id);
	uint8 active = (input_cfg->flags & INPUT_FLAG_PULLUP) ? 0 : 1;

  if (input_cfg->debounce_step == 1 ||
      input_cfg->debounce_value != current_value) {
    input_cfg->debounce_value = current_value;
    input_cfg->debounce_step = 2;
    return;
  }

  if (input_cfg->debounce_step > INPUT_MIN_CYCLE_COUNT) {
    if (current_value == active) {
      supla_esp_input_notify_state_change(input_cfg, INPUT_STATE_ACTIVE);
    } else {
      supla_esp_input_notify_state_change(input_cfg, INPUT_STATE_INACTIVE);
    }

    os_timer_disarm(&input_cfg->debounce_timer);
    input_cfg->debounce_step = 0;
  } else {
    input_cfg->debounce_step++;
  }
}

// This method should be called manually if INPUT_TYPE_CUSTOM is used to inform
// about input state change.
void GPIO_ICACHE_FLASH supla_esp_input_notify_state_change(
    supla_input_cfg_t *input_cfg, int new_state) {
  if (input_cfg->last_state == new_state) {
    return;
  }

  os_timer_disarm(&input_cfg->timer);
  os_timer_setfn(&input_cfg->timer, 
      supla_esp_input_timer_cb, input_cfg);

  input_cfg->last_state = new_state;

  supla_esp_input_legacy_state_change_handling(input_cfg, new_state);
}

bool GPIO_ICACHE_FLASH
supla_esp_input_is_cfg_on_hold_enabled(supla_input_cfg_t *input_cfg) {
  if (!input_cfg) {
    return false;
  }
  if (!(input_cfg->flags & INPUT_FLAG_CFG_BTN)) {
    return false;
  }
  if (input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE || 
      input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE_RS) {
    if (!(input_cfg->flags & INPUT_FLAG_CFG_ON_TOGGLE)) {
      return true;
    }
  }
  return false;
}

// Handling of inputs in standard (pre ActionTrigger) way
// Called only on state change
void GPIO_ICACHE_FLASH supla_esp_input_legacy_state_change_handling(
    supla_input_cfg_t *input_cfg, int new_state) {

  os_timer_disarm(&input_cfg->timer);

  if (new_state == INPUT_STATE_ACTIVE) {
    // Handling of input state change to ACTIVE
    if (input_cfg->flags & INPUT_FLAG_CFG_BTN) {
      // Handling of CFG BTN functionality
      if ( supla_esp_cfgmode_started() == 0 ) {
        if (supla_esp_input_is_cfg_on_hold_enabled(input_cfg) || 
            (system_get_time() - input_cfg->last_active >= 2000000)) {
          input_cfg->cfg_counter = 1;
        } else {
          input_cfg->cfg_counter++;
        }

        if (!supla_esp_input_is_cfg_on_hold_enabled(input_cfg) &&
            input_cfg->cfg_counter >= CFG_BTN_PRESS_COUNT) {
          input_cfg->cfg_counter = 0;
          // CFG MODE
          supla_esp_input_start_cfg_mode();
          return;
        }
      } else {
        input_cfg->cfg_counter = 1;
        if (!supla_esp_input_is_cfg_on_hold_enabled(input_cfg) &&
            system_get_time() - supla_esp_cfgmode_entertime() > 3000000) {
          // If we are in CFG mode and there was button press 3s after entering 
          // CFG mode, then EXIT CFG MODE
          supla_system_restart();
          return;
        }
      }
    }

    if (supla_esp_restart_on_cfg_press == 1 &&
        input_cfg->flags & INPUT_FLAG_CFG_BTN &&
        (input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE ||
         input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE_RS)) {
      supla_system_restart();
      return;
    }

   
    if (supla_esp_input_is_cfg_on_hold_enabled(input_cfg)) {
      os_timer_arm(&input_cfg->timer, INPUT_CYCLE_TIME, true);
    }

    input_cfg->last_active = system_get_time();
    if ( supla_esp_cfgmode_started() == 0) {
      supla_esp_gpio_on_input_active(input_cfg);
    }
  } else if (new_state == INPUT_STATE_INACTIVE) {
    if (input_cfg->flags & INPUT_FLAG_CFG_BTN) {
      // Handling of CFG BTN functionality
      if ( input_cfg->cfg_counter > 0 && supla_esp_cfgmode_started() == 1 && 
          (system_get_time() - supla_esp_cfgmode_entertime() > 3000000)) {
        // If we are in CFG mode and there was button press 3s after entering 
        // CFG mode, then EXIT CFG MODE
        supla_system_restart();
        return;
      }
    }
    if ( supla_esp_cfgmode_started() == 0) {
      supla_esp_gpio_on_input_inactive(input_cfg);
    }
  }

}

void GPIO_ICACHE_FLASH
supla_esp_input_legacy_timer_handling(supla_input_cfg_t *input_cfg, int state) {
  if (state == INPUT_STATE_ACTIVE) {
    if (supla_esp_input_is_cfg_on_hold_enabled(input_cfg)) {
      // if input is used for config and it is monostable, then cfg_counter is
      // used to calculate how long button is active
      input_cfg->cfg_counter++;

      if (input_cfg->cfg_counter * INPUT_CYCLE_TIME >=
          GET_CFG_PRESS_TIME(input_cfg)) {

        os_timer_disarm(&input_cfg->timer);
        input_cfg->cfg_counter = 0;

        // CFG MODE
        if (supla_esp_cfgmode_started() == 0) {
          supla_esp_input_start_cfg_mode();
        } else if (input_cfg->flags & INPUT_FLAG_FACTORY_RESET) {
          factory_defaults(1);
          supla_log(LOG_DEBUG, "Factory defaults");
          os_delay_us(500000);
          supla_system_restart();
        }
      }
    }
  }
}

// Timer used to determine how long button is active or inactive
LOCAL void supla_esp_input_timer_cb(void *timer_arg) {
  supla_input_cfg_t *input_cfg = (supla_input_cfg_t *)timer_arg;

  supla_esp_input_legacy_timer_handling(input_cfg, input_cfg->last_state);
}

void GPIO_ICACHE_FLASH supla_esp_input_start_cfg_mode(void) {
  if (supla_esp_cfgmode_started() == 0) {
#ifdef BEFORE_CFG_ENTER
    BEFORE_CFG_ENTER
#endif
#ifdef MQTT_SUPPORT_ENABLED
    supla_esp_mqtt_client_stop();
#endif /*MQTT_SUPPORT_ENABLED*/
    supla_esp_devconn_stop();
    supla_esp_cfgmode_start();
  }
}

uint32 GPIO_ICACHE_FLASH
supla_esp_input_get_cfg_press_time(supla_input_cfg_t *input_cfg) {
	return CFG_BTN_PRESS_TIME;
}

