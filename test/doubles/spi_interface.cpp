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

#include <c_types.h>
#include <cassert>

#include "spi_interface.h"

bool spiStateActive = false;

SpiInterface::SpiInterface() { instance = this; }

SpiInterface::~SpiInterface() {
  assert(!spiStateActive);
  instance = nullptr;
}

SpiInterface *SpiInterface::instance = nullptr;

extern "C" {
void spi_init(void) {
  assert(spiStateActive == false);
  assert(SpiInterface::instance);
  SpiInterface::instance->spi_init();
};

void spi_begin(void) {
  assert(spiStateActive == false);
  spiStateActive = true;
  assert(SpiInterface::instance);
  SpiInterface::instance->spi_begin();
};

void spi_end(void) {
  assert(spiStateActive);
  spiStateActive = false;
  assert(SpiInterface::instance);
  SpiInterface::instance->spi_end();
};

uint8 spi_send_recv(uint8 data) {
  // send_recv should be called only between begin() and end() calls
  assert(spiStateActive);
  assert(SpiInterface::instance);
  return SpiInterface::instance->spi_send_recv(data);
};

#define SPI_CMD_READ 1
#define SPI_CMD_WRITE 0

uint32 spi_transaction(uint8 spi_no, uint8 spi_chip, uint8 cmd_bits,
                       uint16 cmd_data, uint32 addr_bits, uint32 addr_data,
                       uint32 dout_bits, uint32 dout_data, uint32 din_bits,
                       uint32 dummy_bits) {
  assert(SpiInterface::instance);
  assert(spi_no == 1);  // currently used in tests with HSPI only. Adjust if
                        // something else is needed
  assert(spi_chip == 0);
  assert(cmd_bits == 1);
  assert(addr_bits == 7);

  assert(dummy_bits == 0);

  if (cmd_data == SPI_CMD_READ) {
    assert(dout_bits == 0);
    assert(din_bits == 16);
    assert(dout_data == 0);
    return SpiInterface::instance->spi_transaction_read(addr_data);
  } else if (cmd_data == SPI_CMD_WRITE) {
    assert(dout_bits == 16);
    assert(din_bits == 0);
    return SpiInterface::instance->spi_transaction_write(addr_data, dout_data);
  }
  assert(false && "only write and read command are supported");
  return 0;
}
}
