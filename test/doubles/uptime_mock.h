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

#ifndef MOCK_UPTIME_MOCK_H_
#define MOCK_UPTIME_MOCK_H_

#include "uptime_interface.h"
#include <c_types.h>
#include <gmock/gmock.h>
#include <proto.h>

class UptimeMock : public UptimeInterface {
public:
  MOCK_METHOD(unsigned _supla_int64_t, uptime_usec, (), (override));
  MOCK_METHOD(unsigned _supla_int64_t, uptime_msec, (), (override));
  MOCK_METHOD(uint32_t, uptime_sec, (), (override));
};

#endif
