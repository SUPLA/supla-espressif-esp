/*
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

#ifndef SUPLA_DHTSENSOR_H_
#define SUPLA_DHTSENSOR_H_

#include "supla_esp.h"
#include "supla-dev/proto.h"

#ifdef DHTSENSOR

extern ETSTimer supla_dht_timer1;

void DHT_ICACHE_FLASH supla_dht_init(void);
void DHT_ICACHE_FLASH supla_get_temp_and_humidity(char value[SUPLA_CHANNELVALUE_SIZE]);
void DHT_ICACHE_FLASH supla_dht_start(void);

#endif



#endif
