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
 TEST
 */

 // moj klucz
const uint8_t rsa_public_key_bytes[512] = {
    0xf1, 0xa2, 0x3b, 0xb2, 0x4d, 0x2e, 0x22, 0x42,
    0xa1, 0x75, 0x60, 0xc3, 0xa9, 0xbc, 0xb3, 0x77,
    0x82, 0xff, 0x55, 0x76, 0x09, 0x2d, 0x95, 0x06,
    0x32, 0x75, 0xc7, 0x8d, 0xda, 0x7f, 0x99, 0xb6,
    0xce, 0xd2, 0x32, 0x87, 0x6e, 0x6a, 0x71, 0x52,
    0x36, 0x5d, 0x17, 0xdb, 0x56, 0x30, 0xda, 0x44,
    0xa1, 0x01, 0x5a, 0x74, 0x9e, 0xaa, 0xdd, 0x79,
    0x46, 0xf2, 0x3c, 0x61, 0x3a, 0xd3, 0x5b, 0xaf,
    0xdd, 0x13, 0x1b, 0x6c, 0x41, 0xb3, 0x91, 0x3f,
    0x53, 0x09, 0xfb, 0xed, 0xa1, 0x13, 0x76, 0xb1,
    0x3d, 0x3d, 0xf3, 0x05, 0x56, 0x42, 0x3b, 0x31,
    0xc9, 0x46, 0x79, 0x4f, 0xd5, 0xef, 0x77, 0xcb,
    0xce, 0xa7, 0x1c, 0xdb, 0x59, 0x49, 0x7d, 0xd5,
    0x11, 0xd2, 0xec, 0xb2, 0x81, 0xd1, 0x5c, 0xa6,
    0x20, 0x7b, 0x9a, 0x16, 0x31, 0x92, 0x13, 0xfd,
    0xa4, 0xd2, 0x53, 0x89, 0x69, 0x6d, 0x18, 0xd3,
    0x18, 0x18, 0xe1, 0x35, 0xbb, 0xa9, 0xa4, 0xc7,
    0x89, 0x39, 0xc3, 0x21, 0xd3, 0xa2, 0x9a, 0x85,
    0xbc, 0x8c, 0x32, 0x69, 0xac, 0xf5, 0x0e, 0x41,
    0x42, 0xcc, 0xc3, 0xba, 0x69, 0x44, 0x49, 0x0f,
    0xd3, 0xd4, 0x32, 0xd4, 0x33, 0xf5, 0x85, 0xfd,
    0x5f, 0xe3, 0xf1, 0x24, 0x1d, 0xb2, 0x95, 0xee,
    0xc8, 0x3d, 0x40, 0xba, 0xc1, 0xf6, 0xe9, 0x1a,
    0xaa, 0xbb, 0x4c, 0x31, 0x4b, 0x33, 0xf6, 0xaa,
    0xad, 0x16, 0x5a, 0x02, 0xfb, 0xf0, 0x3e, 0xf5,
    0xba, 0xfa, 0xd4, 0x9b, 0xb3, 0x22, 0xf5, 0x68,
    0x85, 0x41, 0x90, 0x5c, 0x88, 0x33, 0x4c, 0x6e,
    0x6d, 0xed, 0xa2, 0x4a, 0xfe, 0xf1, 0x39, 0xca,
    0xbd, 0x7c, 0x6c, 0xb0, 0x2a, 0x2e, 0x91, 0x86,
    0xb6, 0x5a, 0x25, 0x23, 0x8f, 0xd2, 0xd2, 0xb5,
    0x91, 0x50, 0xff, 0x5b, 0xd3, 0x0f, 0x50, 0xf1,
    0x6f, 0x66, 0xbe, 0x0b, 0x60, 0x95, 0x24, 0x6e,
    0x7b, 0x48, 0xd3, 0x74, 0x5e, 0xf3, 0x91, 0x5c,
    0x69, 0xbf, 0xf5, 0x8a, 0xe8, 0xe3, 0xfd, 0xb2,
    0x19, 0x37, 0x2e, 0xc8, 0xe8, 0x2e, 0x28, 0xfe,
    0x63, 0x85, 0x1d, 0x1c, 0x40, 0xfe, 0xf0, 0x9a,
    0x60, 0x21, 0x76, 0x54, 0xdd, 0xd7, 0xd0, 0xf2,
    0x97, 0xa2, 0x98, 0xc3, 0x10, 0x60, 0x72, 0xbb,
    0xbc, 0xf6, 0x19, 0x6d, 0x58, 0x8d, 0x79, 0xe9,
    0xdf, 0xc5, 0x81, 0xe4, 0xb8, 0xb9, 0x92, 0x16,
    0xb8, 0xd1, 0x28, 0xb3, 0x35, 0x8d, 0xf3, 0x9e,
    0x8d, 0x1f, 0x6d, 0xaf, 0x5f, 0xef, 0x6f, 0x99,
    0xb8, 0xc1, 0xca, 0x5a, 0x8a, 0x3d, 0x25, 0x27,
    0xff, 0xc6, 0x09, 0xe6, 0x0c, 0x34, 0x72, 0x98,
    0x3b, 0x39, 0x14, 0x77, 0xbc, 0x96, 0x1a, 0xe4,
    0xc4, 0x97, 0x19, 0x56, 0xf9, 0xdb, 0xe7, 0x88,
    0x83, 0x00, 0x5e, 0xad, 0x3f, 0xca, 0x8a, 0x21,
    0x31, 0xca, 0x8e, 0xf2, 0x13, 0xd2, 0xec, 0x80,
    0xf4, 0x85, 0x8f, 0x5e, 0xd3, 0xf8, 0x56, 0x0a,
    0x9b, 0x2a, 0xee, 0xf6, 0x2a, 0xa5, 0x0c, 0x84,
    0x07, 0x48, 0xf6, 0x2f, 0xd4, 0xfe, 0x6d, 0xf3,
    0x0b, 0x8a, 0x53, 0xda, 0xfc, 0x4c, 0x24, 0xaf,
    0x24, 0xab, 0x4c, 0xf2, 0x24, 0x48, 0x35, 0xac,
    0x6d, 0x53, 0xf0, 0x69, 0x47, 0xcb, 0xf6, 0xb2,
    0x3b, 0xf2, 0x3c, 0x96, 0x3d, 0x71, 0x81, 0xd9,
    0x89, 0x25, 0xe2, 0x68, 0xb9, 0x93, 0x7a, 0x1f,
    0x36, 0x40, 0xb4, 0x04, 0x0c, 0x6a, 0x46, 0x30,
    0x56, 0x1a, 0x84, 0x88, 0x12, 0x6c, 0x61, 0xeb,
    0x99, 0x6e, 0xb8, 0xdb, 0x4d, 0x0f, 0xdc, 0xd9,
    0x43, 0x0d, 0xf1, 0x99, 0xc1, 0xf6, 0x98, 0xe4,
    0xa6, 0x0c, 0x2d, 0x8f, 0x35, 0x77, 0x6d, 0xd4,
    0x4f, 0x8f, 0x72, 0x9d, 0xc5, 0x1a, 0xe7, 0x0e,
    0x4b, 0x3d, 0x60, 0x1c, 0x1f, 0x01, 0x6a, 0x49,
    0x26, 0xb3, 0x92, 0x90, 0xa5, 0xd7, 0x4c, 0xd7
};
 
#include "dimmer.h"
#include "supla_esp_devconn.h"

uint8 dimmer_brightness = 0;

#define B_CFG_PORT         0
#define B_RELAY1_PORT      4

void ICACHE_FLASH_ATTR supla_esp_board_set_device_name(char *buffer, uint8 buffer_size) {
	ets_snprintf(buffer, buffer_size, "Dimmer");
}


void ICACHE_FLASH_ATTR supla_esp_board_gpio_init(void) {
		
	supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
	supla_input_cfg[0].gpio_id = B_CFG_PORT;
	supla_input_cfg[0].flags = INPUT_FLAG_PULLUP | INPUT_FLAG_CFG_BTN;
	
	supla_relay_cfg[1].gpio_id = B_RELAY1_PORT;
    supla_relay_cfg[1].flags = RELAY_FLAG_RESTORE_FORCE;
    supla_relay_cfg[1].channel = 1;
}

void ICACHE_FLASH_ATTR supla_esp_board_pwm_init(void) {
	supla_esp_channel_set_rgbw_value(0, 0, 0, supla_esp_state.brightness[0], 0, 0);
}

void ICACHE_FLASH_ATTR supla_esp_board_set_channels(TDS_SuplaDeviceChannel_B *channels, unsigned char *channel_count) {

	*channel_count = 2;

	channels[0].Type = SUPLA_CHANNELTYPE_DIMMER;
	channels[0].Number = 0;
	supla_esp_channel_rgbw_to_value(channels[0].value, 0, 0, supla_esp_state.brightness[0]);

	channels[1].Number = 1;
	channels[1].Type = SUPLA_CHANNELTYPE_RELAY;
	channels[1].FuncList = SUPLA_BIT_RELAYFUNC_POWERSWITCH \
								| SUPLA_BIT_RELAYFUNC_LIGHTSWITCH;
	channels[1].Default = SUPLA_CHANNELFNC_POWERSWITCH;
	channels[1].value[0] = supla_esp_gpio_relay_on(B_RELAY1_PORT);
}

char ICACHE_FLASH_ATTR supla_esp_board_set_rgbw_value(int ChannelNumber, int *Color, float *ColorBrightness, float *Brightness) {

	dimmer_brightness = *Brightness;
	
	int hi;
	hi = supla_esp_gpio_output_is_hi(B_RELAY1_PORT);
	
	if ( dimmer_brightness > 100 )
		dimmer_brightness = 100;
	
	if ( hi == 1 )
		supla_esp_pwm_set_percent_duty(50, 100, 0);
	
	supla_esp_pwm_set_percent_duty(dimmer_brightness, 100, 0);
	supla_log(LOG_DEBUG, "Set dimmer : %i", dimmer_brightness);
	
	

	return 1;
}


void ICACHE_FLASH_ATTR supla_esp_board_get_rgbw_value(int ChannelNumber, int *Color, float *ColorBrightness, float *Brightness) {

	if ( Brightness != NULL ) {
			*Brightness = dimmer_brightness;
			supla_log(LOG_DEBUG, "Get dimmer : %i", dimmer_brightness);
	}

}

void ICACHE_FLASH_ATTR supla_esp_board_send_channel_values_with_delay(void *srpc) {
	
	supla_esp_channel_value_changed(1, supla_esp_gpio_relay_on(B_RELAY1_PORT));
	
}

