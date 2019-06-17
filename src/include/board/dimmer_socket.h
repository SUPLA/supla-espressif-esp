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

#ifndef DIMMER_SOCKET_H_
#define SUPLA_GATE_MODULE_H_

#define ESP8266_SUPLA_PROTO_VERSION 7

#define LED_LED_PORT  2
#define SUPLA_PWM_COUNT  1
#define DIMMER_CHANNEL   0

#define SUPLA_ESP_SOFTVER "2.7.9.0"
#define AP_SSID "SOCKET_DIMMER"

#define PWM_0_OUT_IO_MUX PERIPHS_IO_MUX_MTDO_U
#define PWM_0_OUT_IO_NUM 15
#define PWM_0_OUT_IO_FUNC  FUNC_GPIO15

#define BOARD_GPIO_OUTPUT_SET_HI if ( port == 4 )  {supla_log(LOG_DEBUG, "warunek port = %i", port);\
if ( hi == 1 )  {\
supla_log(LOG_DEBUG, "warunek hi = %i", hi);\
supla_esp_pwm_set_percent_duty(50, 100, 0);\
} else\
{ supla_esp_pwm_set_percent_duty(0, 100, 0); }\
}

void ICACHE_FLASH_ATTR supla_esp_board_pwm_init(void);
char ICACHE_FLASH_ATTR supla_esp_board_set_rgbw_value(int ChannelNumber, int *Color, float *ColorBrightness, float *Brightness);
void ICACHE_FLASH_ATTR supla_esp_board_send_channel_values_with_delay(void *srpc);

#endif
