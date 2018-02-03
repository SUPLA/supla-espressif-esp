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
#include <user_interface.h>
#include <espconn.h>
#include <spi_flash.h>
#include <osapi.h>
#include <mem.h>


#include "supla_esp_cfg.h"
#include "supla-dev/log.h"

SuplaEspCfg supla_esp_cfg;
SuplaEspState supla_esp_state;
static ETSTimer supla_esp_cfg_timer1;

char CFG_ICACHE_FLASH_ATTR
supla_esp_cfg_save(SuplaEspCfg *cfg) {

	ets_intr_lock();
	spi_flash_erase_sector(CFG_SECTOR);

	if ( SPI_FLASH_RESULT_OK == spi_flash_write(CFG_SECTOR * SPI_FLASH_SEC_SIZE, (uint32*)cfg, sizeof(SuplaEspCfg)) ) {
		//supla_log(LOG_DEBUG, "CFG WRITE SUCCESS");
		ets_intr_unlock();
		return 1;
	}

	ets_intr_unlock();
	supla_log(LOG_DEBUG, "CFG WRITE FAIL!");
	return 0;
}

void CFG_ICACHE_FLASH_ATTR
_supla_esp_save_state(void *timer_arg) {

	ets_intr_lock();
	spi_flash_erase_sector(CFG_SECTOR+1);

	if ( SPI_FLASH_RESULT_OK == spi_flash_write((CFG_SECTOR+1) * SPI_FLASH_SEC_SIZE, (uint32*)&supla_esp_state, sizeof(SuplaEspState)) ) {
		//supla_log(LOG_DEBUG, "STATE WRITE SUCCESS");
		ets_intr_unlock();
		return;
	}

	ets_intr_unlock();
	supla_log(LOG_DEBUG, "STATE WRITE FAIL!");
}

void CFG_ICACHE_FLASH_ATTR
supla_esp_save_state(int delay) {

	os_timer_disarm(&supla_esp_cfg_timer1);

	if ( delay > 0 ) {

		os_timer_setfn(&supla_esp_cfg_timer1, _supla_esp_save_state, NULL);
	    os_timer_arm (&supla_esp_cfg_timer1, delay, 0);

	} else {
		_supla_esp_save_state(NULL);
	}


}

void CFG_ICACHE_FLASH_ATTR factory_defaults(char save) {

	char GUID[SUPLA_GUID_SIZE];
	char AuthKey[SUPLA_AUTHKEY_SIZE];
	char TAG[6];
	char Test;

	memcpy(GUID, supla_esp_cfg.GUID, SUPLA_GUID_SIZE);
	memcpy(AuthKey, supla_esp_cfg.AuthKey, SUPLA_AUTHKEY_SIZE);
	memcpy(TAG, supla_esp_cfg.TAG, 6);
	Test = supla_esp_cfg.Test;

	memset(&supla_esp_cfg, 0, sizeof(SuplaEspCfg));
	memcpy(supla_esp_cfg.GUID, GUID, SUPLA_GUID_SIZE);
	memcpy(supla_esp_cfg.AuthKey, AuthKey, SUPLA_AUTHKEY_SIZE);
	memcpy(supla_esp_cfg.TAG, TAG, 6);

	supla_esp_cfg.CfgButtonType = BTN_TYPE_MONOSTABLE;
	supla_esp_cfg.Button1Type = BTN1_DEFAULT;
	supla_esp_cfg.Button2Type = BTN2_DEFAULT;

	memset(&supla_esp_state, 0, sizeof(SuplaEspState));
	supla_esp_cfg.Test = Test;

	if ( save == 1 ) {

		supla_esp_cfg_save(&supla_esp_cfg);
		supla_esp_save_state(0);

	}

}

char CFG_ICACHE_FLASH_ATTR
supla_esp_cfg_init(void) {

	char TAG[6] = {'S','U','P','L','A', 6}; // 6 == v6
	char mac[6];
	int a;

	os_timer_disarm(&supla_esp_cfg_timer1);

	if ( SPI_FLASH_RESULT_OK == spi_flash_read(CFG_SECTOR * SPI_FLASH_SEC_SIZE, (uint32*)&supla_esp_cfg, sizeof(SuplaEspCfg)) ) {
	   if ( memcmp(supla_esp_cfg.TAG, TAG, 6) == 0 ) {

		   supla_log(LOG_DEBUG, "CFG READ SUCCESS!");

		   /*
		   supla_log(LOG_DEBUG, "SSID: %s", supla_esp_cfg.WIFI_SSID);
		   //supla_log(LOG_DEBUG, "Wifi PWD: %s", supla_esp_cfg.WIFI_PWD);
		   supla_log(LOG_DEBUG, "SVR: %s", supla_esp_cfg.Server);
		   supla_log(LOG_DEBUG, "Location ID: %i", supla_esp_cfg.LocationID);
		   //supla_log(LOG_DEBUG, "Location PWD: %s", supla_esp_cfg.LocationPwd);
		   supla_log(LOG_DEBUG, "CFG BUTTON TYPE: %s", supla_esp_cfg.CfgButtonType == BTN_TYPE_MONOSTABLE ? "button" : "switch");

		   supla_log(LOG_DEBUG, "BUTTON1 TYPE: %s", supla_esp_cfg.Button1Type == BTN_TYPE_MONOSTABLE ? "button" : "switch");
		   supla_log(LOG_DEBUG, "BUTTON2 TYPE: %s", supla_esp_cfg.Button2Type == BTN_TYPE_MONOSTABLE ? "button" : "switch");
		   
		   supla_log(LOG_DEBUG, "LedOff: %i", supla_esp_cfg.StatusLedOff);
		   supla_log(LOG_DEBUG, "InputCfgTriggerOff: %i", supla_esp_cfg.InputCfgTriggerOff);
		   */

			if ( SPI_FLASH_RESULT_OK == spi_flash_read((CFG_SECTOR+1) * SPI_FLASH_SEC_SIZE, (uint32*)&supla_esp_state, sizeof(SuplaEspState)) ) {
			    supla_log(LOG_DEBUG, "STATE READ SUCCESS!");
			} else {
				supla_log(LOG_DEBUG, "STATE READ FAIL!");
			}

		   return 1;
	   }
	}

	factory_defaults(0);
	supla_esp_cfg.Test = 0;
	memcpy(supla_esp_cfg.TAG, TAG, 6);


	os_get_random((unsigned char*)supla_esp_cfg.GUID, SUPLA_GUID_SIZE);
	os_get_random((unsigned char*)supla_esp_cfg.AuthKey, SUPLA_AUTHKEY_SIZE);

	if ( SUPLA_GUID_SIZE >= 6 ) {
		wifi_get_macaddr(STATION_IF, (unsigned char*)mac);

		for(a=0;a<6;a++)
			supla_esp_cfg.GUID[a] = (supla_esp_cfg.GUID[a] * mac[a]) % 255;
	}

	if ( SUPLA_GUID_SIZE >=12 ) {
		wifi_get_macaddr(SOFTAP_IF, (unsigned char*)mac);

		for(a=0;a<6;a++)
			supla_esp_cfg.GUID[a+6] = ( supla_esp_cfg.GUID[a+6] * mac[a] ) % 255;
	}

	for(a=0;a<SUPLA_GUID_SIZE;a++) {
		supla_esp_cfg.GUID[a]= (supla_esp_cfg.GUID[a] + system_get_time() + spi_flash_get_id() + system_get_chip_id() + system_get_rtc_time()) % 255;
	}

	a = SUPLA_GUID_SIZE > SUPLA_AUTHKEY_SIZE ? SUPLA_AUTHKEY_SIZE : SUPLA_GUID_SIZE;
	for(;a>0;a--) {
		supla_esp_cfg.AuthKey[a] += supla_esp_cfg.GUID[a];
	}

	#ifdef CFG_AFTER_GUID_GEN
	CFG_AFTER_GUID_GEN;
	#endif


	if ( supla_esp_cfg_save(&supla_esp_cfg) == 1 ) {

		supla_esp_save_state(1);
		return 1;
	}

	return 0;
}



/*
char CFG_ICACHE_FLASH_ATTR supla_esp_write_log(char *log) {

	supla_esp_state.len++;

	if ( supla_esp_state.len < 1 || supla_esp_state.len > 20 )
		supla_esp_state.len = 1;

	ets_snprintf(supla_esp_state.log[supla_esp_state.len-1], 200, "%s", log);

	supla_esp_save_state(200000);

}
*/
