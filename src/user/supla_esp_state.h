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

#ifndef SUPLA_ESP_STATE_H_
#define SUPLA_ESP_STATE_H_

#include "supla_esp.h"

void ICACHE_FLASH_ATTR supla_esp_set_state(int __pri, const char *message);
const char *ICACHE_FLASH_ATTR supla_esp_get_laststate(void);

#endif /* SUPLA_ESP_STATE_H_ */
