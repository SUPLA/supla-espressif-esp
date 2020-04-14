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

#ifndef SUPLA_ESP_CFG_H_
#define SUPLA_ESP_CFG_H_

#include <c_types.h>

#include "supla-dev/proto.h"
#include "supla_esp.h"

#define BTN_TYPE_MONOSTABLE       0
#define BTN_TYPE_BISTABLE         1

#define THERM_NONE				  0
#define THERM_DS18B20			  1
#define THERM_DHT22				  2

typedef struct {

	char TAG[6];
	char GUID[SUPLA_GUID_SIZE];
    char AuthKey[SUPLA_AUTHKEY_SIZE];

	char Server[SERVER_MAXSIZE];
	char Email[SUPLA_EMAIL_MAXSIZE];

	int LocationID;
    char LocationPwd[SUPLA_LOCATION_PWD_MAXSIZE];

    char WIFI_SSID[WIFI_SSID_MAXSIZE];
    char WIFI_PWD[WIFI_PWD_MAXSIZE];

    char CfgButtonType;
    char Button1Type;
    char Button2Type;

    char StatusLedOff;
    char InputCfgTriggerOff;

    char FirmwareUpdate;
    char Test;

    char UpsideDown;

    unsigned int Time1[CFG_TIME1_COUNT];
    unsigned int Time2[CFG_TIME2_COUNT];

    char Trigger;
    char ZeroInitialEnergy;
	
	char ThermometerType;
    char zero[200];

}SuplaEspCfg;

typedef struct {

	char TAG[6];
	char GUID[SUPLA_GUID_SIZE];
    char AuthKey[SUPLA_AUTHKEY_SIZE];

	char Server[SERVER_MAXSIZE];
	char Email[SUPLA_EMAIL_MAXSIZE];

	int LocationID;
    char LocationPwd[SUPLA_LOCATION_PWD_MAXSIZE];

    char WIFI_SSID[WIFI_SSID_MAXSIZE];
    char WIFI_PWD[WIFI_PWD_MAXSIZE];

    char CfgButtonType;
    char Button1Type;
    char Button2Type;

    char StatusLedOff;
    char InputCfgTriggerOff;
	char ThermometerType;

    char FirmwareUpdate;
    char Test;

    char UpsideDown;

    unsigned int Time1[2];
    unsigned int Time2[2];

    char Trigger;

    char zero[200];

}SuplaEspCfg_old_v6;

typedef struct {

	char TAG[6];
	char GUID[SUPLA_GUID_SIZE];
	char Server[SERVER_MAXSIZE];
	int LocationID;
    char LocationPwd[SUPLA_LOCATION_PWD_MAXSIZE];
    char WIFI_SSID[WIFI_SSID_MAXSIZE];
    char WIFI_PWD[WIFI_PWD_MAXSIZE];
    char CfgButtonType;
    char Button1Type;
    char Button2Type;
    char StatusLedOff;
    char InputCfgTriggerOff;
	char ThermometerType;
    char FirmwareUpdate;
    char Test;
    char Email[SUPLA_EMAIL_MAXSIZE];
    char AuthKey[SUPLA_AUTHKEY_SIZE];
    char UpsideDown;
    unsigned int Time1[2];
    unsigned int Time2[2];
    char Trigger;
    char zero[200];

}SuplaEspCfg_old_v5B;

typedef struct {

	char TAG[6];
	char GUID[SUPLA_GUID_SIZE];
	char Server[SERVER_MAXSIZE];
	int LocationID;
    char LocationPwd[SUPLA_LOCATION_PWD_MAXSIZE];
    char WIFI_SSID[WIFI_SSID_MAXSIZE];
    char WIFI_PWD[WIFI_PWD_MAXSIZE];
    char CfgButtonType;
    char Button1Type;
    char Button2Type;
    char StatusLedOff;
    char InputCfgTriggerOff;
	char ThermometerType;
    char FirmwareUpdate;
    char Test;
    unsigned int FullOpeningTime[2];
    unsigned int FullClosingTime[2];
    char Email[SUPLA_EMAIL_MAXSIZE];
    char AuthKey[SUPLA_AUTHKEY_SIZE];
    char UpsideDown;
    char zero[200];

}SuplaEspCfg_old_v5A;

typedef struct {

	char Relay[RELAY_MAX_COUNT];

    int color[RS_MAX_COUNT];
    char color_brightness[RS_MAX_COUNT];
    char brightness[RS_MAX_COUNT];

    int rs_position[RS_MAX_COUNT];
    
    unsigned int Time1Left[CFG_TIME1_COUNT];
    unsigned int Time2Left[CFG_TIME2_COUNT];
	unsigned int full_energy;	

    char turnedOff[RS_MAX_COUNT];

    char zero[200];
/*
	char ltag;
	char len;
	char log[20][200];
*/

}SuplaEspState;

#define CMD_MAXSIZE 100
extern char *user_cmd;

extern SuplaEspCfg supla_esp_cfg;
extern SuplaEspState supla_esp_state;

char CFG_ICACHE_FLASH_ATTR supla_esp_write_state(char *message);
char CFG_ICACHE_FLASH_ATTR supla_esp_read_state(char *message);

char CFG_ICACHE_FLASH_ATTR supla_esp_cfg_init(void);
char CFG_ICACHE_FLASH_ATTR supla_esp_cfg_ready_to_connect(void);
char CFG_ICACHE_FLASH_ATTR supla_esp_cfg_save(SuplaEspCfg *cfg);
void CFG_ICACHE_FLASH_ATTR supla_esp_save_state(int delay);
void CFG_ICACHE_FLASH_ATTR factory_defaults(char save);


//char CFG_ICACHE_FLASH_ATTR supla_esp_write_log(char *log);


#endif /* SUPLA_ESP_CFG_H_ */
