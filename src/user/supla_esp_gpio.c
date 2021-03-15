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

#include "supla-dev/log.h"

#define GPIO_OUTPUT_GET(gpio_no)     ((gpio_output_get()>>gpio_no)&BIT0)

#define LED_RED    0x1
#define LED_GREEN  0x2
#define LED_BLUE   0x4

#define CFG_BTN_PRESS_COUNT     10

#define RS_STATE_STOP           0
#define RS_STATE_DOWN           1
#define RS_STATE_UP             2

supla_input_cfg_t supla_input_cfg[INPUT_MAX_COUNT];
supla_relay_cfg_t supla_relay_cfg[RELAY_MAX_COUNT];
supla_roller_shutter_cfg_t supla_rs_cfg[RS_MAX_COUNT];

unsigned int supla_esp_gpio_init_time = 0;

static char supla_last_state = STATE_UNKNOWN;
static ETSTimer supla_gpio_timer1;
static ETSTimer supla_gpio_timer2;
unsigned char supla_esp_restart_on_cfg_press = 0;

#ifdef _ROLLERSHUTTER_SUPPORT
void
supla_esp_gpio_rs_calibrate(supla_roller_shutter_cfg_t *rs_cfg, unsigned int full_time, unsigned int time, int pos) {

	if ( ( *rs_cfg->position < 100 || *rs_cfg->position > 10100 )
		 && full_time > 0 ) {

		full_time *= 1.1; // 10% margin

		if ( time >= full_time ) {
			*rs_cfg->position = pos;
			supla_esp_save_state(RS_SAVE_STATE_DELAY);
		}
	};

}

#define RS_DIRECTION_NONE   0
#define RS_DIRECTION_UP     2
#define RS_DIRECTION_DOWN   1

void
supla_esp_gpio_rs_set_relay_delayed(void *timer_arg) {

	supla_esp_gpio_rs_set_relay((supla_roller_shutter_cfg_t*)timer_arg, ((supla_roller_shutter_cfg_t*)timer_arg)->delayed_trigger.value, 0, 0);

}

void
supla_esp_gpio_rs_set_relay(supla_roller_shutter_cfg_t *rs_cfg, uint8 value, uint8 cancel_task, uint8 stop_delay) {

	if ( rs_cfg == NULL ) {
		return;
	}

	unsigned int t = system_get_time();
	unsigned int delay_time = 0;
	
	if ( cancel_task ) {
		supla_esp_gpio_rs_cancel_task(rs_cfg);
	}
	
	
	os_timer_disarm(&rs_cfg->delayed_trigger.timer);

	if ( value == RS_RELAY_OFF )  {
		
		if ( RS_STOP_DELAY
			 && stop_delay == 1
			 && rs_cfg->start_time > 0
			 && rs_cfg->stop_time == 0
			 && t >= rs_cfg->start_time
			 && (t - rs_cfg->start_time)/1000 < RS_STOP_DELAY) {

			delay_time = RS_STOP_DELAY - (t - rs_cfg->start_time)/1000 + 1;
		}
		
	} else {

		supla_relay_cfg_t *rel = value == RS_RELAY_UP ? rs_cfg->down : rs_cfg->up;

		if ( __supla_esp_gpio_relay_is_hi(rel) ) {

			supla_esp_gpio_relay_hi(rel->gpio_id, 0, 0);
			os_delay_us(10000);
			
			t = system_get_time();
		}
		

		if ( RS_START_DELAY
			 && rs_cfg->start_time == 0
			 && rs_cfg->stop_time > 0
			 && t >= rs_cfg->stop_time
			 && (t - rs_cfg->stop_time)/1000 < RS_START_DELAY) {

			delay_time = RS_START_DELAY - (t - rs_cfg->stop_time)/1000 + 1;
		}
		
	}
	
	//supla_log(LOG_DEBUG, "VALUE: %i", value);
	
	if ( delay_time > 100 ) {
		
		rs_cfg->delayed_trigger.value = value;
		
		//supla_log(LOG_DEBUG, "Delay: %i", delay_time);
		
		os_timer_setfn(&rs_cfg->delayed_trigger.timer, supla_esp_gpio_rs_set_relay_delayed, rs_cfg);
		os_timer_arm(&rs_cfg->delayed_trigger.timer, delay_time, 0);
		
		return;
	}
	
	if (  value == RS_RELAY_UP ) {
		supla_esp_gpio_relay_hi(rs_cfg->up->gpio_id, 1, 0);
	} else if ( value == RS_RELAY_DOWN ) {
		supla_esp_gpio_relay_hi(rs_cfg->down->gpio_id, 1, 0);
	} else {
		supla_esp_gpio_relay_hi(rs_cfg->up->gpio_id, 0, 0);
		supla_esp_gpio_relay_hi(rs_cfg->down->gpio_id, 0, 0);
	}
	
}

uint8 
supla_esp_gpio_rs_get_value(supla_roller_shutter_cfg_t *rs_cfg) {
	
	if ( rs_cfg != NULL ) {
		
		if ( 1 == __supla_esp_gpio_relay_is_hi(rs_cfg->up) ) {
			return RS_RELAY_UP;
		} else if ( 1 == __supla_esp_gpio_relay_is_hi(rs_cfg->down) ) {
			return RS_RELAY_DOWN;
		}
		
	}

	
	return RS_RELAY_OFF;
}

void
supla_esp_gpio_rs_move_position(supla_roller_shutter_cfg_t *rs_cfg, unsigned int *full_time, unsigned int *time, uint8 up) {


	if ( (*rs_cfg->position) < 100
		 || (*rs_cfg->position) > 10100
		 || (*full_time) == 0 ) return;

	int last_pos = *rs_cfg->position;

	int p = ((*time) * 100.00 / (*full_time) * 100);

	unsigned int x = p * (*full_time) / 10000;


	if ( p > 0 ) {

		//supla_log(LOG_DEBUG, "p=%i, x=%i, last_pos=%i, full_time=%i, time=%i",p,x,last_pos,*full_time,*time);

		if ( up == 1 ) {

			if ( (*rs_cfg->position) - p <= 100 )
				(*rs_cfg->position) = 100;
			else
				(*rs_cfg->position) -= p;

		} else {

			if ( (*rs_cfg->position) + p >= 10100 )
				(*rs_cfg->position) = 10100;
			else
				(*rs_cfg->position) += p;

		}

		if ( last_pos != *rs_cfg->position ) {
			supla_esp_save_state(RS_SAVE_STATE_DELAY);
		}


	}

	if ( ((*rs_cfg->position) == 100 && up == 1) || ((*rs_cfg->position) == 10100 && up == 0) ) {

		if ( (*time) >= (*full_time) * 1.1 ) {

			supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_OFF, 0, 0);
			//supla_log(LOG_DEBUG, "Timeout full_time + 10%");

		}

		return;
	}


	if ( x <= (*time) )
		(*time) -= x;
	else  // if some error
		(*time) = 0;

}


uint8
supla_esp_gpio_rs_time_margin(supla_roller_shutter_cfg_t *rs_cfg, unsigned int *full_time, unsigned int time, uint8 m) {

	return  ((*full_time) > 0 && ( time * 100 / (*full_time) ) < m ) ? 1 : 0;

}

void
supla_esp_gpio_rs_task_processing(supla_roller_shutter_cfg_t *rs_cfg) {

	if ( rs_cfg->task.active == 0 )
		return;

	if ( *rs_cfg->position < 100
	     || *rs_cfg->position > 10100 ) {

		if ( 0 == __supla_esp_gpio_relay_is_hi(rs_cfg->down)
			 && 0 == __supla_esp_gpio_relay_is_hi(rs_cfg->up)
			 && *rs_cfg->full_opening_time > 0
			 && *rs_cfg->full_closing_time > 0) {

			if ( rs_cfg->task.percent < 50 )
				supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_UP, 0, 0);
			else
				supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_DOWN, 0, 0);

		}

		return;
	}

	uint8 percent = ((*rs_cfg->position)-100)/100;


	if ( rs_cfg->task.direction == RS_DIRECTION_NONE ) {

		if ( percent > rs_cfg->task.percent ) {

			rs_cfg->task.direction = RS_DIRECTION_UP;
			supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_UP, 0, 0);

		    //supla_log(LOG_DEBUG, "task go UP %i,%i", percent, rs_cfg->task.percent);

		} else if ( percent < rs_cfg->task.percent ) {

			rs_cfg->task.direction = RS_DIRECTION_DOWN;
			supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_DOWN, 0, 0);

			//supla_log(LOG_DEBUG, "task go DOWN, %i,%i", percent, rs_cfg->task.percent);

		} else {
			//supla_log(LOG_DEBUG, "task finished #1");

			rs_cfg->task.active = 0;
			supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_OFF, 0, 0);
		}

	} else if ( ( rs_cfg->task.direction == RS_DIRECTION_UP
				  && percent <= rs_cfg->task.percent )
				|| ( rs_cfg->task.direction == RS_DIRECTION_DOWN
					 && percent >= rs_cfg->task.percent )  ) {

		if ( rs_cfg->task.percent == 0
			 && 1 == supla_esp_gpio_rs_time_margin(rs_cfg, rs_cfg->full_opening_time, rs_cfg->up_time, 5) ) { // margin 5%

			//supla_log(LOG_DEBUG, "UP MARGIN 5%");

		} else if ( rs_cfg->task.percent == 100
				    && 1 == supla_esp_gpio_rs_time_margin(rs_cfg, rs_cfg->full_closing_time, rs_cfg->down_time, 5) ) {

			//supla_log(LOG_DEBUG, "DOWN MARGIN 5%");

		} else {

			rs_cfg->task.active = 0;
			supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_OFF, 0, 0);

			//supla_log(LOG_DEBUG, "task finished #2");

		}


	}



}

void
supla_esp_gpio_rs_timer_cb(void *timer_arg) {

	supla_roller_shutter_cfg_t *rs_cfg = (supla_roller_shutter_cfg_t*)timer_arg;

	if ( supla_esp_gpio_init_time == 0 )
		return;

	unsigned int t = system_get_time()/100000;

	if ( t == 0 )
		t = 1;


	if ( 1 == __supla_esp_gpio_relay_is_hi(rs_cfg->up) ) {

		rs_cfg->down_time = 0;
		rs_cfg->up_time += (t-rs_cfg->last_time)*100;

		supla_esp_gpio_rs_calibrate(rs_cfg, *rs_cfg->full_opening_time, rs_cfg->up_time, 100);
		supla_esp_gpio_rs_move_position(rs_cfg, rs_cfg->full_opening_time, &rs_cfg->up_time, 1);

	} else if ( 1 == __supla_esp_gpio_relay_is_hi(rs_cfg->down) ) {

		rs_cfg->down_time += (t-rs_cfg->last_time)*100;
		rs_cfg->up_time = 0;

		supla_esp_gpio_rs_calibrate(rs_cfg, *rs_cfg->full_closing_time, rs_cfg->down_time, 10100);
		supla_esp_gpio_rs_move_position(rs_cfg, rs_cfg->full_closing_time, &rs_cfg->down_time, 0);

	} else {

		if ( rs_cfg->up_time != 0 )
			rs_cfg->up_time = 0;

		if ( rs_cfg->down_time != 0 )
			rs_cfg->down_time = 0;

	}



	supla_esp_gpio_rs_task_processing(rs_cfg);


	if ( rs_cfg->last_time-rs_cfg->n >= 10 ) { // 10 == 1 sec.

		if ( rs_cfg->last_position != *rs_cfg->position ) {

			rs_cfg->last_position = *rs_cfg->position;
			supla_esp_channel_value_changed(rs_cfg->up->channel, (rs_cfg->last_position-100)/100);
		}

		if ( rs_cfg->up_time > 600000 || rs_cfg->down_time > 600000 ) { // 10 min. - timeout
			supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_OFF, 0, 0);
		}
		
		//supla_log(LOG_DEBUG, "UT: %i, DT: %i, FOT: %i, FCT: %i, pos: %i", rs_cfg->up_time, rs_cfg->down_time, *rs_cfg->full_opening_time, *rs_cfg->full_closing_time, *rs_cfg->position);
		rs_cfg->n = t;
	}



	rs_cfg->last_time = t;
}

void
supla_esp_gpio_rs_cancel_task(supla_roller_shutter_cfg_t *rs_cfg) {

	if ( rs_cfg == NULL )
		return;

	//supla_log(LOG_DEBUG, "Task canceled");

	rs_cfg->task.active = 0;
	rs_cfg->task.percent = 0;
	rs_cfg->task.direction = RS_DIRECTION_NONE;

	supla_esp_save_state(RS_SAVE_STATE_DELAY);
}


void supla_esp_gpio_rs_add_task(int idx, uint8 percent) {

	if ( idx < 0
		 || idx >= RS_MAX_COUNT
		 || ((*supla_rs_cfg[idx].position)-100)/100 == percent )
		return;

	if ( percent > 100 )
		percent = 100;

	//supla_log(LOG_DEBUG, "Task added %i", percent);

	supla_rs_cfg[idx].task.percent = percent;
	supla_rs_cfg[idx].task.direction = RS_DIRECTION_NONE;
	supla_rs_cfg[idx].task.active = 1;

	supla_esp_save_state(RS_SAVE_STATE_DELAY);

}

#endif /*_ROLLERSHUTTER_SUPPORT*/

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

void  supla_esg_gpio_start_cfg_mode(void) {

	if ( supla_esp_cfgmode_started() == 0 ) {

		#ifdef BEFORE_CFG_ENTER
			BEFORE_CFG_ENTER
		#endif

		supla_esp_devconn_stop();
		supla_esp_cfgmode_start();

	};

}

void
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
	int a;

	ETS_GPIO_INTR_DISABLE();

	for(a=0;a<INPUT_MAX_COUNT;a++) {

		input_cfg = &supla_input_cfg[a];

		if ( input_cfg->gpio_id != 255
				&& input_cfg->gpio_id < 16
				&& ( input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE
					 || input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE_RS
					 || input_cfg->type == INPUT_TYPE_BTN_BISTABLE ) ) {

			gpio_pin_intr_state_set(GPIO_ID_PIN(input_cfg->gpio_id), lock == 1 ? GPIO_PIN_INTR_DISABLE : GPIO_PIN_INTR_ANYEDGE);

			if ( lock == 1 )
				GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(input_cfg->gpio_id));

		}



	}

	ETS_GPIO_INTR_ENABLE();


}

supla_roller_shutter_cfg_t  *
supla_esp_gpio_get_rs_cfg(supla_relay_cfg_t *rel_cfg) {

	if ( rel_cfg == NULL )
		return NULL;

	int a;

	for(a=0;a<RS_MAX_COUNT;a++)
		if ( supla_rs_cfg[a].up == rel_cfg
		       || supla_rs_cfg[a].down == rel_cfg ) {

			return &supla_rs_cfg[a];
		}

	return NULL;
}

supla_roller_shutter_cfg_t  *
supla_esp_gpio_get_rs__cfg(int port) {

	int a;

	if ( port == 255 )
		return NULL;

    for(a=0;a<RELAY_MAX_COUNT;a++)
    	if ( supla_relay_cfg[a].gpio_id == port ) {
    		return supla_esp_gpio_get_rs_cfg(&supla_relay_cfg[a]);
    	}

	return NULL;
}

char supla_esp_gpio_relay_hi(int port, char hi, char save_before) {
	
    unsigned int t = system_get_time();
    unsigned int *time = NULL;
    int a;
    char result = 0;
    char *state = NULL;
    supla_roller_shutter_cfg_t *rs_cfg = NULL;
    char _hi;
    
    if ( hi == 255 ) {
    	hi = supla_esp_gpio_relay_is_hi(port) == 1 ? 0 : 1;
    }

    _hi = hi;
    
   
    for(a=0;a<RELAY_MAX_COUNT;a++)
    	if ( supla_relay_cfg[a].gpio_id == port ) {

    		time = &supla_relay_cfg[a].last_time;

    		if ( supla_relay_cfg[a].flags & RELAY_FLAG_RESTORE
    			 || supla_relay_cfg[a].flags & RELAY_FLAG_RESTORE_FORCE )
    			state = &supla_esp_state.Relay[a];

    		if ( supla_relay_cfg[a].flags &  RELAY_FLAG_LO_LEVEL_TRIGGER )
    			_hi = hi == HI_VALUE ? LO_VALUE : HI_VALUE;

    		rs_cfg = supla_esp_gpio_get_rs_cfg(&supla_relay_cfg[a]);
    		break;
    	}
    
    //supla_log(LOG_DEBUG, "port=%i, hi=%i, save_before=%i",port, hi, save_before);
    
    system_soft_wdt_stop();

    if ( save_before == 1
         && state != NULL
    	 && ( time == NULL
    	      || abs(t-(*time)) >= RELAY_MIN_DELAY ) ) {

		*state = hi;
		supla_esp_save_state(0);

    }

    supla_esp_gpio_btn_irq_lock(1);
    os_delay_us(10);

	#ifdef RELAY_BEFORE_CHANGE_STATE
    RELAY_BEFORE_CHANGE_STATE;
	#endif

    if ( time == NULL
    	 || abs(t-(*time)) >= RELAY_MIN_DELAY ) {

        //supla_log(LOG_DEBUG, "1. port = %i, hi = %i", port, _hi);

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

    	if ( time != NULL ) {
    		*time = t;
    	}

    	if ( rs_cfg != NULL ) {

    		if ( __supla_esp_gpio_relay_is_hi(rs_cfg->up) == 0
    			 && __supla_esp_gpio_relay_is_hi(rs_cfg->down) == 0 ) {

    			if ( rs_cfg->start_time != 0 ) {
    				rs_cfg->start_time = 0;
    			}
    			
    			if ( rs_cfg->stop_time == 0 ) {
    				rs_cfg->stop_time = t;
    			}
    			

    		} else if ( __supla_esp_gpio_relay_is_hi(rs_cfg->up) != 0
    				    || __supla_esp_gpio_relay_is_hi(rs_cfg->down) != 0 ) {

    			if ( rs_cfg->start_time == 0 ) {
    				rs_cfg->start_time = t;
    			}
    			
    			if ( rs_cfg->stop_time != 0 ) {
    				rs_cfg->stop_time = 0;
    			}

    		}

    	}
    	result = 1;
    }

    //supla_log(LOG_DEBUG, "2. port = %i, hi = %i, time=%i, t=%i, %i", port, hi, *time, t, t-(*time));

    os_delay_us(10);
    supla_esp_gpio_btn_irq_lock(0);

	#ifdef RELAY_AFTER_CHANGE_STATE
	RELAY_AFTER_CHANGE_STATE;
	#endif

    if ( result == 1
		 && state != NULL
		 && save_before != 255 ) {

		*state = hi;
		supla_esp_save_state(SAVE_STATE_DELAY);
    }


    system_soft_wdt_restart();

    return result;
}

LOCAL void supla_esp_gpio_relay_switch(supla_input_cfg_t *input_cfg, char hi) {

	if (  input_cfg->relay_gpio_id != 255 ) {

		//supla_log(LOG_DEBUG, "RELAY");

		supla_esp_gpio_relay_hi(input_cfg->relay_gpio_id, hi, 0);

		if ( input_cfg->channel != 255 )
			supla_esp_channel_value_changed(input_cfg->channel, supla_esp_gpio_relay_is_hi(input_cfg->relay_gpio_id));

	}

}

LOCAL void
supla_esp_gpio_on_input_active(supla_input_cfg_t *input_cfg) {

	//supla_log(LOG_DEBUG, "active");

	#ifdef BOARD_ON_INPUT_ACTIVE
	BOARD_ON_INPUT_ACTIVE;
	#endif

	if ( input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE_RS ) {

		//supla_log(LOG_DEBUG, "RELAY HI");
		#ifdef _ROLLERSHUTTER_SUPPORT
			supla_roller_shutter_cfg_t *rs_cfg = supla_esp_gpio_get_rs__cfg(input_cfg->relay_gpio_id);
			if ( rs_cfg != NULL ) {
				supla_esp_gpio_rs_set_relay(rs_cfg, rs_cfg->up->gpio_id == input_cfg->relay_gpio_id ? RS_RELAY_UP : RS_RELAY_DOWN, 1, 1);
			}
		#endif /*_ROLLERSHUTTER_SUPPORT*/

	} else if ( input_cfg->type == INPUT_TYPE_BTN_BISTABLE ) {

        #ifndef COUNTDOWN_TIMER_DISABLED
		if (input_cfg->channel != 255) {
		   supla_esp_countdown_timer_disarm(input_cfg->channel);
		}
        #endif /*COUNTDOWN_TIMER_DISABLED*/

		//supla_log(LOG_DEBUG, "RELAY");
		supla_esp_gpio_relay_switch(input_cfg, 255);

	} else if ( input_cfg->type == INPUT_TYPE_SENSOR
				&&  input_cfg->channel != 255 ) {

		supla_esp_channel_value_changed(input_cfg->channel, 1);
	}

	input_cfg->last_state = 1;
}

LOCAL void
supla_esp_gpio_on_input_inactive(supla_input_cfg_t *input_cfg) {

	//supla_log(LOG_DEBUG, "inactive");

	#ifdef BOARD_ON_INPUT_INACTIVE
	BOARD_ON_INPUT_INACTIVE;
	#endif

	if ( input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE_RS ) {

		//supla_log(LOG_DEBUG, "RELAY LO");
		#ifdef _ROLLERSHUTTER_SUPPORT
			supla_roller_shutter_cfg_t *rs_cfg = supla_esp_gpio_get_rs__cfg(input_cfg->relay_gpio_id);
			if ( rs_cfg != NULL ) {
				supla_esp_gpio_rs_set_relay(rs_cfg, RS_RELAY_OFF, 1, 1);
			}
		#endif /*_ROLLERSHUTTER_SUPPORT*/
	} else if ( input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE
		 || input_cfg->type == INPUT_TYPE_BTN_BISTABLE ) {

        #ifndef COUNTDOWN_TIMER_DISABLED
		if (input_cfg->channel != 255) {
		   supla_esp_countdown_timer_disarm(input_cfg->channel);
		}
        #endif /*COUNTDOWN_TIMER_DISABLED*/

		supla_esp_gpio_relay_switch(input_cfg, 255);

	} else if ( input_cfg->type == INPUT_TYPE_SENSOR
			    &&  input_cfg->channel != 255 ) {

		supla_esp_channel_value_changed(input_cfg->channel, 0);
	}

	input_cfg->last_state = 0;
}

uint32 supla_esp_gpio_get_cfg_press_time(supla_input_cfg_t *input_cfg) {
	return CFG_BTN_PRESS_TIME;
}

LOCAL void
supla_esp_gpio_input_timer_cb(void *timer_arg) {

	supla_input_cfg_t *input_cfg = (supla_input_cfg_t *)timer_arg;
	uint8 v = gpio__input_get(input_cfg->gpio_id);
	uint8 active = (input_cfg->flags & INPUT_FLAG_PULLUP) ? 0 : 1;

	if ( input_cfg->step == 1 ) {

		if ( v == active ) {

			input_cfg->cycle_counter = 1;

			if ( input_cfg->flags & INPUT_FLAG_CFG_BTN
				 && ( input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE
					  || input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE_RS
				      || (system_get_time() - input_cfg->last_active) >= 2000000 ) ) {

				input_cfg->cfg_counter = 0;
				//supla_log(LOG_DEBUG, "RESET cfg_counter = 0");
			}

			input_cfg->step = 2;
			return;

		} else {

			if ( input_cfg->cycle_counter >= INPUT_MIN_CYCLE_COUNT ) {

				supla_esp_gpio_on_input_inactive(input_cfg);

			} else if ( input_cfg->cycle_counter < 255 ) {

				input_cfg->cycle_counter++;
				return;
			}
		}

	} else if ( input_cfg->step > 1 ) {

		if ( input_cfg->cycle_counter < 255 )
			input_cfg->cycle_counter++;

		if ( v == active ) {

			if ( input_cfg->step == 3
					&& input_cfg->flags & INPUT_FLAG_CFG_BTN
					&& ( input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE
						 || input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE_RS) ) {

				if ( input_cfg->cfg_counter < 2048 )
					input_cfg->cfg_counter++;

				if ( supla_esp_restart_on_cfg_press == 1 ) {
					supla_system_restart();
					return;
				}

				if ( (input_cfg->cfg_counter * INPUT_CYCLE_TIME) >= GET_CFG_PRESS_TIME(input_cfg) ) {

					// CFG MODE
					if ( supla_esp_cfgmode_started() == 0 ) {

						supla_esg_gpio_start_cfg_mode();

					} else if ( input_cfg->flags & INPUT_FLAG_FACTORY_RESET ) {

						factory_defaults(1);
						supla_log(LOG_DEBUG, "Factory defaults");
						os_delay_us(500000);
						supla_system_restart();
					}


					input_cfg->step = 0;
					os_timer_disarm(&input_cfg->timer);
					return;
				}

			}


			if ( input_cfg->step == 2
				 && input_cfg->cycle_counter >= INPUT_MIN_CYCLE_COUNT ) {


				supla_esp_gpio_on_input_active(input_cfg);


				if ( (input_cfg->flags & INPUT_FLAG_CFG_BTN) == 0
						|| input_cfg->type == INPUT_TYPE_BTN_BISTABLE ) {

					os_timer_disarm(&input_cfg->timer);

				}


				input_cfg->cycle_counter = 1;
				input_cfg->last_active = system_get_time();
				input_cfg->step = 3;

			}


			return;
		}


		if ( input_cfg->step == 3 ) {

			if ( input_cfg->cycle_counter >= INPUT_MIN_CYCLE_COUNT ) {

				if ( input_cfg->flags & INPUT_FLAG_CFG_BTN
						&& input_cfg->type == INPUT_TYPE_BTN_BISTABLE ) {

					input_cfg->cfg_counter++;

					//supla_log(LOG_DEBUG, "cfg_counter = %i", input_cfg->cfg_counter);

					if ( supla_esp_cfgmode_started() == 0 ) {

						if ( input_cfg->cfg_counter >= CFG_BTN_PRESS_COUNT ) {
							input_cfg->cfg_counter = 0;

							// CFG MODE
							supla_esg_gpio_start_cfg_mode();
						}

					} else if ( (system_get_time() - supla_esp_cfgmode_entertime) > 3000000 ) {

						//  EXIT CFG MODE
						supla_system_restart();

					}



				}

				if ( input_cfg->flags & INPUT_FLAG_CFG_BTN
						&& supla_esp_cfgmode_started() == 1
						&& ( input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE
							 || input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE_RS )
						&& (system_get_time() - supla_esp_cfgmode_entertime) > 3000000 ) {

					// EXIT CFG MODE
					supla_system_restart();

				} else {

					supla_esp_gpio_on_input_inactive(input_cfg);

				}


			} else {
				return;
			}

		}

	}


    input_cfg->step = 0;
    os_timer_disarm(&input_cfg->timer);

}

void
supla_esp_gpio_start_input_timer(supla_input_cfg_t *input_cfg) {
	os_timer_disarm(&input_cfg->timer);
	os_timer_setfn(&input_cfg->timer, supla_esp_gpio_input_timer_cb, input_cfg);
	os_timer_arm (&input_cfg->timer, INPUT_CYCLE_TIME, true);
}

LOCAL void
supla_esp_gpio_intr_handler(void *params) {


	int a;
	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	supla_input_cfg_t *input_cfg;

	#ifdef BOARD_INTR_HANDLER
	BOARD_INTR_HANDLER;
	#endif

	//supla_log(LOG_DEBUG, "INTR");

	for(a=0;a<INPUT_MAX_COUNT;a++) {

		input_cfg = &supla_input_cfg[a];

		if ( input_cfg->gpio_id != 255
			 && input_cfg->gpio_id < 16
			 && gpio_status & BIT(input_cfg->gpio_id) ) {

			ETS_GPIO_INTR_DISABLE();

            gpio_pin_intr_state_set(GPIO_ID_PIN(input_cfg->gpio_id), GPIO_PIN_INTR_DISABLE);
            GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(input_cfg->gpio_id)); // //clear interrupt status

            if ( input_cfg->type != INPUT_TYPE_CUSTOM &&
            		( input_cfg->step == 0
            		  || input_cfg->step == 3 ) ) {

            	if ( input_cfg->step == 0 )
                	input_cfg->step = 1;

            	supla_esp_gpio_start_input_timer(input_cfg);
            }

            gpio_pin_intr_state_set(GPIO_ID_PIN(input_cfg->gpio_id), GPIO_PIN_INTR_ANYEDGE);
            gpio_status = gpio_status ^ BIT(input_cfg->gpio_id);

            ETS_GPIO_INTR_ENABLE();
		}
	}


	// Disable uncaught interrupts
	if (gpio_status != 0) {
		for(a=0;a<16;a++) {
			if ( gpio_status & BIT(a) && INTR_CLEAR_MASK & BIT(a) ) {
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

	memset(&supla_input_cfg, 0, sizeof(supla_input_cfg));
	memset(&supla_relay_cfg, 0, sizeof(supla_relay_cfg));
	memset(&supla_rs_cfg, 0, sizeof(supla_rs_cfg));

	for (a=0; a<INPUT_MAX_COUNT; a++) {
		supla_input_cfg[a].gpio_id = 255;
		supla_input_cfg[a].relay_gpio_id = 255;
		supla_input_cfg[a].channel = 255;
		supla_input_cfg[a].last_state = 255;
	}

	for (a=0; a<RELAY_MAX_COUNT; a++) {
		supla_relay_cfg[a].gpio_id = 255;
		supla_relay_cfg[a].channel = 255;
	}


	for(a=0; a<RS_MAX_COUNT; a++) {
		supla_rs_cfg[a].position = &supla_esp_state.rs_position[a];
		supla_rs_cfg[a].full_opening_time = &supla_esp_cfg.Time1[a];
		supla_rs_cfg[a].full_closing_time = &supla_esp_cfg.Time2[a];
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
				gpio_pin_intr_state_set(GPIO_ID_PIN(supla_relay_cfg[a].gpio_id), GPIO_PIN_INTR_DISABLE);

			} else if (supla_relay_cfg[a].gpio_id == 16) {
				gpio16_output_conf();
			}

			supla_relay_cfg[a].last_time = 2147483647;

			if ( supla_relay_cfg[a].flags & RELAY_FLAG_RESTORE_FORCE ) {

				//supla_log(LOG_DEBUG, "RESTORE_FORCE, %i, %i, %i", a, supla_relay_cfg[a].gpio_id, supla_esp_state.Relay[a]);
				supla_esp_gpio_relay_hi(supla_relay_cfg[a].gpio_id, supla_esp_state.Relay[a], 255);

			} else if ( supla_relay_cfg[a].flags & RELAY_FLAG_RESTORE ) {

				struct rst_info *rtc_info = system_get_rst_info();

				if ( rtc_info->reason == 0 ) {
					   //supla_log(LOG_DEBUG, "RESTORE");
					   supla_esp_gpio_relay_hi(supla_relay_cfg[a].gpio_id, supla_esp_state.Relay[a], 255);
				}

			} else if ( supla_relay_cfg[a].flags & RELAY_FLAG_RESET ) {

				//supla_log(LOG_DEBUG, "LO_VALUE");
				supla_esp_gpio_relay_hi(supla_relay_cfg[a].gpio_id, LO_VALUE, 0);

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

}

void
supla_esp_gpio_set_hi(int port, char hi) {

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

void supla_esp_gpio_set_led(char r, char g, char b) {

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
void
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

void
supla_esp_gpio_led_blinking(int led, int time) {

	supla_esp_gpio_set_led(0, 0, 0);

	os_timer_setfn(&supla_gpio_timer1, supla_esp_gpio_led_blinking_func, (void*)led);
	os_timer_arm (&supla_gpio_timer1, time, true);

}
#endif

void GPIO_ICACHE_FLASH
supla_esp_gpio_state_disconnected(void) {

	if ( supla_last_state == STATE_DISCONNECTED )
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

	if ( supla_last_state == STATE_IPRECEIVED )
		return;

	supla_last_state = STATE_IPRECEIVED;

	supla_log(LOG_DEBUG, "IP Received");

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

	if ( supla_last_state == STATE_CONNECTED )
		return;

	supla_last_state = STATE_CONNECTED;

	supla_log(LOG_DEBUG, "Server connected");

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




