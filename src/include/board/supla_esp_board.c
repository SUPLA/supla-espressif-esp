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

#if defined(__BOARD_wifisocket)  \
      || defined(__BOARD_wifisocket_x4) \
      || defined(__BOARD_wifisocket_54)

#include "board/wifisocket.c"

#elif defined(__BOARD_gate_module) \
      || defined(__BOARD_gate_module_dht11)    \
      || defined(__BOARD_gate_module_dht22)    \

#include "board/gate_module.c"

#elif defined(__BOARD_gate_module_wroom)    \
      || defined(__BOARD_gate_module2_wroom)

#include "board/gate_module_wroom.c"

#elif defined(__BOARD_sonoff_dual)

#include "board/sonoff_dual.c"

#elif defined(__BOARD_sonoff_socket)

#include "board/sonoff_socket.c"

#elif defined(__BOARD_yunschan)

#include "board/yunschan.c"

#elif defined(__BOARD_sonoff_th10) \
	  || defined(__BOARD_sonoff_th16)

#include "board/sonoff_th.c"

#elif defined(__BOARD_dimmer)

#include "board/dimmer.c"

#elif defined(__BOARD_dimmer_socket)

#include "board/dimmer_socket.c"

#elif defined(__BOARD_vl_dimmer)

#include "board/acs_vl_dimmer.c"

#elif defined(__BOARD_EgyIOT)

#elif defined(__BOARD_zam_row_01)

#include "board/acs_zam_row_01.c"

#elif defined(__BOARD_zam_row_01_tester)

#include "board/acs_zam_row_01_tester.c"

#elif defined(__BOARD_zam_row_02)

#include "board/acs_zam_row_02.c"

#elif defined(__BOARD_zam_row_04)

#include "board/acs_zam_row_04.c"

#elif defined(__BOARD_zam_row_07) \
	  || defined(__BOARD_zam_row_07_demo)

#include "board/acs_zam_row_07.c"

#elif defined(__BOARD_zam_srw_01) \
      || defined(__BOARD_n_srw_01) \
	  || defined(__BOARD_k_srw_01) 

#include "board/acs_zam_srw_01.c"

#elif defined(__BOARD_zam_srw_01_tester)

#include "board/acs_zam_srw_01_tester.c"

#elif defined(__BOARD_zam_srw_02)

#include "board/acs_zam_srw_02.c"

#elif defined(__BOARD_zam_srw_03)

#include "board/acs_zam_srw_03.c"

#elif defined(__BOARD_zam_sbp_01) \
	  || defined(__BOARD_n_sbp_01) \
	  || defined(__BOARD_k_sbp_01)

#include "board/acs_zam_sbp_01.c"

#elif defined(__BOARD_zam_slw_01)

#include "board/acs_zam_slw_01.c"

#elif defined(__BOARD_zam_slw_02)

#include "board/acs_zam_slw_02.c"

#elif defined(__BOARD_zam_pnw_01)

#include "board/acs_zam_pnw_01.c"

#elif defined(__BOARD_zam_mew_01)

#include "board/acs_zam_mew_01.c"

#elif defined(__BOARD_h801)

#include "board/h801.c"

#elif defined(__BOARD_lightswitch_x2) \
        || defined(__BOARD_lightswitch_x2_DHT11) \
        || defined(__BOARD_lightswitch_x2_DHT22) \
        || defined(__BOARD_lightswitch_x2_54) \
        || defined(__BOARD_lightswitch_x2_54_DHT11) \
        || defined(__BOARD_lightswitch_x2_54_DHT22)

#include "board/lightswitch.c"

#elif defined(__BOARD_impulse_counter)

#include "board/impulse_counter.c"

#elif defined(__BOARD_hp_homeplus)

#include "board/acs_hp_homeplus.c"

#elif defined(__BOARD_inCan_DS) \
    ||  defined(__BOARD_inCan_DHT11)\
    ||  defined(__BOARD_inCan_DHT22)\
    ||  defined(__BOARD_inCanRS_DS) \
    ||  defined(__BOARD_inCanRS_DHT11)\
    ||  defined(__BOARD_inCanRS_DHT22)

#include "board/inCan.c"

// Moje Plytki

#elif defined(__BOARD_k_gate_module)   // nice

#include "board/k_gate_module.c"

#elif defined(__BOARD_k_dimmer)

#include "board/k_dimmer.c"

#elif defined(__BOARD_k_gniazdko_neo)

#include "board/k_gniazdko_neo.c"

#elif defined(__BOARD_k_impulse_counter)

#include "board/k_impulse_counter.c"

#elif defined(__BOARD_k_impulse_counter_3)

#include "board/k_impulse_counter_3.c"

#elif defined(__BOARD_k_rs_module) \
      || defined(__BOARD_k_rs_module_ds18b20) \
      || defined(__BOARD_k_rs_module_DHT22) \

#include "board/k_rs_module.c"

#elif defined(__BOARD_k_smoke_module) \
      || defined(__BOARD_k_smoke_module_ds18b20)	\
      || defined(__BOARD_k_smoke_module_DHT22)		\

#include "board/k_smoke_module.c"

#elif defined(__BOARD_k_socket) \
      || defined(__BOARD_k_socket_ds18b20)	\
      || defined(__BOARD_k_socket_DHT22)		\

#include "board/k_socket.c"

#elif defined(__BOARD_k_socket_dual) \
      || defined(__BOARD_k_socket_dual_ds18b20)	\
      || defined(__BOARD_k_socket_dual_DHT22)		\

#include "board/k_socket_dual.c"

#elif defined(__BOARD_k_sonoff) \
      || defined(__BOARD_k_sonoff_ds18b20)	\
      || defined(__BOARD_k_sonoff_DHT22)		\

#include "board/k_sonoff.c"

#elif defined(__BOARD_k_sonoff_touch) \
      ||  defined(__BOARD_k_sonoff_touch_dual) \
      || defined(__BOARD_k_sonoff_touch_triple) \

#include "board/k_sonoff_touch.c"

#endif
