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

#include "supla_esp.h"

#if defined(__BOARD_wifisocket) || defined(__BOARD_wifisocket_x4) || \
    defined(__BOARD_wifisocket_54)

#include "board/wifisocket.c"

#elif defined(__BOARD_gate_module) || defined(__BOARD_gate_module_dht11) || \
    defined(__BOARD_gate_module_dht22)

#include "board/gate_module.c"

#elif defined(__BOARD_gate_module_wroom) || defined(__BOARD_gate_module2_wroom)

#include "board/gate_module_wroom.c"

#elif defined(__BOARD_rs_module) || defined(__BOARD_rs_module_wroom)

#include "board/rs_module.c"

#elif defined(__BOARD_sonoff) || defined(__BOARD_sonoff_ds18b20)

#include "board/sonoff.c"

#elif defined(__BOARD_sonoff_touch) || defined(__BOARD_sonoff_touch_dual)

#include "board/sonoff_touch.c"

#elif defined(__BOARD_sonoff_dual)

#include "board/sonoff_dual.c"

#elif defined(__BOARD_sonoff_socket)

#include "board/sonoff_socket.c"

#elif defined(__BOARD_sonoff_th10) || defined(__BOARD_sonoff_th16)

#include "board/sonoff_th.c"

#elif defined(__BOARD_dimmer)

#include "board/dimmer.c"

#elif defined(__BOARD_h801)

#include "board/h801.c"

#elif defined(__BOARD_lightswitch_x2) ||        \
    defined(__BOARD_lightswitch_x2_DHT11) ||    \
    defined(__BOARD_lightswitch_x2_DHT22) ||    \
    defined(__BOARD_lightswitch_x2_54) ||       \
    defined(__BOARD_lightswitch_x2_54_DHT11) || \
    defined(__BOARD_lightswitch_x2_54_DHT22)

#include "board/lightswitch.c"

#elif defined(__BOARD_lightswitch_at)

#include "board/lightswitch_at.c"

#elif defined(__BOARD_impulse_counter)

#include "board/impulse_counter.c"

#elif defined(__BOARD_inCan_DS) || defined(__BOARD_inCan_DHT11) || \
    defined(__BOARD_inCan_DHT22) || defined(__BOARD_inCanRS_DS) || \
    defined(__BOARD_inCanRS_DHT11) || defined(__BOARD_inCanRS_DHT22)

#include "board/inCan.c"

#endif

#ifdef SUPLA_ESP_BOARD_SUBDEF
#include "board/supla_esp_board_subdef.c"
#endif
