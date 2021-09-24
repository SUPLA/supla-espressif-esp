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

#include "supla_esp_mock.h"

extern "C" {
#include <supla_esp.h>

void supla_system_restart(void) {
  assert(BoardInterface::instance);
  return BoardInterface::instance->supla_system_restart();
}

void supla_system_restart_with_delay(uint32 delay_ms) {
  assert(BoardInterface::instance);
  return BoardInterface::instance->supla_system_restart_with_delay(delay_ms);
}

void factory_reset_mock() {
  assert(BoardInterface::instance);
  return BoardInterface::instance->factory_reset();
}
}

BoardInterface *BoardInterface::instance = nullptr;

BoardInterface::BoardInterface() { instance = this; }

BoardInterface::~BoardInterface() { instance = nullptr; }
