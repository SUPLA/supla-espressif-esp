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


#include <os_type.h>
#include <osapi.h>
#include <eagle_soc.h>
#include <gpio.h>

#include "supla_w1.h"

#ifdef W1_GPIO0
  int supla_w1_pin = GPIO_ID_PIN(0);
#elif defined(W1_GPIO4)
  int supla_w1_pin = GPIO_ID_PIN(4);
#elif defined(W1_GPIO5)
  int supla_w1_pin = GPIO_ID_PIN(5);
#elif defined(W1_GPIO3)
  int supla_w1_pin = GPIO_ID_PIN(3);
#elif defined(W1_GPIO14)
  int supla_w1_pin = GPIO_ID_PIN(14);
#else
  int supla_w1_pin = GPIO_ID_PIN(2);
#endif
  
void ICACHE_FLASH_ATTR supla_w1_init(void) {

	#ifdef W1_GPIO0
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
		PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);
    	#elif defined(W1_GPIO4)
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
		PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO4_U);
    	#elif defined(W1_GPIO5)
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
		PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);
	#elif defined(W1_GPIO3)

	    system_uart_swap ();

		PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3);
		PIN_PULLUP_EN(PERIPHS_IO_MUX_U0RXD_U);
	#elif defined(W1_GPIO14)
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
		PIN_PULLUP_EN(PERIPHS_IO_MUX_MTMS_U);
    #else
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
		PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO2_U);
    #endif
	
    GPIO_DIS_OUTPUT( supla_w1_pin );
}
