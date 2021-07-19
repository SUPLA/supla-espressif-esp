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

#include "user_interface.h"

bool wifi_get_macaddr(uint8 if_index, uint8 *macaddr) { return true; }

void system_set_os_print(uint8 onoff) {}

uint32 system_get_chip_id() { return 0; }

uint32 system_get_rtc_time() { return 0; }

uint32 system_get_free_heap_size(void) { return 0; }

bool wifi_softap_get_config(struct softap_config *config) { return true; }

bool wifi_set_opmode(uint8 opmode) { return true; }

bool wifi_softap_set_config(struct softap_config *config) { return true; }

bool wifi_get_ip_info(uint8 if_index, struct ip_info *info) { return true; }

sint8 wifi_station_get_rssi(void) { return 0; }

void system_soft_wdt_stop(void) { return; }

void system_soft_wdt_restart(void) { return; }

struct rst_info *system_get_rst_info(void) {
  return 0;
}

uint8 wifi_station_get_connect_status(void) { return 0; }

bool wifi_station_disconnect(void) { return true; }

bool wifi_station_set_config(struct station_config *config) { return true; }

bool wifi_station_set_auto_connect(uint8 set) { return true; }

bool wifi_station_connect(void) { return true; }

void system_upgrade_flag_set(uint8 flag) { return; }

void system_upgrade_reboot(void) { return; }

enum flash_size_map system_get_flash_size_map(void) {
  return FLASH_SIZE_16M_MAP_1024_1024;
}

uint8 system_upgrade_userbin_check(void) { return 0; }
