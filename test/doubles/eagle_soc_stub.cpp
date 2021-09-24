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

#include "eagle_soc_stub.h"

EagleSocStub::EagleSocStub() : currentGpioState(0), intrStatus(0) {}

uint32 EagleSocStub::gpioRegRead(uint32 reg) {
  if (reg == GPIO_OUT_ADDRESS) {
    return currentGpioState;
  } else if (reg == GPIO_STATUS_ADDRESS) {
    return intrStatus;
  }
  assert(false);
  return 0;
}

void EagleSocStub::gpioRegWrite(uint32 reg, uint32 value) {
  if (reg == GPIO_STATUS_W1TC_ADDRESS) {
    intrStatus &= ~value;
  }
}

void EagleSocStub::gpioOutputSet(uint32 port, uint8 value) {
  intrStatus |= (1 << port); // for any_edge interrupt
  if (value) {
    currentGpioState |= (1 << port);
  } else {
    currentGpioState &= ~(1 << port);
  }
}

bool EagleSocStub::getGpioValue(uint8_t port) {
  return (currentGpioState & (1 << port));

}



