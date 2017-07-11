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

#include "board/supla_esp_board.c"

#ifdef __FOTA
#include "supla_update.h"
#endif

/*
void *supla_malloc(size_t size) {

	void *result = os_malloc(size);

	if ( result == NULL ) {
		supla_log(LOG_DEBUG, "Free heap size: %i/%i", system_get_free_heap_size(), size);
	}

	return result;
}
*/

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

	 struct rst_info *rtc_info = system_get_rst_info();
	 supla_log(LOG_DEBUG, "RST reason: %i", rtc_info->reason);

	 system_soft_wdt_restart();

     wifi_status_led_uninstall();
     supla_esp_cfg_init();
     supla_esp_gpio_init();

     supla_log(LOG_DEBUG, "Starting %i", system_get_time());

     #ifdef BOARD_ESP_STARTING
     BOARD_ESP_STARTING
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

     if ( supla_esp_cfg.LocationID == 0
    		 || supla_esp_cfg.LocationPwd[0] == 0
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

	supla_esp_devconn_start();

	system_print_meminfo();

/*
	if ( supla_esp_state.ltag != 25 ) {
		supla_log(LOG_DEBUG, "Log state reset");
		memset(supla_esp_state.log, 0, 4000);
		supla_esp_state.len = 0;
		supla_esp_state.ltag = 25;
		supla_esp_save_state(1);
	}

	if ( supla_esp_state.len < 0 || supla_esp_state.len > 20 )
		supla_esp_state.len = 0;

	int a;
	for(a=0;a<supla_esp_state.len;a++)
		supla_log(LOG_DEBUG, "%i. %s", a, supla_esp_state.log[a]);
*/

}

