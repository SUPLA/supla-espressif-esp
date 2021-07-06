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

#ifndef _supla_spi_interface_h
#define _supla_spi_interface_h

#include <c_types.h>

class SpiInterface {
public:
  SpiInterface();
  virtual ~SpiInterface();
  virtual void spi_init(void) = 0;
  virtual void spi_begin(void) = 0;
  virtual void spi_end(void) = 0;
  virtual uint8 spi_send_recv(uint8 data) = 0;
  virtual uint32 spi_transaction_write(uint32 addr, uint32 writeData) = 0;
  virtual uint32 spi_transaction_read(uint32 addr) = 0;

  static SpiInterface *instance;
};

#endif
