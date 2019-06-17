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

#include "supla_esp_pwm.h"

#include "supla_esp_gpio.h"

#ifdef SUPLA_PWM_COUNT

#include "supla-dev/log.h"

#include <ets_sys.h>
#include <eagle_soc.h>
#include <osapi.h>
#include <pwm.h>



void ICACHE_FLASH_ATTR supla_esp_pwm_init(void) {
	
    uint32 io_info[][3] =
     {
		 {PWM_0_OUT_IO_MUX,PWM_0_OUT_IO_FUNC,PWM_0_OUT_IO_NUM},
		 {PWM_1_OUT_IO_MUX,PWM_1_OUT_IO_FUNC,PWM_1_OUT_IO_NUM},
		 {PWM_2_OUT_IO_MUX,PWM_2_OUT_IO_FUNC,PWM_2_OUT_IO_NUM},
		 {PWM_3_OUT_IO_MUX,PWM_3_OUT_IO_FUNC,PWM_3_OUT_IO_NUM},
		 {PWM_4_OUT_IO_MUX,PWM_4_OUT_IO_FUNC,PWM_4_OUT_IO_NUM}
     };

    uint32 duty = 0;

    pwm_init(PWM_PERIOD, &duty, SUPLA_PWM_COUNT, io_info);
    os_delay_us(1000);
    pwm_start();
    
    int a;
    for(a=0;a<SUPLA_PWM_COUNT;a++)
    	supla_esp_pwm_set_percent_duty(0, 0, a);
    
    supla_esp_board_pwm_init();

}

void ICACHE_FLASH_ATTR supla_esp_pwm_set_percent_duty(uint8 percent, uint8 percent_percent, uint8 channel) {
	
	if ( percent > 100 )
		percent = 100;
	
	if ( percent_percent > 100 )
		percent_percent = 100;
	
	
	uint32 duty = ((PWM_PERIOD * 1000 / 45) * percent) / 100;
	duty = (duty * percent_percent) / 100;
	
	supla_log(LOG_DEBUG, "DUTY: %i, CHANNEL: %i", duty, channel);

	pwm_set_duty(duty, channel);
	os_delay_us(1000);
	pwm_start();

}

char ICACHE_FLASH_ATTR supla_esp_pwm_is_on(void) {
	
	int a;
	
	for(a=0;a<SUPLA_PWM_COUNT;a++)
		if ( pwm_get_duty(a) > 0 )
			return 1;
		
	return 0;
}

void ICACHE_FLASH_ATTR supla_esp_pwm_on(char on) {
	
	int a;
	
	for(a=0;a<SUPLA_PWM_COUNT;a++)
		supla_esp_pwm_set_percent_duty(on == 1 ? 100 : 0, 100, a);
	
}

     
#endif
