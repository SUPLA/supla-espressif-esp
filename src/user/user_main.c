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


#include <string.h>
#include <stdio.h>

#include <ip_addr.h>
#include <osapi.h>
#include <mem.h>
#include <user_interface.h>

#include "supla_esp.h"
#include "supla_esp_cfg.h"
#include "supla_esp_devconn.h"
#include "supla_esp_cfgmode.h"
#include "supla_esp_gpio.h"
#include "supla_esp_pwm.h"
#include "supla_dht.h"
#include "supla_ds18b20.h"
#include "supla-dev/log.h"
#include "supla_esp_electricity_meter.h"
#include "supla_esp_impulse_counter.h"

#include "board/supla_esp_board.c"

#ifdef __FOTA
#include "supla_update.h"
#endif

void ICACHE_FLASH_ATTR supla_system_restart(void) {

	supla_esp_devconn_before_system_restart();

	#ifdef BOARD_BEFORE_REBOOT
	supla_esp_board_before_reboot();
	#endif

	supla_log(LOG_DEBUG, "RESTART");
	supla_log(LOG_DEBUG, "Free heap size: %i", system_get_free_heap_size());

	system_restart();

}

uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{

    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}


void MAIN_ICACHE_FLASH user_rf_pre_init() {};



void MAIN_ICACHE_FLASH user_init(void)
{

#ifdef BOARD_USER_INIT
	BOARD_USER_INIT;
#else

	 struct rst_info *rtc_info = system_get_rst_info();
	 supla_log(LOG_DEBUG, "RST reason: %i", rtc_info->reason);

	 system_soft_wdt_restart();

     wifi_status_led_uninstall();
     supla_esp_cfg_init();
     supla_esp_gpio_init();

     supla_log(LOG_DEBUG, "Starting %i", system_get_time());

     #ifdef BOARD_ESP_STARTING
     BOARD_ESP_STARTING;
     #endif

	 #if NOSSL == 1
      supla_log(LOG_DEBUG, "NO SSL!");
	 #endif

	 #ifdef __FOTA
	  supla_esp_update_init();
	 #endif

     supla_esp_devconn_init();


     #ifdef DS18B20
		 supla_ds18b20_init();
     #endif

     #ifdef DHTSENSOR
		 supla_dht_init();
     #endif

	#ifdef SUPLA_PWM_COUNT
	     supla_esp_pwm_init();
	#endif

	#ifdef ELECTRICITY_METER_COUNT
		 supla_esp_em_init();
	#endif

	#ifdef IMPULSE_COUNTER_COUNT
		 supla_esp_ic_init();
	#endif

     if ( ( (supla_esp_cfg.LocationID == 0 || supla_esp_cfg.LocationPwd[0] == 0)
    		 && supla_esp_cfg.Email[0] == 0 )
    		|| supla_esp_cfg.Server[0] == 0
    		|| supla_esp_cfg.WIFI_PWD[0] == 0
    		|| supla_esp_cfg.WIFI_SSID[0] == 0 ) {

    	 supla_esp_cfgmode_start();
    	 return;
     }


    #ifdef DS18B20
		supla_ds18b20_start();
    #endif

	#ifdef DHTSENSOR
		supla_dht_start();
	#endif

	#ifdef ELECTRICITY_METER_COUNT
		 supla_esp_em_start();
	#endif

	#ifdef IMPULSE_COUNTER_COUNT
		 supla_esp_ic_start();
	#endif

	supla_esp_devconn_start();

	system_print_meminfo();
	
	#ifdef BOARD_ESP_STARTED
	BOARD_ESP_STARTED;
	#endif
	
#endif /*BOARD_USER_INIT*/

}

