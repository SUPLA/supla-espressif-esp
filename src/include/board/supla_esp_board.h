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

#include <os_type.h>
#include "supla_esp.h"

#ifndef SUPLA_ESP_BOARD_H_
#define SUPLA_ESP_BOARD_H_

#if defined(__BOARD_wifisocket) || defined(__BOARD_wifisocket_x4) || \
    defined(__BOARD_wifisocket_54)

#include "board/wifisocket.h"

#elif defined(__BOARD_gate_module) || defined(__BOARD_gate_module_dht11) || \
    defined(__BOARD_gate_module_dht22)

#include "board/gate_module.h"

#elif defined(__BOARD_gate_module_wroom) || defined(__BOARD_gate_module2_wroom)

#include "board/gate_module_wroom.h"

#elif defined(__BOARD_rs_module) || defined(__BOARD_rs_module_wroom)

#include "board/rs_module.h"

#elif defined(__BOARD_sonoff) || defined(__BOARD_sonoff_ds18b20)

#include "board/sonoff.h"

#elif defined(__BOARD_sonoff_touch) || defined(__BOARD_sonoff_touch_dual)

#include "board/sonoff_touch.h"

#elif defined(__BOARD_sonoff_dual)

#include "board/sonoff_dual.h"

#elif defined(__BOARD_sonoff_socket)

#include "board/sonoff_socket.h"

#elif defined(__BOARD_sonoff_th10) || defined(__BOARD_sonoff_th16)

#include "board/sonoff_th.h"

#elif defined(__BOARD_dimmer)

#include "board/dimmer.h"

#elif defined(__BOARD_h801)

#include "board/h801.h"

#elif defined(__BOARD_lightswitch_x2) ||        \
    defined(__BOARD_lightswitch_x2_DHT11) ||    \
    defined(__BOARD_lightswitch_x2_DHT22) ||    \
    defined(__BOARD_lightswitch_x2_54) ||       \
    defined(__BOARD_lightswitch_x2_54_DHT11) || \
    defined(__BOARD_lightswitch_x2_54_DHT22)

#include "board/lightswitch.h"

#elif defined(__BOARD_impulse_counter)

#include "board/impulse_counter.h"

#elif defined(__BOARD_inCan_DS) || defined(__BOARD_inCan_DHT11) || \
    defined(__BOARD_inCan_DHT22) || defined(__BOARD_inCanRS_DS) || \
    defined(__BOARD_inCanRS_DHT11) || defined(__BOARD_inCanRS_DHT22)

#include "board/inCan.h"

// Moje Plytki

#elif defined(__BOARD_k_gate_module) // nice

#include "board/k_gate_module.h"

#elif defined(__BOARD_k_dimmer)

#include "board/k_dimmer.h"

#elif defined(__BOARD_k_gniazdko_neo)

#include "board/k_gniazdko_neo.h"

#elif defined(__BOARD_k_impulse_counter)

#include "board/k_impulse_counter.h"

#elif defined(__BOARD_k_impulse_counter_3)

#include "board/k_impulse_counter_3.h"

#elif defined(__BOARD_k_rs_module) \
      || defined(__BOARD_k_rs_module_ds18b20) \
      || defined(__BOARD_k_rs_module_DHT22)
	  
#include "board/k_rs_module.h"

#elif defined(__BOARD_k_smoke_module) \
      || defined(__BOARD_k_smoke_module_ds18b20)	\
      || defined(__BOARD_k_smoke_module_DHT22)

#include "board/k_smoke_module.h"

#elif defined(__BOARD_k_socket) \
      || defined(__BOARD_k_socket_ds18b20)	\
      || defined(__BOARD_k_socket_DHT22)

#include "board/k_socket.h"

#elif defined(__BOARD_k_socket_dual) \
      || defined(__BOARD_k_socket_dual_ds18b20)	\
      || defined(__BOARD_k_socket_dual_DHT22)

#include "board/k_socket_dual.h"

#elif defined(__BOARD_k_sonoff) \
      || defined(__BOARD_k_sonoff_ds18b20)	\
      || defined(__BOARD_k_sonoff_DHT22)

#include "board/k_sonoff.h"

#elif defined(__BOARD_k_sonoff_touch)  \
      ||  defined(__BOARD_k_sonoff_touch_dual)  \
      ||  defined(__BOARD_k_sonoff_touch_triple)  \

#include "board/k_sonoff_touch.h"

#elif defined(__BOARD_k_yunschan)

#include "board/k_yunschan.h"

#elif defined(__BOARD_k_socket_01)

#include "board/k_socket_01.h"

#elif defined(__BOARD_k_sonoff_pow_R2)

#include "board/k_sonoff_pow_R2.h"

#endif

#endif
