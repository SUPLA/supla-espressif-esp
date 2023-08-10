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

// used for Button1Type, Button2Type, BtnType[x]
#define BTN_TYPE_MONOSTABLE       0
#define BTN_TYPE_BISTABLE         1
#define BTN_TYPE_MOTION_SENSOR    2

#define STAIRCASE_BTN_TYPE_RESET  0
#define STAIRCASE_BTN_TYPE_TOGGLE 1

// Defines in which mode button is used.
// USE_INTERNALLY will not publish ActionTrigger. Button may be used for
// device's internal functionality (like relay toggle) or may be disabled
#define BTN_MODE_USE_INTERNALLY 0
// PUBLISH_AT mode will publish ActionTrigger depending on button input. 
#define BTN_MODE_PUBLISH_AT     1

#define CLEAN_CONFIG_SIGNATURE 0xDEADBABE

typedef struct {
  char TAG[6];
  char GUID[SUPLA_GUID_SIZE];
  char AuthKey[SUPLA_AUTHKEY_SIZE];

  char Server[SERVER_MAXSIZE];

  union {
    char Email[SUPLA_EMAIL_MAXSIZE];
    char Username[SUPLA_EMAIL_MAXSIZE];
  };

  union {
    int LocationID;
    int Port;
  };

  union {
    char LocationPwd[SUPLA_LOCATION_PWD_MAXSIZE];
    char Password[SUPLA_LOCATION_PWD_MAXSIZE];
  };

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

  unsigned int Flags;  // CFG_FLAG_*
  char MqttTopicPrefix[MQTT_PREFIX_SIZE];
  char MqttQoS;

  uint32_t OvercurrentThreshold1;
  uint32_t OvercurrentThreshold2;

  unsigned char MqttPoolPublicationDelay;

  unsigned int AutoCalOpenTime[RS_MAX_COUNT];
  unsigned int AutoCalCloseTime[RS_MAX_COUNT];

  char StaircaseButtonType;

  // Button options are not applied to input configuration automatically.
  // Please implement it in board code depending on needs.
  // ButtonType defines type of button: monostable, bistable
  char ButtonType[BTN_MAX_COUNT];
  // ButtonMode defines if button is used only internally by device (or is 
  // disalbed), or if ActionTrigger should be published
  char ButtonMode[BTN_MAX_COUNT];

  uint32_t CleanConfigSignature;

  // Don't add new parameters here without decrementing zero[200] array below.
  // It is not guaranteed that bytes outside of this 200 bytes array will be
  // initialized to 0 when CleanConfigSignature is set.
  // So if needed, add new parameter and decrease below array size accordingly.
  char zero[200];

} SuplaEspCfg;

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

  unsigned int Time1[2];
  unsigned int Time2[2];

  char Trigger;

  char zero[200];

} SuplaEspCfg_old_v6;

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
    
    unsigned int Time1Left[STATE_CFG_TIME1_COUNT];
    unsigned int Time2Left[STATE_CFG_TIME2_COUNT];

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

#ifdef BOARD_SAVE_STATE
bool CFG_ICACHE_FLASH_ATTR supla_esp_board_save_state(SuplaEspState *);
#endif /*BOARD_SAVE_STATE*/

#ifdef BOARD_LOAD_STATE
bool CFG_ICACHE_FLASH_ATTR supla_esp_board_load_state(SuplaEspState *);
#endif /*BOARD_LOAD_STATE*/

//char CFG_ICACHE_FLASH_ATTR supla_esp_write_log(char *log);


#endif /* SUPLA_ESP_CFG_H_ */
