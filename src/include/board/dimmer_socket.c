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
 
#include "dimmer_socket.h"
#include "supla_esp_devconn.h" 
 
//#include "supla_esp.h"

uint8 dimmer_brightness = 0;

#define B_CFG_PORT          0
#define B_RELAY1_PORT       4
#define B_RELAY2_PORT       5

//#define B_SENSOR_PORT1     12


void ICACHE_FLASH_ATTR supla_esp_board_set_device_name(char *buffer, uint8 buffer_size) {
	
	ets_snprintf(buffer, buffer_size, "DIMMER SOCKET");
	
}

void ICACHE_FLASH_ATTR supla_esp_board_gpio_init(void) {
		
	supla_input_cfg[0].type = supla_esp_cfg.CfgButtonType = INPUT_TYPE_BTN_MONOSTABLE;
	supla_input_cfg[0].gpio_id = B_CFG_PORT;
	supla_input_cfg[0].flags = INPUT_FLAG_PULLUP | INPUT_FLAG_CFG_BTN;
	
	/*supla_input_cfg[1].type = INPUT_TYPE_SENSOR;
	supla_input_cfg[1].gpio_id = B_SENSOR_PORT1;
	supla_input_cfg[1].channel = 2;*/
	
	// ---------------------------------------
	// ---------------------------------------

    supla_relay_cfg[0].gpio_id = B_RELAY1_PORT;
    supla_relay_cfg[0].flags = RELAY_FLAG_RESET;
    supla_relay_cfg[0].channel = 0;
    
	supla_relay_cfg[1].gpio_id = B_RELAY2_PORT;
    supla_relay_cfg[1].flags = RELAY_FLAG_RESET;
    supla_relay_cfg[1].channel = 1;
}

void ICACHE_FLASH_ATTR supla_esp_board_pwm_init(void) {
	supla_esp_channel_set_rgbw_value(0, 0, 0, supla_esp_state.brightness[0], 0, 0);
	//supla_esp_pwm_set_percent_duty(0, 100, 0);
}

void ICACHE_FLASH_ATTR supla_esp_board_set_channels(TDS_SuplaDeviceChannel_B *channels, unsigned char *channel_count) {
	
    *channel_count = 3;
	
	channels[0].Type = SUPLA_CHANNELTYPE_DIMMER;
	channels[0].Number = 0;
	supla_esp_channel_rgbw_to_value(channels[0].value, 0, 0, supla_esp_state.brightness[0]); 

	channels[1].Number = 1;
	channels[1].Type = SUPLA_CHANNELTYPE_RELAY;
	channels[1].FuncList =  SUPLA_BIT_RELAYFUNC_POWERSWITCH \
								| SUPLA_BIT_RELAYFUNC_LIGHTSWITCH;
	channels[1].Default = SUPLA_CHANNELFNC_POWERSWITCH;
	channels[1].value[0] = supla_esp_gpio_relay_on(B_RELAY1_PORT);
	
	channels[2].Number = 2;
	channels[2].Type = SUPLA_CHANNELTYPE_RELAY;
	channels[2].FuncList = SUPLA_BIT_RELAYFUNC_POWERSWITCH \
								| SUPLA_BIT_RELAYFUNC_LIGHTSWITCH;
	channels[2].Default = SUPLA_CHANNELFNC_POWERSWITCH;
	channels[2].value[0] = supla_esp_gpio_relay_on(B_RELAY2_PORT);

	/*channels[3].Number = 3;
	channels[3].Type = SUPLA_CHANNELTYPE_SENSORNO;
	channels[3].FuncList = 0;
	channels[3].Default = 0;
	channels[3].value[0] = 0;*/

	

}

char ICACHE_FLASH_ATTR supla_esp_board_set_rgbw_value(int ChannelNumber, int *Color, float *ColorBrightness, float *Brightness) {

	dimmer_brightness = *Brightness;
	
	if ( dimmer_brightness > 100 )
		dimmer_brightness = 100;
	
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
	supla_esp_channel_value_changed(2, supla_esp_gpio_relay_on(B_RELAY2_PORT));

}