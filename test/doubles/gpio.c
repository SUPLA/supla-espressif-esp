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

#include "gpio.h"

uint32 gpio_input_get(void) { return 0; }

void gpio_output_set(uint32 set_mask, uint32 clear_mask, uint32 enable_mask,
                     uint32 disable_mask) {
  return;
}

void gpio_register_set(uint32 reg_id, uint32 value) {}

void gpio_pin_intr_state_set(uint32 i, GPIO_INT_TYPE intr_state) {}
