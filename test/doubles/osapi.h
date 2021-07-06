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

#ifndef SUPLA_OSAPI_STUB_H
#define SUPLA_OSAPI_STUB_H

#include "c_types.h"
#include "ets_sys.h"
#include <stdarg.h>
#include <stdio.h>

typedef ETSTimer os_timer_t;
typedef void (*os_timer_func_t)(void *);

void os_timer_arm(os_timer_t *ptimer, uint32_t time, bool repeat_flag);
void os_timer_disarm(os_timer_t *ptimer);
void os_timer_setfn(os_timer_t *ptimer, os_timer_func_t *pfunction, void *parg);

void os_delay_us(uint32_t us);

#define ets_snprintf snprintf

#endif
