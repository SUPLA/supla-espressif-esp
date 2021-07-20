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

#include "eagle_soc_intf.h"

extern "C" {
uint32 GPIO_REG_READ(uint8 reg) {
  assert(EagleSocInterface::instance);
  return EagleSocInterface::instance->gpioRegRead(reg);
}

void GPIO_OUTPUT_SET(uint32 port, uint8 value) {
  assert(value == 0 || value == 1);
  assert(port >= 0 && port <= 15);
  assert(EagleSocInterface::instance);
  return EagleSocInterface::instance->gpioOutputSet(port, value);
}

void GPIO_REG_WRITE(uint8 reg, uint8 val){};
}

EagleSocInterface *EagleSocInterface::instance = nullptr;

EagleSocInterface::EagleSocInterface() { instance = this; }

EagleSocInterface::~EagleSocInterface() { instance = nullptr; }
