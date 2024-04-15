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


#ifndef SUPLA_ESP_CLIENT_H_
#define SUPLA_ESP_CLIENT_H_



#include "supla_esp.h"
#include "supla_esp_state.h"

// Maintaining backward compatibility
#define supla_esp_devconn_laststate supla_esp_get_laststate
// ----------------------------------

void DEVCONN_ICACHE_FLASH supla_esp_devconn_init(void);
void DEVCONN_ICACHE_FLASH supla_esp_devconn_release(void);
void DEVCONN_ICACHE_FLASH supla_esp_devconn_start(void);
void DEVCONN_ICACHE_FLASH supla_esp_devconn_stop(void);
char DEVCONN_ICACHE_FLASH supla_esp_devconn_is_registered(void);
void DEVCONN_ICACHE_FLASH supla_esp_channel_value__changed(int channel_number, char value[SUPLA_CHANNELVALUE_SIZE]);
void DEVCONN_ICACHE_FLASH supla_esp_channel_value_changed(int channel_number, char v);
void DEVCONN_ICACHE_FLASH supla_esp_channel_extendedvalue_changed(unsigned char channel_number, TSuplaChannelExtendedValue *value);
void DEVCONN_ICACHE_FLASH supla_esp_set_channel_result(unsigned char ChannelNumber, _supla_int_t SenderID, char Success);
void DEVCONN_ICACHE_FLASH supla_esp_devconn_send_channel_values_with_delay(void);
void DEVCONN_ICACHE_FLASH supla_esp_devconn_send_channel_values_with__delay(int time_ms);
void DEVCONN_ICACHE_FLASH supla_esp_devconn_before_system_restart(void);
uint8 DEVCONN_ICACHE_FLASH supla_esp_devconn_any_outgoingdata_exists(void);

void DEVCONN_ICACHE_FLASH supla_esp_devconn_before_cfgmode_start(void);
void DEVCONN_ICACHE_FLASH supla_esp_devconn_before_update_start(void);

void ICACHE_FLASH_ATTR supla_esp_board_send_channel_values_with_delay(void *srpc);

void DEVCONN_ICACHE_FLASH supla_esp_devconn_send_action_trigger(
    unsigned char channel_number,  _supla_int_t action_trigger);

#ifdef ELECTRICITY_METER_COUNT
void DEVCONN_ICACHE_FLASH supla_esp_channel_em_value_changed(unsigned char channel_number, TElectricityMeter_ExtendedValue_V2 *em_ev);
#endif /*ELECTRICITY_METER_COUNT*/

#if ESP8266_SUPLA_PROTO_VERSION >= 12 || defined(CHANNEL_STATE_TOOLS)
void DEVCONN_ICACHE_FLASH
supla_esp_get_channel_state(_supla_int_t ChannelNumber, _supla_int_t ReceiverID,
                            TDSC_ChannelState *state);
void DEVCONN_ICACHE_FLASH supla_esp_get_channel_functions(void);
void DEVCONN_ICACHE_FLASH supla_esp_channel_value__changed_b(
    int channel_number, char value[SUPLA_CHANNELVALUE_SIZE],
    unsigned char offline);
void DEVCONN_ICACHE_FLASH supla_esp_channel_value__changed_c(
    int channel_number, char value[SUPLA_CHANNELVALUE_SIZE],
    unsigned char offline, unsigned _supla_int_t validity_time_sec);
#endif /*ESP8266_SUPLA_PROTO_VERSION >= 12 || defined(CHANNEL_STATE_TOOLS)*/

#if defined(RGB_CONTROLLER_CHANNEL) \
    || defined(RGBW_CONTROLLER_CHANNEL) \
    || defined(RGBWW_CONTROLLER_CHANNEL) \
    || defined(DIMMER_CHANNEL)

typedef struct {
    double h;
    double s;
    double v;
} hsv;

hsv DEVCONN_ICACHE_FLASH rgb2hsv(int rgb);
int DEVCONN_ICACHE_FLASH hsv2rgb(hsv in);

void DEVCONN_ICACHE_FLASH
supla_esp_channel_rgbw_to_value(char value[SUPLA_CHANNELVALUE_SIZE], int color, char color_brightness, char brightness);

#ifdef RGBW_ONOFF_SUPPORT
void DEVCONN_ICACHE_FLASH
supla_esp_channel_set_rgbw_value(int ChannelNumber, int Color, char ColorBrightness, char Brightness, char TurnOnOff, char smoothly, char send_value_changed);
#else
void DEVCONN_ICACHE_FLASH
supla_esp_channel_set_rgbw_value(int ChannelNumber, int Color, char ColorBrightness, char Brightness, char smoothly, char send_value_changed);
#endif /*RGBW_ONOFF_SUPPORT*/

#endif

void DEVCONN_ICACHE_FLASH
supla_esp_calcfg_result(TDS_DeviceCalCfgResult *result);
void DEVCONN_ICACHE_FLASH
supla_esp_calcfg_request(TSD_DeviceCalCfgRequest *request);

#ifdef BOARD_ON_USER_LOCALTIME_RESULT
void DEVCONN_ICACHE_FLASH supla_esp_devconn_get_user_localtime(void);
#endif /*BOARD_ON_USER_LOCALTIME_RESULT*/

#ifdef BOARD_ON_CHANNEL_INT_PARAMS_RESULT
void DEVCONN_ICACHE_FLASH
supla_esp_devconn_get_channel_int_params(unsigned char channel_number);
#endif /*BOARD_ON_CHANNEL_INT_PARAMS_RESULT*/

#ifdef BOARD_CALCFG
bool ICACHE_FLASH_ATTR
supla_esp_board_calcfg_request(TSD_DeviceCalCfgRequest *request);
#endif /*BOARD_CALCFG*/

// moved to public interface for testing purposes
void DEVCONN_ICACHE_FLASH
supla_esp_channel_set_value(TSD_SuplaChannelNewValue *new_value);
void DEVCONN_ICACHE_FLASH supla_esp_srpc_init(void);
void DEVCONN_ICACHE_FLASH
supla_esp_channel_config_result(TSD_ChannelConfig *result);

#ifdef BOARD_CHANNEL_CONFIG
void ICACHE_FLASH_ATTR
supla_esp_board_channel_config(TSD_ChannelConfig *config);
#endif /*BOARD_CHANNEL_CONFIG*/

#endif /* SUPLA_ESP_CLIENT_H_ */
