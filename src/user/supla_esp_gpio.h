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


#ifndef SUPLA_ESP_GPIO_H_
#define SUPLA_ESP_GPIO_H_

#include "supla_esp.h"

#define RELAY_FLAG_RESET              0x01
#define RELAY_FLAG_RESTORE            0x02
#define RELAY_FLAG_RESTORE_FORCE      0x04
#define RELAY_FLAG_IRQ_LOCKED         0x08
#define RELAY_FLAG_LO_LEVEL_TRIGGER   0x10
#define RELAY_FLAG_VIRTUAL_GPIO       0x20

#define RS_RELAY_OFF   0
#define RS_RELAY_UP    2
#define RS_RELAY_DOWN  1

typedef struct {

	uint8 gpio_id;
	uint8 flags;
	uint8 type;
	uint8 step;
	uint8 cycle_counter;
	uint16 cfg_counter;
	uint8 relay_gpio_id;
	uint8 channel;
	uint8 last_state;

	ETSTimer timer;
	unsigned int last_active;

}supla_input_cfg_t;


typedef struct {

	uint8 gpio_id;
	uint8 flags;
	uint8 channel;
	unsigned _supla_int_t channel_flags;

	unsigned int last_time;

}supla_relay_cfg_t;

typedef struct {

	uint8 percent;
	uint8 direction;
	uint8 active;

}rs_task_t;

typedef struct {

	ETSTimer timer;
	uint8 value;

}supla_rs_delayed_trigger;

typedef struct {

	supla_relay_cfg_t *up;
	supla_relay_cfg_t *down;

	int *position;

  unsigned int *full_opening_time;
  unsigned int *full_closing_time;

	unsigned int last_comm_time;

	unsigned int up_time;
	unsigned int down_time;
	unsigned int last_time;
	unsigned int stop_time;
	unsigned int start_time;

	ETSTimer timer;
	supla_rs_delayed_trigger delayed_trigger;
	rs_task_t task;

	int last_position;

}supla_roller_shutter_cfg_t;

extern supla_input_cfg_t supla_input_cfg[INPUT_MAX_COUNT];
extern supla_relay_cfg_t supla_relay_cfg[RELAY_MAX_COUNT];
extern supla_roller_shutter_cfg_t supla_rs_cfg[RS_MAX_COUNT];
extern unsigned int supla_esp_gpio_init_time;
extern unsigned char supla_esp_restart_on_cfg_press;

void gpio16_output_conf(void);
void gpio16_output_set(uint8 value);
void gpio16_input_conf(void);
uint8 gpio16_input_get(void);
uint8 gpio16_output_get(void);
uint8 gpio__input_get(uint8 port);

void GPIO_ICACHE_FLASH supla_esp_gpio_init(void);

void GPIO_ICACHE_FLASH supla_esp_gpio_state_disconnected(void);
void GPIO_ICACHE_FLASH supla_esp_gpio_state_ipreceived(void);
void GPIO_ICACHE_FLASH supla_esp_gpio_state_connected(void);
void GPIO_ICACHE_FLASH supla_esp_gpio_state_cfgmode(void);

#ifdef __FOTA
void GPIO_ICACHE_FLASH supla_esp_gpio_state_update(void);
#endif

void supla_esp_gpio_start_input_timer(supla_input_cfg_t *input_cfg);

void supla_esp_gpio_set_hi(int port, char hi);
char supla_esp_gpio_output_is_hi(int port);
char supla_esp_gpio_relay_is_hi(int port);
char __supla_esp_gpio_relay_is_hi(supla_relay_cfg_t *relay_cfg);
char supla_esp_gpio_relay_hi(int port, char hi, char save_before);
char supla_esp_gpio_relay_on(int port);

void supla_esp_gpio_set_led(char r, char g, char b);
void supla_esp_gpio_led_blinking(int led, int time);

#ifdef _ROLLERSHUTTER_SUPPORT
void supla_esp_gpio_rs_set_relay(supla_roller_shutter_cfg_t *rs_cfg,
                                 uint8 value, uint8 cancel_task,
                                 uint8 stop_delay);
uint8 GPIO_ICACHE_FLASH
supla_esp_gpio_rs_get_value(supla_roller_shutter_cfg_t *rs_cfg);
void GPIO_ICACHE_FLASH
supla_esp_gpio_rs_cancel_task(supla_roller_shutter_cfg_t *rs_cfg);
void GPIO_ICACHE_FLASH supla_esp_gpio_rs_add_task(int idx, uint8 percent);

supla_roller_shutter_cfg_t *supla_esp_gpio_get_rs__cfg(int port);
#endif /*_ROLLERSHUTTER_SUPPORT*/

#endif /* SUPLA_ESP_GPIO_H_ */

