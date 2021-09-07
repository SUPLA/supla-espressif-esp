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
void GPIO_ICACHE_FLASH supla_esp_input_legacy_timer_cb(void *timer_arg);
void GPIO_ICACHE_FLASH supla_esp_input_advanced_timer_cb(void *timer_arg);
void GPIO_ICACHE_FLASH supla_esp_input_legacy_state_change_handling(
    supla_input_cfg_t *input_cfg, int new_state);
void GPIO_ICACHE_FLASH supla_esp_input_start_cfg_mode(void);
uint32 GPIO_ICACHE_FLASH
supla_esp_input_get_cfg_press_time(supla_input_cfg_t *input_cfg);
void GPIO_ICACHE_FLASH
supla_esp_input_send_action_trigger(supla_input_cfg_t *input_cfg, int action);
void GPIO_ICACHE_FLASH supla_esp_input_advanced_state_change_handling(
    supla_input_cfg_t *input_cfg, int new_state);

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

  input_cfg->last_state = new_state;

  if (supla_esp_input_is_advanced_mode_enabled(input_cfg)) {
    os_timer_setfn(&input_cfg->timer, supla_esp_input_advanced_timer_cb,
        input_cfg);
    supla_esp_input_advanced_state_change_handling(input_cfg, new_state);
  } else {
    os_timer_setfn(&input_cfg->timer, supla_esp_input_legacy_timer_cb,
        input_cfg);
    supla_esp_input_legacy_state_change_handling(input_cfg, new_state);
  }
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
            (system_get_time() - input_cfg->last_state_change >= 2000*1000)) {
          input_cfg->click_counter = 1;
        } else {
          input_cfg->click_counter++;
        }

        if (!supla_esp_input_is_cfg_on_hold_enabled(input_cfg) &&
            input_cfg->click_counter >= CFG_BTN_PRESS_COUNT) {
          input_cfg->click_counter = 0;
          // CFG MODE
          supla_esp_input_start_cfg_mode();
          return;
        }
      } else {
        input_cfg->click_counter = 1;
        if (!supla_esp_input_is_cfg_on_hold_enabled(input_cfg) &&
            system_get_time() - supla_esp_cfgmode_entertime() > 3000*1000) {
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

    input_cfg->last_state_change = system_get_time();
    if ( supla_esp_cfgmode_started() == 0) {
      supla_esp_gpio_on_input_active(input_cfg);
    }
  } else if (new_state == INPUT_STATE_INACTIVE) {
    if (input_cfg->flags & INPUT_FLAG_CFG_BTN) {
      // Handling of CFG BTN functionality
      if ( input_cfg->click_counter > 0 && supla_esp_cfgmode_started() == 1 && 
          (system_get_time() - supla_esp_cfgmode_entertime() > 3000*1000)) {
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

// Timer used to determine how long button is active or inactive
void GPIO_ICACHE_FLASH supla_esp_input_legacy_timer_cb(void *timer_arg) {
  supla_input_cfg_t *input_cfg = (supla_input_cfg_t *)timer_arg;

  if (input_cfg->last_state == INPUT_STATE_ACTIVE) {
    if (supla_esp_input_is_cfg_on_hold_enabled(input_cfg)) {
      // if input is used for config and it is monostable, then click_counter is
      // used to calculate how long button is active
      input_cfg->click_counter++;

      if (input_cfg->click_counter * INPUT_CYCLE_TIME >=
          GET_CFG_PRESS_TIME(input_cfg)) {

        os_timer_disarm(&input_cfg->timer);
        input_cfg->click_counter = 0;

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

void GPIO_ICACHE_FLASH supla_esp_input_set_active_actions(
    supla_input_cfg_t *input_cfg, unsigned _supla_int_t active_actions) {
  if (input_cfg) {
    input_cfg->active_actions = input_cfg->supported_actions & active_actions;
  }
}

bool GPIO_ICACHE_FLASH
supla_esp_input_is_advanced_mode_enabled(supla_input_cfg_t *input_cfg) {
  // TODO: return false when we are in cfgmode
  if (input_cfg) {
    return input_cfg->active_actions != 0;
  }
  return false;
}

// Handling of inputs in advanced way (ActionTrigger with multiclick)
// Called only on state change
// Called only when we are not in cfgmode
void GPIO_ICACHE_FLASH supla_esp_input_advanced_state_change_handling(
    supla_input_cfg_t *input_cfg, int new_state) {

  os_timer_disarm(&input_cfg->timer);

  // Monostable buttons counts "click" when state change to "active"
  // Bistable buttons counts "click" on each state change
  if (new_state == INPUT_STATE_ACTIVE || (
        (input_cfg->type & INPUT_TYPE_BTN_BISTABLE) ||
        (input_cfg->type & INPUT_TYPE_BTN_BISTABLE_RS))) {

    input_cfg->click_counter++;
    if (input_cfg->flags & INPUT_FLAG_CFG_BTN &&
        !supla_esp_input_is_cfg_on_hold_enabled(input_cfg)) {
      // Handling of CFG BTN functionality
      if (input_cfg->click_counter >= CFG_BTN_PRESS_COUNT) {
        input_cfg->click_counter = 0;
        // CFG MODE
        supla_esp_input_start_cfg_mode();
      }
    } else if (input_cfg->click_counter >= 5) { // input_cfg->max_possible_clicks) {
      supla_esp_input_send_action_trigger(input_cfg, 0);
      input_cfg->click_counter = 0;
      input_cfg->last_state_change = system_get_time();
      return; // don't arm timer
    }
  } 

  input_cfg->last_state_change = system_get_time();
  os_timer_arm(&input_cfg->timer, INPUT_CYCLE_TIME, true);
}

// Send action triger notification to server
// If "action" parameter is != 0, then it sends it, otherwise it sends 
// TOGGLE_x or PRESS_x depending on current click_counter
void GPIO_ICACHE_FLASH
supla_esp_input_send_action_trigger(supla_input_cfg_t *input_cfg, int action) {
  if (action == 0) {
    // if there is related relay gpio id, then single click/press is used for
    // device's local action
    if (input_cfg->click_counter == 1 && input_cfg->relay_gpio_id != 255) {
      supla_esp_gpio_on_input_active(input_cfg);
      return;
    }
    // map click_counter to proper action
    if (input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE || 
        input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE_RS) {
     switch (input_cfg->click_counter) {
       case 1:
         action = SUPLA_ACTION_CAP_SHORT_PRESS_x1;
         break;
       case 2:
         action = SUPLA_ACTION_CAP_SHORT_PRESS_x2;
         break;
       case 3:
         action = SUPLA_ACTION_CAP_SHORT_PRESS_x3;
         break;
       case 4:
         action = SUPLA_ACTION_CAP_SHORT_PRESS_x4;
         break;
       case 5:
         action = SUPLA_ACTION_CAP_SHORT_PRESS_x5;
         break;
     };
    } else if (input_cfg->type == INPUT_TYPE_BTN_BISTABLE ||
        input_cfg->type == INPUT_TYPE_BTN_BISTABLE_RS) {
     switch (input_cfg->click_counter) {
       case 1:
         action = SUPLA_ACTION_CAP_TOGGLE_x1;
         break;
       case 2:
         action = SUPLA_ACTION_CAP_TOGGLE_x2;
         break;
       case 3:
         action = SUPLA_ACTION_CAP_TOGGLE_x3;
         break;
       case 4:
         action = SUPLA_ACTION_CAP_TOGGLE_x4;
         break;
       case 5:
         action = SUPLA_ACTION_CAP_TOGGLE_x5;
         break;
     };
    }

  }

  if (!(action & input_cfg->active_actions)) {
    supla_log(LOG_DEBUG, "Input: action %d is not activated", action);
    return;
  }

  if (action == 0) {
    supla_log(LOG_DEBUG,
        "Input: missing action trigger mapping for click_counter %d",
        input_cfg->click_counter);
    return;
  }

  if (input_cfg->channel != 255) {
    char value[SUPLA_CHANNELVALUE_SIZE] = {};
    memcpy(value, &action, sizeof(action));
    supla_esp_channel_value__changed(input_cfg->channel, value);
#ifdef MQTT_SUPPORT_ENABLED
    // TODO send to MQTT
    supla_esp_mqtt_send_action_trigger(channel, action);
#endif
  }
}

// Timer used to determine how long button is active or inactive in advanced
// mode
void GPIO_ICACHE_FLASH supla_esp_input_advanced_timer_cb(void *timer_arg) {
  supla_input_cfg_t *input_cfg = (supla_input_cfg_t *)timer_arg;

  unsigned int delta_time = system_get_time() - input_cfg->last_state_change;

  // MONOSTABLE buttons
  if (input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE ||
      input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE_RS) {
    if (input_cfg->last_state == INPUT_STATE_ACTIVE) {
      // state ACTIVE
      if (supla_esp_input_is_cfg_on_hold_enabled(input_cfg)) {
        if (delta_time >= GET_CFG_PRESS_TIME(input_cfg) * 1000) {
          os_timer_disarm(&input_cfg->timer);
          input_cfg->click_counter = 0;
          supla_esp_input_start_cfg_mode();
        }
      }
      if (input_cfg->click_counter == 1 && 
          (input_cfg->active_actions & SUPLA_ACTION_CAP_HOLD) &&
          delta_time >= BTN_HOLD_TIME_MS * 1000) {
        supla_esp_input_send_action_trigger(input_cfg, SUPLA_ACTION_CAP_HOLD);
        input_cfg->click_counter = 0;
        if (!supla_esp_input_is_cfg_on_hold_enabled(input_cfg)) {
          os_timer_disarm(&input_cfg->timer);
        }
      }
    } else if (input_cfg->last_state == INPUT_STATE_INACTIVE) {
      // state INACTIVE
      if (delta_time >= BTN_MULTICLICK_TIME_MS * 1000) {
        os_timer_disarm(&input_cfg->timer);
        supla_esp_input_send_action_trigger(input_cfg, 0);
        input_cfg->click_counter = 0;
      }
    }

  // BISTABLE buttons
  } else if (input_cfg->type == INPUT_TYPE_BTN_BISTABLE ||
      input_cfg->type == INPUT_TYPE_BTN_BISTABLE_RS) {
     // TODO
  }
}

