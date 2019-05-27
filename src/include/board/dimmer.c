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

#include "dimmer.h"
#include "supla_esp_devconn.h"

uint8 dimmer_brightness = 0;

#define B_CFG_PORT         0

void ICACHE_FLASH_ATTR supla_esp_board_set_device_name(char *buffer, uint8 buffer_size) {
	ets_snprintf(buffer, buffer_size, "Dimmer");
}


void ICACHE_FLASH_ATTR supla_esp_board_gpio_init(void) {
		
	supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
	supla_input_cfg[0].gpio_id = B_CFG_PORT;
	supla_input_cfg[0].flags = INPUT_FLAG_PULLUP | INPUT_FLAG_CFG_BTN;
}

void ICACHE_FLASH_ATTR supla_esp_board_pwm_init(void) {
	supla_esp_channel_set_rgbw_value(0, 0, 0, supla_esp_state.brightness[0], 0, 0);
}

void ICACHE_FLASH_ATTR supla_esp_board_set_channels(TDS_SuplaDeviceChannel_C *channels, unsigned char *channel_count) {

	*channel_count = 1;

	channels[0].Type = SUPLA_CHANNELTYPE_DIMMER;
	channels[0].Number = 0;
	supla_esp_channel_rgbw_to_value(channels[0].value, 0, 0, supla_esp_state.brightness[0]);

}

char ICACHE_FLASH_ATTR supla_esp_board_set_rgbw_value(int ChannelNumber, int *Color, float *ColorBrightness, float *Brightness) {

	dimmer_brightness = *Brightness;
	
	if ( dimmer_brightness > 100 )
		dimmer_brightness = 100;
	
	supla_esp_pwm_set_percent_duty(dimmer_brightness, 100, 0);

	return 1;
}


void ICACHE_FLASH_ATTR supla_esp_board_get_rgbw_value(int ChannelNumber, int *Color, float *ColorBrightness, float *Brightness) {

	if ( Brightness != NULL ) {
			*Brightness = dimmer_brightness;
	}

}

void ICACHE_FLASH_ATTR supla_esp_board_send_channel_values_with_delay(void *srpc) {

}
