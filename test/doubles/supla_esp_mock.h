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

#ifndef _TEST_SUPLA_ESP_MOCK_H
#define _TEST_SUPLA_ESP_MOCK_H

#include <c_types.h>
#include <gmock/gmock.h>

extern "C" {
#include <supla_esp.h>
};

class BoardInterface {
public:
  BoardInterface();
  virtual ~BoardInterface();

  virtual void supla_system_restart() = 0;
  virtual void supla_system_restart_with_delay(uint32 delayMs) = 0;
  virtual void factory_reset() = 0;
  static BoardInterface *instance;
};

class BoardMock : public BoardInterface {
public:
  MOCK_METHOD(void, supla_system_restart, (), (override));
  MOCK_METHOD(void, supla_system_restart_with_delay, (uint32 delayMs),
      (override));
  MOCK_METHOD(void, factory_reset, (), (override));
};

#endif /*_TEST_SUPLA_ESP_MOCK_H*/


