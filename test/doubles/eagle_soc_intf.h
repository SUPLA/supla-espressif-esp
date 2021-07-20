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

#ifndef _SUPLA_TEST_EAGLE_SOC_INTF_H
#define _SUPLA_TEST_EAGLE_SOC_INTF_H

#include <c_types.h>
#include <gmock/gmock.h>

extern "C" {
uint32 GPIO_REG_READ(uint8 reg);
void GPIO_REG_WRITE(uint8 reg, uint8 val);
}

class EagleSocInterface {
public:
  EagleSocInterface();
  virtual ~EagleSocInterface();

  virtual uint32 gpioRegRead(uint8 reg) = 0;
  virtual void gpioOutputSet(uint32 port, uint8 value) = 0;
  static EagleSocInterface *instance;
};

#endif /*_SUPLA_TEST_EAGLE_SOC_INTF_H*/
