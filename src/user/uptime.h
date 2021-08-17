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

#ifndef _SUPLA_UPTIME_H_
#define _SUPLA_UPTIME_H_

#include <supla_esp.h>
#include <c_types.h>
#include <supla-dev/proto.h>

void MAIN_ICACHE_FLASH supla_esp_uptime_init(void);

unsigned _supla_int64_t MAIN_ICACHE_FLASH uptime_usec(void);

unsigned _supla_int64_t MAIN_ICACHE_FLASH uptime_msec(void);

uint32 MAIN_ICACHE_FLASH uptime_sec(void);

#endif /*_SUPLA_UPTIME_H_*/
