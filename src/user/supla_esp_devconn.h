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


void DEVCONN_ICACHE_FLASH supla_esp_devconn_init(void);
void DEVCONN_ICACHE_FLASH supla_esp_devconn_start(void);
void DEVCONN_ICACHE_FLASH supla_esp_devconn_stop(void);
char * DEVCONN_ICACHE_FLASH supla_esp_devconn_laststate(void);
char DEVCONN_ICACHE_FLASH supla_esp_devconn_is_registered(void);
void DEVCONN_ICACHE_FLASH supla_esp_channel_value__changed(int channel_number, char value[SUPLA_CHANNELVALUE_SIZE]);
void DEVCONN_ICACHE_FLASH supla_esp_channel_value_changed(int channel_number, char v);
void DEVCONN_ICACHE_FLASH supla_esp_channel_extendedvalue_changed(unsigned char channel_number, TSuplaChannelExtendedValue *value);
void DEVCONN_ICACHE_FLASH supla_esp_devconn_send_channel_values_with_delay(void);
void DEVCONN_ICACHE_FLASH supla_esp_devconn_system_restart(void);

void DEVCONN_ICACHE_FLASH supla_esp_devconn_before_cfgmode_start(void);
void DEVCONN_ICACHE_FLASH supla_esp_devconn_before_update_start(void);

#ifdef ELECTRICITY_METER_COUNT
void DEVCONN_ICACHE_FLASH supla_esp_channel_em_value_changed(unsigned char channel_number, TElectricityMeter_ExtendedValue *em_ev);
#endif /*ELECTRICITY_METER_COUNT*/

#if defined(POWSENSOR2)
void ICACHE_FLASH_ATTR supla_pow_R2_setup(void);
void ICACHE_FLASH_ATTR supla_getVoltage(char value[SUPLA_CHANNELVALUE_SIZE]);
void ICACHE_FLASH_ATTR supla_getCurrent(char value[SUPLA_CHANNELVALUE_SIZE]);
void ICACHE_FLASH_ATTR supla_getPower(char value[SUPLA_CHANNELVALUE_SIZE]);
void ICACHE_FLASH_ATTR uart_status(unsigned int relay_laststate);
void ICACHE_FLASH_ATTR supla_micros();
#endif

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

#ifdef BOARD_CALCFG
void DEVCONN_ICACHE_FLASH supla_esp_calcfg_result(TDS_DeviceCalCfgResult *result);
#endif /*BOARD_CALCFG*/

#endif /* SUPLA_ESP_CLIENT_H_ */
