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

#include "time_interface.h"

extern "C" {
  uint32 system_get_time() {
    assert(TimeInterface::instance);
    return TimeInterface::instance->system_get_time();
  }

}


TimeInterface *TimeInterface::instance = nullptr;

TimeInterface::TimeInterface() { instance = this; }

TimeInterface::~TimeInterface() { instance = nullptr; }

