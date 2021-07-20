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

#ifndef _supla_spi_mock_h
#define _supla_spi_mock_h

#include "spi_interface.h"
#include <gmock/gmock.h>

class SpiMock : public SpiInterface {
public:
  MOCK_METHOD(void, spi_init, (), (override));
  MOCK_METHOD(uint8, spi_send_recv, (uint8), (override));
  MOCK_METHOD(uint32, spi_transaction_read, (uint32), (override));
  MOCK_METHOD(uint32, spi_transaction_write, (uint32, uint32), (override));

  // begin and end methods are not mocked - there will be failed assertion when
  // spi_send_recv method is called without propoer begin/end sequnce
  void spi_begin();
  void spi_end();
};

#endif
