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
static int btn_hold_time_ms = BTN_HOLD_TIME_MS;
static int btn_multiclick_time_ms = BTN_MULTICLICK_TIME_MS;

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

  // Silent period disables inputs for first 400 ms after startup in order to
  // init current input state.
  static bool silent_period = true;
  if (silent_period) {
    if (system_get_time() - supla_esp_gpio_init_time < 400000) {
      input_cfg->last_state = new_state;
      return;
    }  else {
      silent_period = false;
    }
  }
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
  if (input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE) {
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
        input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE) {
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

      if (system_get_time() - input_cfg->last_state_change >=
          GET_CFG_PRESS_TIME(input_cfg)*1000) {

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

void GPIO_ICACHE_FLASH supla_esp_input_set_active_triggers(
    supla_input_cfg_t *input_cfg, unsigned _supla_int_t active_triggers) {
  if (input_cfg) {
    unsigned _supla_int_t prev_triggers = input_cfg->active_triggers;
    input_cfg->active_triggers =
      input_cfg->action_trigger_cap & active_triggers;

    input_cfg->max_clicks = 0;

    if ((input_cfg->flags & INPUT_FLAG_CFG_BTN) &&
        !supla_esp_input_is_cfg_on_hold_enabled(input_cfg)) {
      input_cfg->max_clicks = CFG_BTN_PRESS_COUNT;
    }

    int max_clicks_from_actions = 0;
    if ((input_cfg->active_triggers & SUPLA_ACTION_CAP_SHORT_PRESS_x5) ||
        (input_cfg->active_triggers & SUPLA_ACTION_CAP_TOGGLE_x5) ) {
      max_clicks_from_actions = 5;
    } else if ((input_cfg->active_triggers & SUPLA_ACTION_CAP_SHORT_PRESS_x4) ||
        (input_cfg->active_triggers & SUPLA_ACTION_CAP_TOGGLE_x4) ) {
      max_clicks_from_actions = 4;
    } else if ((input_cfg->active_triggers & SUPLA_ACTION_CAP_SHORT_PRESS_x3) ||
        (input_cfg->active_triggers & SUPLA_ACTION_CAP_TOGGLE_x3) ) {
      max_clicks_from_actions = 3;
    } else if ((input_cfg->active_triggers & SUPLA_ACTION_CAP_SHORT_PRESS_x2) ||
        (input_cfg->active_triggers & SUPLA_ACTION_CAP_TOGGLE_x2) ) {
      max_clicks_from_actions = 2;
    } else if ((input_cfg->active_triggers & SUPLA_ACTION_CAP_SHORT_PRESS_x1) ||
        (input_cfg->active_triggers & SUPLA_ACTION_CAP_TOGGLE_x1) ) {
      max_clicks_from_actions = 1;
    }
        
    if (input_cfg->max_clicks < max_clicks_from_actions) {
      input_cfg->max_clicks = max_clicks_from_actions;
    }

    if (prev_triggers != input_cfg->active_triggers) {
        os_timer_disarm(&input_cfg->timer);
        input_cfg->click_counter = 0;
    }
  }
}

bool GPIO_ICACHE_FLASH
supla_esp_input_is_advanced_mode_enabled(supla_input_cfg_t *input_cfg) {
  if ( supla_esp_cfgmode_started() == 0 && input_cfg) {
    // if input is used for roller shutter, we have to check if input for
    // another direction is in advanced mode. If yes, then we switch both
    // inputs to advanced mode in order to get the same default behabior
    // for both inputs
    supla_roller_shutter_cfg_t *rs_cfg =
      supla_esp_gpio_get_rs__cfg(input_cfg->relay_gpio_id);
    if (rs_cfg) {
      int another_gpio_id = (input_cfg->relay_gpio_id == rs_cfg->up->gpio_id) ?
        rs_cfg->down->gpio_id : rs_cfg->up->gpio_id;
      int i = 0;
      for (; i < INPUT_MAX_COUNT; i++) {
        if (supla_input_cfg[i].relay_gpio_id == another_gpio_id) {
          break;
        }
      }
      if (i < INPUT_MAX_COUNT) {
        return input_cfg->active_triggers != 0 ||
          supla_input_cfg[i].active_triggers != 0;
      }
    }

    return input_cfg->active_triggers != 0;
  }
  return false;
}

// Handling of inputs in advanced way (ActionTrigger with multiclick)
// Called only on state change
// Called only when we are not in cfgmode
void GPIO_ICACHE_FLASH supla_esp_input_advanced_state_change_handling(
    supla_input_cfg_t *input_cfg, int new_state) {

  os_timer_disarm(&input_cfg->timer);

  if (input_cfg->click_counter != -1) {
    // Monostable buttons counts "click" when state change to "active"
    // Bistable buttons counts "click" on each state change
    if (new_state == INPUT_STATE_ACTIVE ||
        (input_cfg->type & INPUT_TYPE_BTN_BISTABLE)) {

      input_cfg->click_counter++;

      if (input_cfg->type == INPUT_TYPE_BTN_BISTABLE) {
        if (new_state == INPUT_STATE_ACTIVE) {
          supla_esp_input_send_action_trigger(input_cfg, 
              SUPLA_ACTION_CAP_TURN_ON);
        } else {
          supla_esp_input_send_action_trigger(input_cfg, 
              SUPLA_ACTION_CAP_TURN_OFF);
        }
      }

      if (input_cfg->flags & INPUT_FLAG_CFG_BTN &&
          !supla_esp_input_is_cfg_on_hold_enabled(input_cfg)) {
        // Handling of CFG BTN functionality
        if (input_cfg->click_counter >= CFG_BTN_PRESS_COUNT) {
          input_cfg->click_counter = 0;
          // CFG MODE
          supla_esp_input_start_cfg_mode();
        }
      } 
    }
  }

  input_cfg->last_state_change = system_get_time();
  os_timer_arm(&input_cfg->timer, INPUT_CYCLE_TIME, true);
}

// Timer used to determine how long button is active or inactive in advanced
// mode
void GPIO_ICACHE_FLASH supla_esp_input_advanced_timer_cb(void *timer_arg) {
  supla_input_cfg_t *input_cfg = (supla_input_cfg_t *)timer_arg;

  unsigned int delta_time = system_get_time() - input_cfg->last_state_change;

  // MONOSTABLE buttons
  if (input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE) {
    if (input_cfg->last_state == INPUT_STATE_ACTIVE &&
        input_cfg->click_counter != -1) {
      // state ACTIVE
      if (supla_esp_input_is_cfg_on_hold_enabled(input_cfg)) {
        if (delta_time >= GET_CFG_PRESS_TIME(input_cfg) * 1000) {
          os_timer_disarm(&input_cfg->timer);
          input_cfg->click_counter = 0;
          supla_esp_input_start_cfg_mode();
        }
      }
      if (input_cfg->click_counter == 1 && 
          (input_cfg->active_triggers & SUPLA_ACTION_CAP_HOLD) &&
          delta_time >= btn_hold_time_ms * 1000) {
        supla_esp_input_send_action_trigger(input_cfg, SUPLA_ACTION_CAP_HOLD);
        input_cfg->click_counter = 0;
        if (!supla_esp_input_is_cfg_on_hold_enabled(input_cfg)) {
          os_timer_disarm(&input_cfg->timer);
        }
      }
    } 
  }

  // BISTABLE buttons and inactive state for monostable
  if (input_cfg->last_state == INPUT_STATE_INACTIVE || 
      input_cfg->type == INPUT_TYPE_BTN_BISTABLE) {
    // if below condition is true, it means that button was
    // released for more time then multiclick detection.
    // As a result we send detected click_counter as an action trigger
    // and we reset the counter so it will be ready for next detection.
      if (delta_time >= btn_multiclick_time_ms * 1000) {
        os_timer_disarm(&input_cfg->timer);
        supla_esp_input_send_action_trigger(input_cfg, 0);
        input_cfg->click_counter = 0;
    // If below condition is met, then user clicked more times than max
    // active and detectable click count. It will send detected action
    // (max configured click count).
      } else if (input_cfg->click_counter >= input_cfg->max_clicks) {
        supla_esp_input_send_action_trigger(input_cfg, 0);
        input_cfg->click_counter = -1;
    // Special handling of situation where max configured click count is
    // 0 or 1. In such case we don't have to wait btn_multiclick_time_ms 
    // for next button detection
        if (input_cfg->max_clicks <= 1) {
          os_timer_disarm(&input_cfg->timer);
          input_cfg->click_counter = 0;
        }
      }
  }
}

// Send action triger notification to server
// If "action" parameter is != 0, then it sends it, otherwise it sends 
// TOGGLE_x or PRESS_x depending on current click_counter
void GPIO_ICACHE_FLASH
supla_esp_input_send_action_trigger(supla_input_cfg_t *input_cfg, int action) {
  if (action == 0) {
    if (input_cfg->click_counter == -1) {
      return;
    }
    // if there is related relay gpio id, then single click/press is used for
    // device's local action
    if (input_cfg->click_counter == 1 && input_cfg->relay_gpio_id != 255) {
      supla_esp_gpio_on_input_active(input_cfg);
      return;
    }
    // map click_counter to proper action
    if (input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE) {
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
    } else if (input_cfg->type == INPUT_TYPE_BTN_BISTABLE) {
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

  if (!(action & input_cfg->active_triggers)) {
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
    int i = 1;
    for (;i < 32; i++) {
      if (action >> i == 0) break;
    }
    supla_log(LOG_DEBUG, "Input[ch: %d] sending action trigger (1 << %d)",
        input_cfg->channel, i-1);

    supla_esp_devconn_send_action_trigger(input_cfg->channel, action);

#ifdef MQTT_SUPPORT_ENABLED
#ifdef MQTT_HA_ACTION_TRIGGER_SUPPORT
    supla_esp_mqtt_send_action_trigger(input_cfg->channel, action);
#endif /*MQTT_HA_ACTION_TRIGGER_SUPPORT*/
#endif /*MQTT_SUPPORT_ENABLED*/
  }
}

void GPIO_ICACHE_FLASH supla_esp_input_set_hold_time_ms(int time_ms) {
  btn_hold_time_ms = time_ms;
  if (btn_hold_time_ms <= 0) {
    btn_hold_time_ms = BTN_HOLD_TIME_MS;
  }
}

void GPIO_ICACHE_FLASH supla_esp_input_set_multiclick_time_ms(int time_ms) {
  btn_multiclick_time_ms = time_ms;
  if (btn_multiclick_time_ms <= 0) {
    btn_multiclick_time_ms = BTN_MULTICLICK_TIME_MS;
  }
}

