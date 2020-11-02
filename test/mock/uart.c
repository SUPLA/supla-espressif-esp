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

#include "uart.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
  uint16 pos;
  char *pdata;
  uint16 size;
} _uart_mock_buff;

_uart_mock_buff uart_mock_buff;

void rx_buff_set_ptr(char *pdata, uint16 size) {
  uart_mock_buff.pos = 0;
  uart_mock_buff.pdata = pdata;
  uart_mock_buff.size = size;
}

uint16 rx_buff_deq(char *pdata, uint16 data_len) {
  if (uart_mock_buff.size - uart_mock_buff.pos < data_len) {
    data_len = uart_mock_buff.size - uart_mock_buff.pos;
  }

  if (data_len > 0) {
    memcpy(pdata, &uart_mock_buff.pdata[uart_mock_buff.pos], data_len);
    uart_mock_buff.pos += data_len;
  }

  return data_len;
}
