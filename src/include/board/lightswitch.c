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
#include "supla_dht.h"
#include "supla_ds18b20.h"

void supla_esp_board_set_device_name(char *buffer, uint8 buffer_size) {

 #if defined(__BOARD_lightswitch_x2) || defined(__BOARD_lightswitch_x2_54)

	ets_snprintf(buffer, buffer_size, "SUPLA-LIGHTSWITCH-x2-DS");

  #elif defined(__BOARD_lightswitch_x2_DHT11) || defined(__BOARD_lightswitch_x2_54_DHT11)

	ets_snprintf(buffer, buffer_size, "SUPLA-LIGHTSWITCH-x2-DHT11");

  #elif defined(__BOARD_lightswitch_x2_DHT22) || defined(__BOARD_lightswitch_x2_54_DHT22)

	ets_snprintf(buffer, buffer_size, "SUPLA-LIGHTSWITCH-x2-DHT22");

 #endif

}


void supla_esp_board_gpio_init(void) {

	supla_input_cfg[0].type = supla_esp_cfg.CfgButtonType == BTN_TYPE_BISTABLE ? INPUT_TYPE_BTN_BISTABLE : INPUT_TYPE_BTN_MONOSTABLE;
	supla_input_cfg[0].gpio_id = B_CFG_PORT;
	supla_input_cfg[0].flags = INPUT_FLAG_PULLUP | INPUT_FLAG_CFG_BTN;
	supla_input_cfg[0].relay_gpio_id = B_RELAY1_PORT;
	supla_input_cfg[0].channel = 0;
	
	supla_input_cfg[1].type = supla_esp_cfg.CfgButtonType == BTN_TYPE_BISTABLE ? INPUT_TYPE_BTN_BISTABLE : INPUT_TYPE_BTN_MONOSTABLE;
	supla_input_cfg[1].gpio_id = B_BTN2_PORT;
	supla_input_cfg[1].flags = INPUT_FLAG_PULLUP;
	supla_input_cfg[1].relay_gpio_id = B_RELAY2_PORT;
	supla_input_cfg[1].channel = 1;

// -----------------------------------------------------------
	supla_relay_cfg[0].gpio_id = B_RELAY1_PORT;
	supla_relay_cfg[0].flags = RELAY_FLAG_RESET;
//	supla_relay_cfg[0].flags = RELAY_FLAG_RESTORE;
	supla_relay_cfg[0].channel = 0;
	
	supla_relay_cfg[1].gpio_id = B_RELAY2_PORT;
	supla_relay_cfg[1].flags = RELAY_FLAG_RESET;
//	supla_relay_cfg[1].flags = RELAY_FLAG_RESTORE;
	supla_relay_cfg[1].channel = 1;

}


void supla_esp_board_set_channels(TDS_SuplaDeviceChannel_B *channels, unsigned char *channel_count) {
	
	*channel_count = 3;
	
	channels[0].Number = 0;
	channels[0].Type = SUPLA_CHANNELTYPE_RELAY;
	channels[0].FuncList = SUPLA_BIT_FUNC_LIGHTSWITCH;
	channels[0].Default = SUPLA_CHANNELFNC_LIGHTSWITCH;
	channels[0].value[0] = supla_esp_gpio_relay_on(B_RELAY1_PORT);

	channels[1].Number = 1;
	channels[1].Type = channels[0].Type;
	channels[1].FuncList = channels[0].FuncList;
	channels[1].Default = channels[0].Default;
	channels[1].value[0] = supla_esp_gpio_relay_on(B_RELAY2_PORT);

	channels[2].Number = 2;

	#if defined(DS18B20)

		channels[2].Type = SUPLA_CHANNELTYPE_THERMOMETERDS18B20;
		supla_get_temperature(channels[2].value);

	#elif defined(SENSOR_DHT11)

		channels[2].Type = SUPLA_CHANNELTYPE_DHT11;
		supla_get_temp_and_humidity(channels[2].value);

	#elif defined(SENSOR_DHT22)

		channels[2].Type = SUPLA_CHANNELTYPE_DHT22;
		supla_get_temp_and_humidity(channels[2].value);

	#endif

	channels[2].FuncList = 0;
	channels[2].Default = 0;



}


void supla_esp_board_send_channel_values_with_delay(void *srpc) {

	supla_esp_channel_value_changed(0, supla_esp_gpio_relay_on(B_RELAY1_PORT));
	supla_esp_channel_value_changed(1, supla_esp_gpio_relay_on(B_RELAY2_PORT));

}

