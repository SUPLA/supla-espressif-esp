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
#include "supla_esp_input.h"

#define RELAY_FLAG_RESET              0x01
#define RELAY_FLAG_RESTORE            0x02
#define RELAY_FLAG_RESTORE_FORCE      0x04
#define RELAY_FLAG_IRQ_LOCKED         0x08
#define RELAY_FLAG_LO_LEVEL_TRIGGER   0x10
#define RELAY_FLAG_VIRTUAL_GPIO       0x20

typedef struct {
  uint8 gpio_id;
  uint8 flags;  // RELAY_FLAG_*
  uint8 channel;
  unsigned _supla_int_t channel_flags; // SUPLA_CHANNEL_FLAG_*
} supla_relay_cfg_t;

extern supla_relay_cfg_t supla_relay_cfg[RELAY_MAX_COUNT];
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

// when this method is called, it will execute default action configured for
// this input when its state changes to "active", i.e. button is pressed.
// (i.e. it will toggle relay state on button press)
void GPIO_ICACHE_FLASH
supla_esp_gpio_on_input_active(supla_input_cfg_t *input_cfg);

// when this method is called, it will execute default action configured for
// this input when its state changes to "inactive", i.e. button is released.
// (i.e. it will toggle relay state on button release)
void GPIO_ICACHE_FLASH
supla_esp_gpio_on_input_inactive(supla_input_cfg_t *input_cfg);

void supla_esp_gpio_set_hi(int port, unsigned char hi);
char supla_esp_gpio_output_is_hi(int port);
char supla_esp_gpio_relay_is_hi(int port);
char __supla_esp_gpio_relay_is_hi(supla_relay_cfg_t *relay_cfg);
char supla_esp_gpio_relay_hi(int port, unsigned char hi, char save_before);
char supla_esp_gpio_relay_on(int port);

// use this method to control relay from local buttons etc.
// It will send updated channel value and manage countdown timers
void GPIO_ICACHE_FLASH supla_esp_gpio_relay_switch(int port,
    unsigned char hi);

void supla_esp_gpio_set_led(char r, char g, char b);
void supla_esp_gpio_led_blinking(int led, int time);

void GPIO_ICACHE_FLASH supla_esp_gpio_relay_set_duration_timer(int channel,
    int newValue, int durationMs, int senderID);
#endif /* SUPLA_ESP_GPIO_H_ */

#ifdef BOARD_ESP_ON_STATE_CHANGED
void GPIO_ICACHE_FLASH supla_esp_board_on_state_changed(char supla_last_state);
#endif

// method used only in UT tests
void GPIO_ICACHE_FLASH supla_esp_gpio_clear_vars(void);
