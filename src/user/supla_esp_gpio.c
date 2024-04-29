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
#include <osapi.h>
#include <mem.h>
#include <os_type.h>
#include <gpio.h>
#include <user_interface.h>

#include "supla_esp.h"
#include "supla_esp_gpio.h"
#include "supla_esp_cfg.h"
#include "supla_esp_devconn.h"
#include "supla_esp_cfgmode.h"
#include "supla_esp_countdown_timer.h"
#include "supla_esp_mqtt.h"
#include "supla_update.h"
#include "supla_esp_rs_fb.h"
#include "supla_esp_input.h"

#include "supla-dev/log.h"

#define GPIO_OUTPUT_GET(gpio_no)     ((gpio_output_get()>>gpio_no)&BIT0)

#define LED_RED    0x1
#define LED_GREEN  0x2
#define LED_BLUE   0x4

supla_relay_cfg_t supla_relay_cfg[RELAY_MAX_COUNT];

unsigned int supla_esp_gpio_init_time = 0;
bool silent_period = true;

static char supla_last_state = STATE_UNKNOWN;
static ETSTimer supla_gpio_timer1;
static ETSTimer supla_gpio_timer2;
static ETSTimer supla_motion_sensor_init_timer;
unsigned char supla_esp_restart_on_cfg_press = 0;

void GPIO_ICACHE_FLASH supla_esp_gpio_set_motion_sensor_state(void *timer_arg);

void
gpio16_output_conf(void)
{
    WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
                   (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1); 	// mux configuration for XPD_DCDC to output rtc_gpio0

    WRITE_PERI_REG(RTC_GPIO_CONF,
                   (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);	//mux configuration for out enable

    WRITE_PERI_REG(RTC_GPIO_ENABLE,
                   (READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe) | (uint32)0x1);	//out enable
}

void
gpio16_output_set(uint8 value)
{
    WRITE_PERI_REG(RTC_GPIO_OUT,
                   (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (uint32)(value & 1));
}

void
gpio16_input_conf(void)
{
    WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
                   (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1); 	// mux configuration for XPD_DCDC and rtc_gpio0 connection

    WRITE_PERI_REG(RTC_GPIO_CONF,
                   (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);	//mux configuration for out enable

    WRITE_PERI_REG(RTC_GPIO_ENABLE,
                   READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe);	//out disable
}

uint8
gpio16_input_get(void)
{
    return (uint8)(READ_PERI_REG(RTC_GPIO_IN_DATA) & 1);
}

uint8
gpio16_output_get(void)
{
    return (uint8)(READ_PERI_REG(RTC_GPIO_OUT) & 1);
}

uint32
gpio_output_get(void)
{
    return GPIO_REG_READ(GPIO_OUT_ADDRESS);
}

uint8
gpio__input_get(uint8 port)
{
#ifdef BOARD_GPIO_INPUT_IS_HI
	BOARD_GPIO_INPUT_IS_HI
#endif

	if ( port == 16 )
      return gpio16_input_get();

	return GPIO_INPUT_GET(GPIO_ID_PIN(port));
}

void GPIO_ICACHE_FLASH
supla_esp_gpio_enable_input_port(char port) {

	if ( port < 1 || port > 15 ) return;

    gpio_output_set(0, 0, 0, BIT(port));

    gpio_register_set(GPIO_PIN_ADDR(port), GPIO_PIN_INT_TYPE_SET(GPIO_PIN_INTR_DISABLE)
                      | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_DISABLE)
                      | GPIO_PIN_SOURCE_SET(GPIO_AS_PIN_SOURCE));

    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(port));
    gpio_pin_intr_state_set(GPIO_ID_PIN(port), GPIO_PIN_INTR_ANYEDGE);

}

void supla_esp_gpio_btn_irq_lock(uint8 lock) {
  supla_input_cfg_t *input_cfg;

  ETS_GPIO_INTR_DISABLE();

  for (int a = 0; a < INPUT_MAX_COUNT; a++) {
    input_cfg = &supla_input_cfg[a];

    if (input_cfg->gpio_id != 255 && input_cfg->gpio_id < 16 &&
        !(input_cfg->flags & INPUT_FLAG_DISABLE_INTR) &&
        (input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE ||
         input_cfg->type == INPUT_TYPE_BTN_BISTABLE ||
         input_cfg->type == INPUT_TYPE_MOTION_SENSOR)) {
      gpio_pin_intr_state_set(GPIO_ID_PIN(input_cfg->gpio_id),
          lock == 1 ? GPIO_PIN_INTR_DISABLE
          : GPIO_PIN_INTR_ANYEDGE);

      if (lock == 1)
        GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(input_cfg->gpio_id));
    }
  }

  ETS_GPIO_INTR_ENABLE();
}

char supla_esp_gpio_relay_hi(int port, unsigned char hi) {
  unsigned int t = system_get_time();
  int a;
  char result = 0;
  char *state = NULL;
  supla_roller_shutter_cfg_t *rs_cfg = NULL;
  char _hi;

  if (hi == 255) {
    hi = supla_esp_gpio_relay_is_hi(port) == 1 ? 0 : 1;
  }

  _hi = hi;

  for (a = 0; a < RELAY_MAX_COUNT; a++) {
    if (supla_relay_cfg[a].gpio_id == port) {

      if (supla_relay_cfg[a].flags & RELAY_FLAG_RESTORE ||
          supla_relay_cfg[a].flags & RELAY_FLAG_RESTORE_FORCE)
        state = &supla_esp_state.Relay[a];

      if (supla_relay_cfg[a].flags & RELAY_FLAG_LO_LEVEL_TRIGGER)
        _hi = hi == HI_VALUE ? LO_VALUE : HI_VALUE;

      rs_cfg = supla_esp_gpio_get_rs_cfg(&supla_relay_cfg[a]);
      break;
    }
  }

  // supla_log(LOG_DEBUG, "port=%i, hi=%i",port, hi);

  system_soft_wdt_stop();

  supla_esp_gpio_btn_irq_lock(1);
  os_delay_us(10);

#ifdef RELAY_BEFORE_CHANGE_STATE
  RELAY_BEFORE_CHANGE_STATE;
#endif

  // supla_log(LOG_DEBUG, "1. port = %i, hi = %i", port, _hi);

  ETS_GPIO_INTR_DISABLE();
  supla_esp_gpio_set_hi(port, _hi);
  ETS_GPIO_INTR_ENABLE();

#ifdef RELAY_DOUBLE_TRY
#if RELAY_DOUBLE_TRY > 0
  os_delay_us(RELAY_DOUBLE_TRY);

  ETS_GPIO_INTR_DISABLE();
  supla_esp_gpio_set_hi(port, _hi);
  ETS_GPIO_INTR_ENABLE();
#endif /*RELAY_DOUBLE_TRY > 0*/
#endif /*RELAY_DOUBLE_TRY*/

  if (rs_cfg != NULL) {

    if (__supla_esp_gpio_relay_is_hi(rs_cfg->up) == 0 &&
        __supla_esp_gpio_relay_is_hi(rs_cfg->down) == 0) {

      if (rs_cfg->start_time != 0) {
        rs_cfg->start_time = 0;
      }

      if (rs_cfg->stop_time == 0) {
        rs_cfg->stop_time = t;
      }

    } else if (__supla_esp_gpio_relay_is_hi(rs_cfg->up) != 0 ||
        __supla_esp_gpio_relay_is_hi(rs_cfg->down) != 0) {

      if (rs_cfg->start_time == 0) {
        rs_cfg->start_time = t;
      }

      if (rs_cfg->stop_time != 0) {
        rs_cfg->stop_time = 0;
      }
    }
  }
  result = 1;

  // supla_log(LOG_DEBUG, "2. port = %i, hi = %i, time=%i, t=%i, %i", port, hi,
  // *time, t, t-(*time));

  os_delay_us(10);
  supla_esp_gpio_btn_irq_lock(0);

#ifdef RELAY_AFTER_CHANGE_STATE
  RELAY_AFTER_CHANGE_STATE;
#endif

  if (result == 1 && state != NULL) {
    *state = hi;
    supla_esp_save_state(SAVE_STATE_DELAY);
  }

  system_soft_wdt_restart();

  return result;
}

void GPIO_ICACHE_FLASH supla_esp_gpio_relay_switch_by_input(
    supla_input_cfg_t *input_cfg, unsigned char hi) {
  supla_esp_gpio_relay_switch(input_cfg->relay_gpio_id, hi);
}

void GPIO_ICACHE_FLASH supla_esp_gpio_relay_switch(int port, unsigned char hi) {

  if (port != 255) {
    int channel = -1;
    for (int i = 0; i < RELAY_MAX_COUNT; i++) {
      if (supla_relay_cfg[i].gpio_id == port) {
        channel = supla_relay_cfg[i].channel;
      }
    }

#ifndef COUNTDOWN_TIMER_DISABLED
    // If Time2 > 0 then it is configured as staircase timer. In such case
    // button behavior depends on cfg parameter
    if (channel < CFG_TIME2_COUNT && hi != 0 &&
        supla_esp_cfg.Time2[channel] > 0 &&
        supla_esp_cfg.StaircaseButtonType == STAIRCASE_BTN_TYPE_RESET) {
      hi = HI_VALUE;
    }
    if (hi == 255) {
      hi = supla_esp_gpio_relay_is_hi(port) == 1 ? LO_VALUE : HI_VALUE;
    }
    if (channel >= 0 && channel < STATE_CFG_TIME2_COUNT) {
      supla_esp_state.Time2Left[channel] = 0;
      supla_esp_gpio_relay_set_duration_timer(channel, hi, 0, 0);
    }
#endif /*COUNTDOWN_TIMER_DISABLED*/

		supla_esp_gpio_relay_hi(port, hi);

    if (channel >= 0) {
#ifdef BOARD_CHANNEL_VALUE_CHANGED
      BOARD_CHANNEL_VALUE_CHANGED
#else
        supla_esp_channel_value_changed(channel, 
            supla_esp_gpio_relay_is_hi(port));
#endif
#ifdef MQTT_SUPPORT_ENABLED
#ifdef MQTT_HA_RELAY_SUPPORT
      supla_esp_board_mqtt_on_relay_state_changed(channel);
#endif
#endif
    }
	}
}

void GPIO_ICACHE_FLASH
supla_esp_gpio_on_input_active(supla_input_cfg_t *input_cfg) {

	//supla_log(LOG_DEBUG, "active");

	#ifdef BOARD_ON_INPUT_ACTIVE
	BOARD_ON_INPUT_ACTIVE;
	#endif

  bool advanced_mode = supla_esp_input_is_advanced_mode_enabled(input_cfg);

  if (((input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE &&
        input_cfg->flags & INPUT_FLAG_TRIGGER_ON_PRESS) ||
      input_cfg->type == INPUT_TYPE_BTN_BISTABLE ||
      input_cfg->type == INPUT_TYPE_MOTION_SENSOR ||
      advanced_mode) &&
      input_cfg->relay_gpio_id != 255) {
    supla_roller_shutter_cfg_t *rs_cfg =
      supla_esp_gpio_get_rs__cfg(input_cfg->relay_gpio_id);
    if (rs_cfg != NULL) {
#ifdef _ROLLERSHUTTER_SUPPORT
      int direction = (rs_cfg->up->gpio_id == input_cfg->relay_gpio_id)
        ? RS_RELAY_UP
        : RS_RELAY_DOWN;

      if (input_cfg->type == INPUT_TYPE_BTN_BISTABLE) {
        supla_esp_gpio_rs_set_relay(rs_cfg, direction, 1, 1);
      } else { // monostable
        if (supla_esp_gpio_rs_get_value(rs_cfg) != RS_RELAY_OFF) {
          supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_OFF, 1, 1);
        } else {
          supla_esp_gpio_rs_set_relay(rs_cfg, direction, 1, 1);
        }
      }
#endif /*_ROLLERSHUTTER_SUPPORT*/
    } else {
      unsigned char newState = 255;
      if (input_cfg->type == INPUT_TYPE_MOTION_SENSOR) {
        if (input_cfg->active_triggers & SUPLA_ACTION_CAP_TURN_ON) {
          // ignore when type is motion sensor and AT is configured for turn on
          return;
        }
        newState = 1;
      }
      supla_esp_gpio_relay_switch_by_input(input_cfg, newState);
    }
  } else if (input_cfg->type == INPUT_TYPE_SENSOR && input_cfg->channel != 255) {

    // TODO: add MQTT support for sensor
    supla_esp_channel_value_changed(input_cfg->channel, 1);
  }

}

void GPIO_ICACHE_FLASH
supla_esp_gpio_on_input_inactive(supla_input_cfg_t *input_cfg) {
  // supla_log(LOG_DEBUG, "inactive");

#ifdef BOARD_ON_INPUT_INACTIVE
  BOARD_ON_INPUT_INACTIVE;
#endif

  if (((input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE &&
        !(input_cfg->flags & INPUT_FLAG_TRIGGER_ON_PRESS)) ||
      input_cfg->type == INPUT_TYPE_BTN_BISTABLE ||
      input_cfg->type == INPUT_TYPE_MOTION_SENSOR) &&
      input_cfg->relay_gpio_id != 255) {
    supla_roller_shutter_cfg_t *rs_cfg =
      supla_esp_gpio_get_rs__cfg(input_cfg->relay_gpio_id);
    if (rs_cfg != NULL) {
#ifdef _ROLLERSHUTTER_SUPPORT
      int direction = (rs_cfg->up->gpio_id == input_cfg->relay_gpio_id)
          ? RS_RELAY_UP
          : RS_RELAY_DOWN;
      if (input_cfg->type == INPUT_TYPE_BTN_BISTABLE) {
        // inactive button state (change to "released"/unpress) should
        // switch off relay only if current RS movement direction is the same
        // as button
        if (supla_esp_gpio_rs_get_value(rs_cfg) == direction
            || (supla_esp_gpio_rs_get_value(rs_cfg) == RS_RELAY_OFF
            && rs_cfg->delayed_trigger.value == direction)) {
          supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_OFF, 1, 1);
        }
      } else { // monostable
        if (supla_esp_gpio_rs_get_value(rs_cfg) != RS_RELAY_OFF) {
          supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_OFF, 1, 1);
        } else {
          supla_esp_gpio_rs_set_relay(rs_cfg, direction, 1, 1);
        }
      }
#endif /*_ROLLERSHUTTER_SUPPORT*/
    } else {
      unsigned char newState = 255;
      if (input_cfg->type == INPUT_TYPE_MOTION_SENSOR) {
        if (input_cfg->active_triggers & SUPLA_ACTION_CAP_TURN_OFF) {
          // ignore when type is motion sensor and AT is configured for turn off
          return;
        }
        newState = 0;
      }
      supla_esp_gpio_relay_switch_by_input(input_cfg, newState);
    }
  } else if (input_cfg->type == INPUT_TYPE_SENSOR &&
      input_cfg->channel != 255) {

    // TODO: add MQTT support for sensor
    supla_esp_channel_value_changed(input_cfg->channel, 0);
  }
}

LOCAL void supla_esp_gpio_intr_handler(void *params) {
  int a;
  uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
  supla_input_cfg_t *input_cfg;

#ifdef BOARD_INTR_HANDLER
  BOARD_INTR_HANDLER;
#endif

  // supla_log(LOG_DEBUG, "INTR");

  for (a = 0; a < INPUT_MAX_COUNT; a++) {
    input_cfg = &supla_input_cfg[a];
    if (input_cfg->gpio_id != 255 && input_cfg->gpio_id < 16 &&
        gpio_status & BIT(input_cfg->gpio_id)) {
      ETS_GPIO_INTR_DISABLE();

      gpio_pin_intr_state_set(GPIO_ID_PIN(input_cfg->gpio_id),
          GPIO_PIN_INTR_DISABLE);
      GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS,
          gpio_status &
          BIT(input_cfg->gpio_id));  // //clear interrupt status

//      supla_log(LOG_DEBUG, "INTR start timer %d", input_cfg->gpio_id);
      supla_esp_input_start_debounce_timer(input_cfg);

      gpio_pin_intr_state_set(GPIO_ID_PIN(input_cfg->gpio_id),
          GPIO_PIN_INTR_ANYEDGE);
      gpio_status = gpio_status ^ BIT(input_cfg->gpio_id);

      ETS_GPIO_INTR_ENABLE();
    }
  }

  // Disable uncaught interrupts
  if (gpio_status != 0) {
    for (a = 0; a < 16; a++) {
      if (gpio_status & BIT(a) && INTR_CLEAR_MASK & BIT(a)) {
        ETS_GPIO_INTR_DISABLE();
        gpio_pin_intr_state_set(GPIO_ID_PIN(a), GPIO_PIN_INTR_DISABLE);
        GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(a));
        ETS_GPIO_INTR_ENABLE();
      }
    }
  }
}

void GPIO_ICACHE_FLASH
supla_esp_gpio_init(void) {

	int a;
	//supla_log(LOG_DEBUG, "supla_esp_gpio_init");

	supla_esp_gpio_init_time = 0;
  supla_esp_restart_on_cfg_press = 0;
  bool schedule_motion_sensor_update = false;
	memset(&supla_input_cfg, 0, sizeof(supla_input_cfg));
	memset(&supla_relay_cfg, 0, sizeof(supla_relay_cfg));
	memset(&supla_rs_cfg, 0, sizeof(supla_rs_cfg));

	for (a=0; a<INPUT_MAX_COUNT; a++) {
		supla_input_cfg[a].gpio_id = 255;
		supla_input_cfg[a].relay_gpio_id = 255;
		supla_input_cfg[a].disabled_relay_gpio_id = 255;
		supla_input_cfg[a].channel = 255;
		supla_input_cfg[a].last_state = INPUT_STATE_INACTIVE;
	}

	for (a=0; a<RELAY_MAX_COUNT; a++) {
		supla_relay_cfg[a].gpio_id = 255;
		supla_relay_cfg[a].channel = 255;
	}


	for(a=0; a<RS_MAX_COUNT; a++) {
		supla_rs_cfg[a].position = &supla_esp_state.rs_position[a];
		supla_rs_cfg[a].tilt = &supla_esp_state.tilt[a];
		supla_rs_cfg[a].full_opening_time = &supla_esp_cfg.Time1[a];
		supla_rs_cfg[a].full_closing_time = &supla_esp_cfg.Time2[a];
		supla_rs_cfg[a].tilt_change_time = &supla_esp_cfg.Time3[a];
    supla_rs_cfg[a].auto_opening_time = &supla_esp_cfg.AutoCalOpenTime[a];
    supla_rs_cfg[a].auto_closing_time = &supla_esp_cfg.AutoCalCloseTime[a];
    supla_rs_cfg[a].tilt_type = &supla_esp_cfg.TiltControlType[a];
    supla_log(
        LOG_DEBUG,
        "RS loaded: position %d, tilt %d, auto open %d, auto close %d, "
        "open %d, close %d, tilting %d, type %d",
        *supla_rs_cfg[a].position, *supla_rs_cfg[a].tilt,
        *supla_rs_cfg[a].auto_opening_time, *supla_rs_cfg[a].auto_closing_time,
        *supla_rs_cfg[a].full_opening_time, *supla_rs_cfg[a].full_closing_time,
        *supla_rs_cfg[a].tilt_change_time, *supla_rs_cfg[a].tilt_type);
    if (*supla_rs_cfg[a].tilt_type == 0 ||
        *supla_rs_cfg[a].tilt_change_time == 0) {
      *supla_rs_cfg[a].tilt = -1;
    }
  }

	#if defined(USE_GPIO3) ||  defined(USE_GPIO1) || defined(UART_SWAP)
		 system_uart_swap ();
	#endif

	ETS_GPIO_INTR_DISABLE();

	GPIO_PORT_INIT;
	ETS_GPIO_INTR_ATTACH(supla_esp_gpio_intr_handler, NULL);

    #ifdef USE_GPIO16_INPUT
	gpio16_input_conf();
    #endif

	#ifdef USE_GPIO16_OUTPUT
	gpio16_output_conf();
	#endif


    #ifdef USE_GPIO3
	   PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3);
	   PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0RXD_U);
    #endif

    #ifdef USE_GPIO1
	   PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1);
	   PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
	#endif

    #ifdef LED_RED_PORT
      #if LED_RED_PORT != 16
	    gpio_pin_intr_state_set(GPIO_ID_PIN(LED_RED_PORT), GPIO_PIN_INTR_DISABLE);
      #endif
    #endif

    #ifdef LED_GREEN_PORT
    gpio_pin_intr_state_set(GPIO_ID_PIN(LED_GREEN_PORT), GPIO_PIN_INTR_DISABLE);
    #endif

    #ifdef LED_BLUE_PORT
    gpio_pin_intr_state_set(GPIO_ID_PIN(LED_BLUE_PORT), GPIO_PIN_INTR_DISABLE);
    #endif

	ETS_GPIO_INTR_ENABLE();

	supla_esp_board_gpio_init();

    ETS_GPIO_INTR_DISABLE();

	for (a=0; a<RELAY_MAX_COUNT; a++) {

		if ( supla_relay_cfg[a].gpio_id != 255 ) {

			  //supla_log(LOG_DEBUG, "relay init %i", supla_relay_cfg[a].gpio_id);

			if ( supla_relay_cfg[a].gpio_id <= 15
				 && !(supla_relay_cfg[a].flags & RELAY_FLAG_VIRTUAL_GPIO ) ) {

				GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(supla_relay_cfg[a].gpio_id));
        gpio_pin_intr_state_set(GPIO_ID_PIN(supla_relay_cfg[a].gpio_id),
            GPIO_PIN_INTR_DISABLE);

			} else if (supla_relay_cfg[a].gpio_id == 16) {
				gpio16_output_conf();
			}

      // In case of motion sensor input we don't restore state, but we'll
      // set relay according to startup state of input.
      int input_number = 0;
      for (; input_number < INPUT_MAX_COUNT; input_number++) {
        if (supla_input_cfg[input_number].relay_gpio_id ==
            supla_relay_cfg[a].gpio_id) {
          break;
        }
      }
      if (input_number < INPUT_MAX_COUNT &&
          supla_input_cfg[input_number].type == INPUT_TYPE_MOTION_SENSOR) {
        schedule_motion_sensor_update = true;
        continue;
      }

      if (supla_relay_cfg[a].flags & RELAY_FLAG_RESTORE_FORCE ||
          (supla_relay_cfg[a].flags & RELAY_FLAG_RESTORE &&
           system_get_rst_info()->reason == 0)) {
				//supla_log(LOG_DEBUG, "RESTORE, %i, %i, %i", a, supla_relay_cfg[a].gpio_id, supla_esp_state.Relay[a]);
#ifndef COUNTDOWN_TIMER_DISABLED
        int channel = supla_relay_cfg[a].channel;
        if (channel >= 0 && channel < STATE_CFG_TIME2_COUNT) {
          supla_log(LOG_DEBUG, 
              "Restoring relay state: ch %d, value %d, duration %d",
              channel, supla_esp_state.Relay[a],
              supla_esp_state.Time2Left[channel]);
          supla_esp_gpio_relay_set_duration_timer(channel,
              supla_esp_state.Relay[a],
              supla_esp_state.Time2Left[channel], 0);
        }
#endif /*COUNTDOWN_TIMER_DISABLED*/
        supla_esp_gpio_relay_hi(supla_relay_cfg[a].gpio_id,
            supla_esp_state.Relay[a]);
			} else if ( supla_relay_cfg[a].flags & RELAY_FLAG_RESET ) {

				//supla_log(LOG_DEBUG, "LO_VALUE");
				supla_esp_gpio_relay_hi(supla_relay_cfg[a].gpio_id, LO_VALUE);

			}
		}

	}


    for (a=0; a<INPUT_MAX_COUNT; a++)
      if ( supla_input_cfg[a].gpio_id != 255
    	    && supla_input_cfg[a].gpio_id < 16
    		&& supla_input_cfg[a].type != 0 ) {

    	  //supla_log(LOG_DEBUG, "input init %i", supla_input_cfg[a].gpio_id);

        gpio_output_set(0, 0, 0, BIT(supla_input_cfg[a].gpio_id));

        gpio_register_set(GPIO_PIN_ADDR(supla_input_cfg[a].gpio_id), GPIO_PIN_INT_TYPE_SET(GPIO_PIN_INTR_DISABLE)
                          | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_DISABLE)
                          | GPIO_PIN_SOURCE_SET(GPIO_AS_PIN_SOURCE));

        GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(supla_input_cfg[a].gpio_id));

        if ( !(supla_input_cfg[a].flags & INPUT_FLAG_DISABLE_INTR) ) {
          //supla_log(LOG_DEBUG, "Input %d - enabling intr anyedge", supla_input_cfg[a].gpio_id);
        	gpio_pin_intr_state_set(GPIO_ID_PIN(supla_input_cfg[a].gpio_id), GPIO_PIN_INTR_ANYEDGE);
        }

    }

    ETS_GPIO_INTR_ENABLE();

    supla_esp_gpio_init_time = system_get_time();

	#ifdef _ROLLERSHUTTER_SUPPORT
		for(a=0;a<RS_MAX_COUNT;a++)
			if ( supla_rs_cfg[a].up != NULL
				 && supla_rs_cfg[a].down != NULL ) {

				supla_rs_cfg[a].stop_time = supla_esp_gpio_init_time;

				os_timer_disarm(&supla_rs_cfg[a].timer);
				os_timer_setfn(&supla_rs_cfg[a].timer, (os_timer_func_t *)supla_esp_gpio_rs_timer_cb, &supla_rs_cfg[a]);
				os_timer_arm(&supla_rs_cfg[a].timer, 10, 1);

			}
	#endif /*_ROLLERSHUTTER_SUPPORT*/

    for (int i = 0; i < INPUT_MAX_COUNT; i++) {
      if (supla_input_cfg[i].gpio_id != 255) {
        if (supla_input_cfg[i].type == INPUT_TYPE_MOTION_SENSOR &&
            supla_input_cfg[i].relay_gpio_id != 255) {
          schedule_motion_sensor_update = true;
        }
        if (!(supla_input_cfg[i].flags & INPUT_FLAG_DISABLE_INTR)) {
          supla_esp_input_start_debounce_timer(&supla_input_cfg[i]);
        }
      }
    }

    if (schedule_motion_sensor_update) {
      os_timer_disarm(&supla_motion_sensor_init_timer);
      os_timer_setfn(&supla_motion_sensor_init_timer, supla_esp_gpio_set_motion_sensor_state, NULL);
      os_timer_arm (&supla_motion_sensor_init_timer, INPUT_SILENT_STARTUP_TIME_MS + 100, 0);
    }

}

void supla_esp_gpio_set_hi(int port, unsigned char hi) {

    //supla_log(LOG_DEBUG, "supla_esp_gpio_set_hi %i, %i", port, hi);

	#ifdef BOARD_GPIO_OUTPUT_SET_HI
	BOARD_GPIO_OUTPUT_SET_HI
	#endif

	if ( port == 16 ) {
		gpio16_output_set(hi == 1 ? 1 : 0);
	} else {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(port), hi == 1 ? 1 : 0);
	}

}

void GPIO_ICACHE_FLASH supla_esp_gpio_set_led(char r, char g, char b) {

	os_timer_disarm(&supla_gpio_timer1);
	os_timer_disarm(&supla_gpio_timer2);

    #ifdef LED_RED_PORT
		supla_esp_gpio_set_hi(LED_RED_PORT, r);
    #endif

    #ifdef LED_GREEN_PORT
		supla_esp_gpio_set_hi(LED_GREEN_PORT, g);
    #endif

	#ifdef LED_BLUE_PORT
		supla_esp_gpio_set_hi(LED_BLUE_PORT, b);
	#endif
}

#ifdef LED_REINIT
void GPIO_ICACHE_FLASH supla_esp_gpio_reinit_led(void) {

	ETS_GPIO_INTR_DISABLE();

	#ifdef LED_RED_PORT
		#if LED_RED_PORT != 16
			GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(LED_RED_PORT));
			gpio_pin_intr_state_set(GPIO_ID_PIN(LED_RED_PORT), GPIO_PIN_INTR_DISABLE);
			gpio_output_set(0, BIT(LED_RED_PORT), BIT(LED_RED_PORT), 0);
		#endif
	#endif

	#ifdef LED_GREEN_PORT
		#if LED_GREEN_PORT != 16
			GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(LED_GREEN_PORT));
			gpio_pin_intr_state_set(GPIO_ID_PIN(LED_GREEN_PORT), GPIO_PIN_INTR_DISABLE);
			gpio_output_set(0, BIT(LED_GREEN_PORT), BIT(LED_GREEN_PORT), 0);
		#endif
	#endif

	#ifdef LED_BLUE_PORT
		#if LED_BLUE_PORT != 16
			GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(LED_BLUE_PORT));
			gpio_pin_intr_state_set(GPIO_ID_PIN(LED_BLUE_PORT), GPIO_PIN_INTR_DISABLE);
			gpio_output_set(0, BIT(LED_BLUE_PORT), BIT(LED_BLUE_PORT), 0);
		#endif
	#endif

    ETS_GPIO_INTR_ENABLE();

}
#endif


#if defined(LED_RED_PORT) || defined(LED_GREEN_PORT) || defined(LED_BLUE_PORT)
void GPIO_ICACHE_FLASH
supla_esp_gpio_led_blinking_func(void *timer_arg) {

	#ifdef LED_RED_PORT
		if ( (int)timer_arg & LED_RED )
			supla_esp_gpio_set_hi(LED_RED_PORT, supla_esp_gpio_output_is_hi(LED_RED_PORT) == 0 ? 1 : 0);
	#endif

	#ifdef LED_GREEN_PORT
		if ( (int)timer_arg & LED_GREEN )
			supla_esp_gpio_set_hi(LED_GREEN_PORT, supla_esp_gpio_output_is_hi(LED_GREEN_PORT) == 0 ? 1 : 0);
	#endif

    #ifdef LED_BLUE_PORT
		if ( (int)timer_arg & LED_BLUE )
			supla_esp_gpio_set_hi(LED_BLUE_PORT, supla_esp_gpio_output_is_hi(LED_BLUE_PORT) == 0 ? 1 : 0);
    #endif

}

void GPIO_ICACHE_FLASH
supla_esp_gpio_led_blinking(int led, int time) {

	supla_esp_gpio_set_led(0, 0, 0);

	os_timer_setfn(&supla_gpio_timer1, supla_esp_gpio_led_blinking_func, (void*)led);
	os_timer_arm (&supla_gpio_timer1, time, true);

}
#endif

void GPIO_ICACHE_FLASH
supla_esp_gpio_state_disconnected(void) {

	if ( supla_last_state == STATE_DISCONNECTED
			|| supla_esp_cfgmode_started()
			|| supla_esp_update_started())
		return;

	supla_last_state = STATE_DISCONNECTED;
	supla_log(LOG_DEBUG, "Disconnected");

	#ifdef BOARD_ESP_ON_STATE_CHANGED
	supla_esp_board_on_state_changed(supla_last_state);
	#endif

	#ifdef LED_REINIT
	supla_esp_gpio_reinit_led();
	#endif

    #if defined(LED_RED_PORT) && defined(LED_GREEN_PORT) && defined(LED_BLUE_PORT)
	supla_esp_gpio_set_led(1, 0, 0);
    #elif defined(LED_GREEN_PORT) && defined(LED_BLUE_PORT)
	supla_esp_gpio_led_blinking(LED_GREEN, 500);
    #elif defined(LED_RED_PORT)
	supla_esp_gpio_led_blinking(LED_RED, 2000);
    #endif

}

void GPIO_ICACHE_FLASH
supla_esp_gpio_state_ipreceived(void) {

	if ( supla_last_state == STATE_IPRECEIVED
			|| supla_esp_cfgmode_started()
			|| supla_esp_update_started())
		return;

	supla_last_state = STATE_IPRECEIVED;

	#ifdef BOARD_ESP_ON_STATE_CHANGED
	supla_esp_board_on_state_changed(supla_last_state);
	#endif

	#if defined(LED_RED_PORT) && defined(LED_GREEN_PORT) && defined(LED_BLUE_PORT)
	supla_esp_gpio_set_led(0, 0, 1);
	#elif defined(LED_GREEN_PORT) && defined(LED_BLUE_PORT)
	supla_esp_gpio_led_blinking(LED_GREEN | LED_BLUE, 500);
	#elif defined(LED_RED_PORT)
	supla_esp_gpio_led_blinking(LED_RED, 500);
	#endif
}

void GPIO_ICACHE_FLASH
supla_esp_gpio_enable_sensors(void *timer_arg) {

	int a;
	supla_esp_gpio_set_led(0, 0, 0);

	ETS_GPIO_INTR_DISABLE();

	for(a=0;a<INPUT_MAX_COUNT;a++)
		if ( supla_input_cfg[a].gpio_id != 255
			 && supla_input_cfg[a].gpio_id <= 16
			 && supla_input_cfg[a].type == INPUT_TYPE_SENSOR )

		supla_esp_gpio_enable_input_port(supla_input_cfg[a].gpio_id);


	ETS_GPIO_INTR_ENABLE();

}


void GPIO_ICACHE_FLASH
supla_esp_gpio_state_connected(void) {

	if ( supla_last_state == STATE_CONNECTED
			|| supla_esp_cfgmode_started()
			|| supla_esp_update_started())
		return;

	supla_last_state = STATE_CONNECTED;

	#ifdef BOARD_ESP_ON_STATE_CHANGED
	supla_esp_board_on_state_changed(supla_last_state);
	#endif

	#if defined(LED_RED_PORT) && defined(LED_GREEN_PORT) && defined(LED_BLUE_PORT)
	supla_esp_gpio_set_led(0, 1, 0);
	#else
	supla_esp_gpio_set_led(0, 0, 0);
	#endif

	os_timer_setfn(&supla_gpio_timer2, supla_esp_gpio_enable_sensors, NULL);
    os_timer_arm (&supla_gpio_timer2, 1000, 0);

	#ifdef BOARD_ON_CONNECT
    supla_esp_board_on_connect();
	#endif
}


void GPIO_ICACHE_FLASH
supla_esp_gpio_state_cfgmode(void) {

	if ( supla_last_state == STATE_CFGMODE )
		return;

	supla_last_state = STATE_CFGMODE;

	#ifdef BOARD_ESP_ON_STATE_CHANGED
	supla_esp_board_on_state_changed(supla_last_state);
	#endif

	#ifdef LED_REINIT
	supla_esp_gpio_reinit_led();
	#endif

	#if defined(LED_RED_PORT) && defined(LED_GREEN_PORT) && defined(LED_BLUE_PORT)
    supla_esp_gpio_led_blinking(LED_BLUE, 500);
	#elif defined(LED_GREEN_PORT) && defined(LED_BLUE_PORT)
	supla_esp_gpio_led_blinking(LED_BLUE, 500);
	#elif defined(LED_RED_PORT)
	supla_esp_gpio_led_blinking(LED_RED, 100);
	#endif

}

#ifdef __FOTA
void GPIO_ICACHE_FLASH supla_esp_gpio_state_update(void) {

	if ( supla_last_state == STATE_UPDATE )
		return;

	supla_last_state = STATE_UPDATE;

	#ifdef BOARD_ESP_ON_STATE_CHANGED
	supla_esp_board_on_state_changed(supla_last_state);
	#endif

    #if defined(LED_RED_PORT)
		supla_esp_gpio_led_blinking(LED_RED, 20);
	#endif
}
#endif

char  supla_esp_gpio_output_is_hi(int port) {

	#ifdef BOARD_GPIO_OUTPUT_IS_HI
		BOARD_GPIO_OUTPUT_IS_HI
	#endif

	if ( port == 16 ) {
		return gpio16_output_get() == 1 ? 1 : 0;
	}


	return GPIO_OUTPUT_GET(port) == 1 ? 1 : 0;
}

char __supla_esp_gpio_relay_is_hi(supla_relay_cfg_t *relay_cfg) {

	char result = supla_esp_gpio_output_is_hi(relay_cfg->gpio_id);

	if ( relay_cfg->flags & RELAY_FLAG_LO_LEVEL_TRIGGER ) {
		result = result == HI_VALUE ? LO_VALUE : HI_VALUE;
	}

	return result;
}

char supla_esp_gpio_relay_is_hi(int port) {

	char result = supla_esp_gpio_output_is_hi(port);
	int a;

    for(a=0;a<RELAY_MAX_COUNT;a++)
    	if ( supla_relay_cfg[a].gpio_id == port ) {

    		if ( supla_relay_cfg[a].flags &  RELAY_FLAG_LO_LEVEL_TRIGGER ) {
    			result = result == HI_VALUE ? LO_VALUE : HI_VALUE;
    		}

    		break;
    	}

    return result;
}

char  supla_esp_gpio_relay_on(int port) {
	return supla_esp_gpio_output_is_hi(port) == HI_VALUE ? 1 : 0;
}

void GPIO_ICACHE_FLASH supla_esp_gpio_relay_set_duration_timer(int channel,
    int newValue,
    int durationMs,
    int senderID) {
#ifndef COUNTDOWN_TIMER_DISABLED
  if (channel < STATE_CFG_TIME2_COUNT && channel < CFG_TIME2_COUNT &&
      supla_esp_cfg.Time2[channel] > 0) {
    // this is for staircase timer - we reset duration ms to the duration
    // that was configred on the server.
    // Staircase timer uses the same implementation as Countdown timer
    if (newValue == 0) {
      durationMs = 0;
      supla_esp_state.Time2Left[channel] = 0;
    } else {
      if (durationMs == 0 || supla_esp_state.Time2Left[channel] != durationMs) {
        durationMs = supla_esp_cfg.Time2[channel];
      }
    }
  }

  supla_esp_countdown_timer_disarm(channel);

  if (durationMs > 0) {

    for (int a = 0; a < RELAY_MAX_COUNT; a++) {
      if (supla_relay_cfg[a].gpio_id != 255 &&
          channel == supla_relay_cfg[a].channel) {

        if (newValue == 1 || supla_relay_cfg[a].channel_flags &
            SUPLA_CHANNEL_FLAG_COUNTDOWN_TIMER_SUPPORTED) {

          char target_value[SUPLA_CHANNELVALUE_SIZE] = {};
          target_value[0] = newValue ? 0 : 1;

          supla_esp_countdown_timer_countdown(durationMs,
              supla_relay_cfg[a].gpio_id,
              channel, target_value, senderID);
        }
#if ESP8266_SUPLA_PROTO_VERSION >= 12
        if (supla_relay_cfg[a].channel_flags &
            SUPLA_CHANNEL_FLAG_COUNTDOWN_TIMER_SUPPORTED) {
          TSuplaChannelExtendedValue *ev = (TSuplaChannelExtendedValue *)malloc(
              sizeof(TSuplaChannelExtendedValue));
          if (ev != NULL) {
            supla_esp_countdown_get_state_ev(channel, ev);
            supla_esp_channel_extendedvalue_changed(channel, ev);
            free(ev);
          }
        }
#endif /*ESP8266_SUPLA_PROTO_VERSION >= 12*/
        break;
      }
    }
  }
#endif /*COUNTDOWN_TIMER_DISABLED*/
}

void GPIO_ICACHE_FLASH supla_esp_gpio_clear_vars(void) {
  supla_last_state = STATE_UNKNOWN;
  supla_esp_gpio_init_time = 0;
  supla_esp_restart_on_cfg_press = 0;
  silent_period = true;
}

void GPIO_ICACHE_FLASH supla_esp_gpio_set_motion_sensor_state(void *timer_arg) {
  for (int i = 0; i < INPUT_MAX_COUNT; i++) {
    supla_input_cfg_t *input = &supla_input_cfg[i];
    if (input->relay_gpio_id != 255 &&
        input->type == INPUT_TYPE_MOTION_SENSOR) {
      supla_log(LOG_DEBUG, "sensor state i %d, active %d", i, input->last_state);
      if (input->last_state == INPUT_STATE_ACTIVE) {
        supla_esp_gpio_on_input_active(input);
      } else if (input->last_state == INPUT_STATE_INACTIVE) {
        supla_esp_gpio_on_input_inactive(input);
      }
    }
  }
}
