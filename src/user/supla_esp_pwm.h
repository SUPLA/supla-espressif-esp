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

#include "supla_esp.h"

//#ifdef SUPLA_PWM_COUNT

#ifndef SUPLA_ESP_PWM_H_
#define SUPLA_ESP_PWM_H_

#include <eagle_soc.h>
#include <os_type.h>


void supla_esp_pwm_init(void);
void supla_esp_pwm_set_percent_duty(uint8 percent, uint8 percent_percent, uint8 channel);
char ICACHE_FLASH_ATTR supla_esp_pwm_is_on(void);
void ICACHE_FLASH_ATTR supla_esp_pwm_on(char on);

#endif /* SUPLA_PWM_COUNT */

//#endif /* SUPLA_PWM_H_ */
