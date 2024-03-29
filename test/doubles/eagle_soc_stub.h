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

#ifndef SUPLA_EAGLE_SOC_STUB_H_
#define SUPLA_EAGLE_SOC_STUB_H_

#include "eagle_soc_intf.h"
#include <c_types.h>

class EagleSocStub : public EagleSocInterface {
public:
  EagleSocStub();

  uint32 gpioRegRead(uint32 reg) override;
  void gpioRegWrite(uint32 reg, uint32 value) override;
  void gpioOutputSet(uint32 port, uint8 value) override;

  bool getGpioValue(uint8_t port);

  uint32 currentGpioState;
  uint32 intrStatus;
};

#endif /*SUPLA_EAGLE_SOC_STUB_H_*/

