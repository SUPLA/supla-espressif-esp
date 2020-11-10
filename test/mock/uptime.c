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

#include "uptime.h"
#include <sys/time.h>
#include <string.h>

unsigned _supla_int64_t uptime_start_usec = 0;

unsigned _supla_int64_t uptime_usec(void) {
  struct timeval now;
  gettimeofday(&now, NULL);

  if (uptime_start_usec == 0) {
    uptime_start_usec = now.tv_sec * 1000000 + now.tv_usec - 1000000;
  }

  return now.tv_sec * 1000000 + now.tv_usec - uptime_start_usec;
}

unsigned _supla_int64_t uptime_msec(void) {
  return uptime_usec() / (unsigned _supla_int64_t)1000;
}

uint32 uptime_sec(void) {
  return uptime_msec() / (unsigned _supla_int64_t)1000;
}
