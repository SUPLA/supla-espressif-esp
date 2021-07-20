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

#include "spi_stub.h"

void SpiStub::spi_init(void){};
void SpiStub::spi_begin(void){};
void SpiStub::spi_end(void){};
uint8 SpiStub::spi_send_recv(uint8 data) { return 0; };
uint32 SpiStub::spi_transaction_read(uint32 addr) { return 0; };
uint32 SpiStub::spi_transaction_write(uint32 addr, uint32 writeData) { return 0; };
