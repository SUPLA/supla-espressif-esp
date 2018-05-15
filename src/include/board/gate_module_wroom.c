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
#include "supla_ds18b20.h"

const uint8_t rsa_public_key_bytes[512] = {
    0xd7, 0x81, 0x53, 0xf7, 0x47, 0x30, 0x5b, 0x63,
    0xfb, 0x17, 0xa8, 0xa0, 0x46, 0x94, 0x7f, 0xcf,
    0xab, 0x46, 0x2b, 0xe1, 0xf9, 0x46, 0x3f, 0xdd,
    0x45, 0x4f, 0x17, 0x15, 0x35, 0xa0, 0x2f, 0x04,
    0xc9, 0x29, 0xa8, 0x42, 0x2b, 0xa0, 0x7f, 0x10,
    0x43, 0x61, 0x37, 0x02, 0x91, 0x87, 0xd7, 0x78,
    0x1b, 0xb9, 0x2e, 0xa3, 0x87, 0x83, 0xaa, 0x5a,
    0x28, 0xce, 0xb7, 0xab, 0x95, 0xd3, 0x63, 0x52,
    0xb5, 0xbf, 0x91, 0xc3, 0x3d, 0x17, 0x95, 0x71,
    0xa6, 0xea, 0x11, 0xa7, 0xa3, 0x8f, 0x49, 0x6a,
    0xdc, 0x5e, 0x43, 0xc9, 0xce, 0xf2, 0xfb, 0xf7,
    0xd0, 0x20, 0xf3, 0x36, 0xe1, 0x88, 0x56, 0x85,
    0xfa, 0xb0, 0xf1, 0x94, 0x9e, 0x67, 0x2a, 0x8c,
    0x61, 0xa4, 0x62, 0x5d, 0x42, 0x7a, 0x7b, 0xf3,
    0x9f, 0xd6, 0xe7, 0x8c, 0x09, 0x72, 0x2f, 0x3b,
    0xc0, 0xe6, 0xe5, 0x8f, 0x47, 0x23, 0xdb, 0x91,
    0x0c, 0x07, 0xcf, 0x98, 0x9a, 0xb9, 0x44, 0x4c,
    0x46, 0xca, 0x1a, 0x4b, 0xb6, 0xe8, 0x5e, 0xbd,
    0xb1, 0x67, 0x41, 0x16, 0x14, 0xe0, 0x26, 0x0e,
    0x78, 0x23, 0x7e, 0x5e, 0x09, 0xa1, 0xb9, 0x5e,
    0x3f, 0x52, 0xdc, 0xb4, 0x1e, 0x67, 0xc0, 0xff,
    0x04, 0xb6, 0x5e, 0x9c, 0xb7, 0xb7, 0xed, 0xd4,
    0xbd, 0x15, 0xa9, 0x30, 0xd7, 0xc2, 0x86, 0x00,
    0x18, 0x1d, 0x75, 0x6d, 0xa4, 0xbb, 0xb8, 0x00,
    0xed, 0x85, 0x75, 0xda, 0x4b, 0x1e, 0xeb, 0xb1,
    0x70, 0xce, 0x72, 0x1d, 0x1d, 0xad, 0x71, 0xb6,
    0xf9, 0x88, 0xa9, 0x57, 0x66, 0x36, 0x28, 0xd8,
    0xd1, 0xa8, 0xcd, 0xcd, 0x2d, 0xfc, 0xb9, 0x6d,
    0xf5, 0x23, 0x28, 0x32, 0x8b, 0xda, 0x6a, 0xf2,
    0x50, 0x6e, 0xe1, 0x68, 0x0b, 0xb6, 0x60, 0x6c,
    0x85, 0x8b, 0x56, 0x16, 0xec, 0x22, 0xcc, 0x91,
    0x51, 0xcd, 0x05, 0x07, 0x3e, 0xe8, 0xe4, 0xa5,
    0x6b, 0x92, 0x97, 0x79, 0xe7, 0x0a, 0xff, 0xd6,
    0xf7, 0x0a, 0x12, 0x2d, 0xc7, 0x27, 0xab, 0x98,
    0x76, 0x19, 0x2b, 0x5e, 0xd8, 0x95, 0x0d, 0x63,
    0xbf, 0x89, 0xe1, 0xfb, 0x71, 0x34, 0x70, 0xbf,
    0x09, 0x6d, 0x09, 0x8e, 0x4c, 0x80, 0x34, 0xf0,
    0xd8, 0x45, 0xd9, 0x41, 0xb6, 0xa8, 0xe8, 0xc5,
    0x1f, 0xa5, 0x6b, 0x3e, 0x49, 0x91, 0x6a, 0xc4,
    0x25, 0x47, 0x49, 0x5e, 0x9f, 0x7e, 0x6a, 0x9a,
    0x53, 0x20, 0xfc, 0xce, 0x79, 0xf9, 0xcd, 0x71,
    0x69, 0x8d, 0xd3, 0x06, 0xab, 0xb7, 0xcd, 0xaf,
    0xfa, 0x39, 0x11, 0x8a, 0xbc, 0xf5, 0xa2, 0xc2,
    0x86, 0x43, 0x9f, 0x4f, 0x9a, 0x70, 0xd2, 0x8a,
    0xcc, 0x42, 0x35, 0x90, 0x32, 0xa4, 0x9e, 0x11,
    0x7e, 0x3e, 0x04, 0x74, 0xc9, 0x09, 0x8e, 0xda,
    0x50, 0x4b, 0x5e, 0x25, 0xff, 0xd7, 0xc2, 0xef,
    0x94, 0x91, 0xd2, 0x51, 0x5b, 0x53, 0xd4, 0x24,
    0x98, 0xb1, 0x59, 0x6f, 0x40, 0xfa, 0x00, 0xf3,
    0xca, 0xc1, 0x8a, 0x4b, 0x7b, 0x20, 0xf5, 0x4c,
    0x96, 0xc4, 0xd9, 0x02, 0x1c, 0x9d, 0x90, 0x3c,
    0xa0, 0xda, 0x90, 0xd8, 0x90, 0x13, 0x37, 0x4e,
    0xd3, 0xba, 0xb2, 0xb6, 0x5a, 0xbe, 0x12, 0x3b,
    0x15, 0x6b, 0x6b, 0x8b, 0x89, 0x95, 0xe2, 0x4a,
    0xe2, 0xd6, 0x3e, 0xec, 0xaf, 0x92, 0x0e, 0x68,
    0xdf, 0x0b, 0xb2, 0x78, 0x33, 0xab, 0x86, 0x7a,
    0x83, 0x4f, 0xa5, 0x06, 0xca, 0xf9, 0x4f, 0x8f,
    0xf7, 0xc3, 0xac, 0x22, 0x96, 0xc6, 0x71, 0xa2,
    0x3c, 0xd6, 0x8c, 0xcc, 0x5c, 0x0b, 0x90, 0xa8,
    0xd2, 0x70, 0x6b, 0x56, 0xa9, 0x6c, 0xb8, 0xd9,
    0x02, 0xa5, 0x2a, 0xbf, 0xce, 0x45, 0x20, 0x7d,
    0x2d, 0xec, 0x74, 0x9d, 0xc8, 0x99, 0xa0, 0x4a,
    0xd2, 0x90, 0xc7, 0xba, 0x57, 0x73, 0xd2, 0xfa,
    0xa8, 0x3a, 0xeb, 0x99, 0x77, 0xbd, 0x0d, 0xb3
};

#if defined(__BOARD_gate_module_wroom)

	#define B_CFG_PORT         13
	#define B_RELAY1_PORT       4
	#define B_RELAY2_PORT      14

	#define B_SENSOR_PORT1      5
	#define B_SENSOR_PORT2     12

#else

	#define B_CFG_PORT         13
	#define B_RELAY1_PORT       4
	#define B_RELAY2_PORT       5

	#define B_SENSOR_PORT1     12
	#define B_SENSOR_PORT2     14

#endif

void ICACHE_FLASH_ATTR supla_esp_board_set_device_name(char *buffer, uint8 buffer_size) {
	
	ets_snprintf(buffer, buffer_size, "SUPLA-GATE-MODULE");
	
}

void ICACHE_FLASH_ATTR supla_esp_board_gpio_init(void) {
		
	supla_input_cfg[0].type = supla_esp_cfg.CfgButtonType == BTN_TYPE_BISTABLE ? INPUT_TYPE_BTN_BISTABLE : INPUT_TYPE_BTN_MONOSTABLE;
	supla_input_cfg[0].gpio_id = B_CFG_PORT;
	supla_input_cfg[0].flags = INPUT_FLAG_PULLUP | INPUT_FLAG_CFG_BTN;
	
	supla_input_cfg[1].type = INPUT_TYPE_SENSOR;
	supla_input_cfg[1].gpio_id = B_SENSOR_PORT1;
	supla_input_cfg[1].channel = 2;
	
	supla_input_cfg[2].type = INPUT_TYPE_SENSOR;
	supla_input_cfg[2].gpio_id = B_SENSOR_PORT2;
	supla_input_cfg[2].channel = 3;
	
	// ---------------------------------------
	// ---------------------------------------

    supla_relay_cfg[0].gpio_id = B_RELAY1_PORT;
    supla_relay_cfg[0].flags = RELAY_FLAG_RESET;
    supla_relay_cfg[0].channel = 0;
    
    supla_relay_cfg[1].gpio_id = B_RELAY2_PORT;
    supla_relay_cfg[1].flags = RELAY_FLAG_RESET;
    supla_relay_cfg[1].channel = 1;

}

void  supla_esp_board_set_channels(TDS_SuplaDeviceChannel_B *channels, unsigned char *channel_count) {
	
	*channel_count = 5;

	channels[0].Number = 0;
	channels[0].Type = SUPLA_CHANNELTYPE_RELAY;
	channels[0].FuncList =  SUPLA_BIT_RELAYFUNC_CONTROLLINGTHEGATEWAYLOCK \
								| SUPLA_BIT_RELAYFUNC_CONTROLLINGTHEGATE \
								| SUPLA_BIT_RELAYFUNC_CONTROLLINGTHEGARAGEDOOR \
								| SUPLA_BIT_RELAYFUNC_CONTROLLINGTHEDOORLOCK;
	channels[0].Default = 0;
	channels[0].value[0] = supla_esp_gpio_relay_on(B_RELAY1_PORT);

	channels[1].Number = 1;
	channels[1].Type = channels[0].Type;
	channels[1].FuncList = channels[0].FuncList;
	channels[1].Default = channels[0].Default;
	channels[1].value[0] = supla_esp_gpio_relay_on(B_RELAY2_PORT);

	channels[2].Number = 2;
	channels[2].Type = SUPLA_CHANNELTYPE_SENSORNO;
	channels[2].FuncList = 0;
	channels[2].Default = 0;
	channels[2].value[0] = 0;

	channels[3].Number = 3;
	channels[3].Type = SUPLA_CHANNELTYPE_SENSORNO;
	channels[3].FuncList = 0;
	channels[3].Default = 0;
	channels[3].value[0] = 0;

	channels[4].Number = 4;
	channels[4].Type = SUPLA_CHANNELTYPE_THERMOMETERDS18B20;

	channels[4].FuncList = 0;
	channels[4].Default = 0;

	supla_get_temperature(channels[4].value);


	
}

void ICACHE_FLASH_ATTR supla_esp_board_send_channel_values_with_delay(void *srpc) {

	supla_esp_channel_value_changed(2, gpio__input_get(B_SENSOR_PORT1));
	supla_esp_channel_value_changed(3, gpio__input_get(B_SENSOR_PORT2));

}
