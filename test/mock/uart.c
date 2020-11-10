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

_uart_mock_buff uart_mock_rx_buff = {.pos = 0, .pdata = 0, .size = 0};
_uart_mock_buff uart_mock_tx_buff = {.pos = 0, .pdata = 0, .size = 0};

void uart_mock_buff_set_ptr(_uart_mock_buff *buf, char *pdata, uint16 size) {
  buf->pos = 0;
  buf->pdata = pdata;
  buf->size = size;
}

void tx_buff_set_ptr(char *pdata, uint16 size, uint16 **pos) {
  uart_mock_buff_set_ptr(&uart_mock_tx_buff, pdata, size);
  if (pos) {
    *pos = &uart_mock_tx_buff.pos;
  }
}

void rx_buff_set_ptr(char *pdata, uint16 size) {
  uart_mock_buff_set_ptr(&uart_mock_rx_buff, pdata, size);
}

uint16 rx_buff_deq(char *pdata, uint16 data_len) {
  if (uart_mock_rx_buff.size - uart_mock_rx_buff.pos < data_len) {
    data_len = uart_mock_rx_buff.size - uart_mock_rx_buff.pos;
  }

  if (data_len > 0) {
    memcpy(pdata, &uart_mock_rx_buff.pdata[uart_mock_rx_buff.pos], data_len);
    uart_mock_rx_buff.pos += data_len;
  }

  return data_len;
}

void tx_buff_enq(char *pdata, uint16 data_len) {
  if (uart_mock_tx_buff.size - uart_mock_tx_buff.pos < data_len) {
    data_len = uart_mock_tx_buff.size - uart_mock_tx_buff.pos;
  }

  if (data_len > 0) {
    memcpy(&uart_mock_tx_buff.pdata[uart_mock_tx_buff.pos], pdata, data_len);
    uart_mock_tx_buff.pos += data_len;
  }
}
