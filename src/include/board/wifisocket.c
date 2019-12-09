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


#ifdef __BOARD_wifisocket_54

	#define B_RELAY1_PORT      5
	#define B_CFG_PORT         4

#elif defined(__BOARD_wifisocket_x4)

	#define B_RELAY1_PORT      12
	#define B_RELAY2_PORT      13
	#define B_RELAY3_PORT      14
	#define B_RELAY4_PORT      16

	#define B_CFG_PORT         0
	#define B_BTN2_PORT        2
	#define B_BTN3_PORT        4
	#define B_BTN4_PORT        5

#else

	#define B_RELAY1_PORT      4
	#define B_CFG_PORT         5

#endif

#ifdef __BOARD_wifisocket_x4
void supla_esp_board_set_device_name(char *buffer, uint8 buffer_size) {
	ets_snprintf(buffer, buffer_size, "SUPLA-SOCKETx4");
}
#else
void ICACHE_FLASH_ATTR supla_esp_board_set_device_name(char *buffer, uint8 buffer_size) {

	ets_snprintf(buffer, buffer_size, "SUPLA-SOCKET");
}
#endif


void ICACHE_FLASH_ATTR supla_esp_board_gpio_init(void) {
		

	#ifdef __BOARD_wifisocket_x4

	supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
	supla_input_cfg[0].gpio_id = B_CFG_PORT;
	supla_input_cfg[0].flags = INPUT_FLAG_PULLUP | INPUT_FLAG_CFG_BTN;
	supla_input_cfg[0].relay_gpio_id = B_RELAY1_PORT;
	supla_input_cfg[0].channel = 0;

	supla_input_cfg[1].type = INPUT_TYPE_BTN_MONOSTABLE;
	supla_input_cfg[1].gpio_id = B_BTN2_PORT;
	supla_input_cfg[1].flags = INPUT_FLAG_PULLUP;
	supla_input_cfg[1].relay_gpio_id = B_RELAY2_PORT;
	supla_input_cfg[1].channel = 1;

	supla_input_cfg[2].type = supla_esp_cfg.CfgButtonType == BTN_TYPE_BISTABLE ? INPUT_TYPE_BTN_BISTABLE : INPUT_TYPE_BTN_MONOSTABLE;
	supla_input_cfg[2].gpio_id = B_BTN3_PORT;
	supla_input_cfg[2].relay_gpio_id = B_RELAY3_PORT;
	supla_input_cfg[2].channel = 2;

	supla_input_cfg[3].type = supla_input_cfg[2].type;
	supla_input_cfg[3].gpio_id = B_BTN4_PORT;
	supla_input_cfg[3].relay_gpio_id = B_RELAY4_PORT;
	supla_input_cfg[3].channel = 3;

	#else

	supla_input_cfg[0].type = supla_esp_cfg.CfgButtonType == BTN_TYPE_BISTABLE ? INPUT_TYPE_BTN_BISTABLE : INPUT_TYPE_BTN_MONOSTABLE;
	supla_input_cfg[0].gpio_id = B_CFG_PORT;
	supla_input_cfg[0].flags = INPUT_FLAG_PULLUP | INPUT_FLAG_CFG_BTN;
	supla_input_cfg[0].relay_gpio_id = B_RELAY1_PORT;
	supla_input_cfg[0].channel = 0;

	#endif

	// ---------------------------------------
	// ---------------------------------------

    supla_relay_cfg[0].gpio_id = B_RELAY1_PORT;
    supla_relay_cfg[0].flags = RELAY_FLAG_RESTORE_FORCE;
    supla_relay_cfg[0].channel = 0;
    
	#ifdef __BOARD_wifisocket_x4

		supla_relay_cfg[1].gpio_id = B_RELAY2_PORT;
		supla_relay_cfg[1].flags = RELAY_FLAG_RESTORE;
		supla_relay_cfg[1].channel = 1;

		supla_relay_cfg[2].gpio_id = B_RELAY3_PORT;
		supla_relay_cfg[2].flags = RELAY_FLAG_RESTORE;
		supla_relay_cfg[2].channel = 2;

		supla_relay_cfg[3].gpio_id = B_RELAY4_PORT;
		supla_relay_cfg[3].flags = RELAY_FLAG_RESTORE;
		supla_relay_cfg[3].channel = 3;

	#endif

}

void ICACHE_FLASH_ATTR supla_esp_board_set_channels(TDS_SuplaDeviceChannel_B *channels, unsigned char *channel_count) {
	
	#if defined(__BOARD_wifisocket_x4)
		*channel_count = 4;
	#else
		*channel_count = 2;
	#endif

	channels[0].Number = 0;
	channels[0].Type = SUPLA_CHANNELTYPE_RELAY;

	channels[0].FuncList = SUPLA_BIT_FUNC_POWERSWITCH \
								| SUPLA_BIT_FUNC_LIGHTSWITCH;

	channels[0].Default = SUPLA_CHANNELFNC_POWERSWITCH;

	channels[0].value[0] = supla_esp_gpio_relay_on(B_RELAY1_PORT);

	#ifndef __BOARD_wifisocket_x4
		channels[1].Number = 1;
		channels[1].Type = SUPLA_CHANNELTYPE_THERMOMETERDS18B20;

		channels[1].FuncList = 0;
		channels[1].Default = 0;

		supla_get_temperature(channels[1].value);

	#endif

	#ifdef __BOARD_wifisocket_x4

		channels[1].Number = 1;
		channels[1].Type = channels[0].Type;
		channels[1].FuncList = channels[0].FuncList;
		channels[1].Default = channels[0].Default;
		channels[1].value[0] = supla_esp_gpio_relay_on(B_RELAY2_PORT);

		channels[2].Number = 2;
		channels[2].Type = channels[0].Type;
		channels[2].FuncList = channels[0].FuncList;
		channels[2].Default = channels[0].Default;
		channels[2].value[0] = supla_esp_gpio_relay_on(B_RELAY3_PORT);

		channels[3].Number = 3;
		channels[3].Type = channels[0].Type;
		channels[3].FuncList = channels[0].FuncList;
		channels[3].Default = channels[0].Default;
		channels[3].value[0] = supla_esp_gpio_relay_on(B_RELAY4_PORT);

	#endif
}

void ICACHE_FLASH_ATTR supla_esp_board_send_channel_values_with_delay(void *srpc) {

	supla_esp_channel_value_changed(0, supla_esp_gpio_relay_on(B_RELAY1_PORT));

	#if defined(__BOARD_wifisocket_x4)

	supla_esp_channel_value_changed(1, supla_esp_gpio_relay_on(B_RELAY2_PORT));
	supla_esp_channel_value_changed(2, supla_esp_gpio_relay_on(B_RELAY3_PORT));
	supla_esp_channel_value_changed(3, supla_esp_gpio_relay_on(B_RELAY4_PORT));

	#endif

}
