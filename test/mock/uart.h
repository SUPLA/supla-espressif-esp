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

#ifndef MOCK_UART_H_
#define MOCK_UART_H_

#include <c_types.h>

void rx_buff_set_ptr(char* pdata, uint16 size);
void tx_buff_set_ptr(char* pdata, uint16 size);
uint16 rx_buff_deq(char* pdata, uint16 data_len);
void tx_buff_enq(char* pdata, uint16 data_len);

#endif /*MOCK_UART_H_*/
