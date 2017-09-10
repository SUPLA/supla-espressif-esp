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

#define B_CFG_PORT          0
#define B_RELAY1_PORT       4
#define B_RELAY2_PORT       5
#define B_BTN1_PORT        13
#define B_BTN2_PORT        14
#define B_SENSOR_PORT1     12


void supla_esp_board_set_device_name(char *buffer, uint8 buffer_size) {
	
	ets_snprintf(buffer, buffer_size, "SUPLA-RS-MODULE");
	
}

void supla_esp_board_gpio_init(void) {
		
	supla_input_cfg[0].type = INPUT_TYPE_BUTTON;
	supla_input_cfg[0].gpio_id = B_CFG_PORT;
	supla_input_cfg[0].flags = INPUT_FLAG_PULLUP | INPUT_FLAG_CFG_BTN;
	
	supla_input_cfg[1].type = INPUT_TYPE_SENSOR;
	supla_input_cfg[1].gpio_id = B_SENSOR_PORT1;
	supla_input_cfg[1].channel = 1;
	
	supla_input_cfg[2].type = INPUT_TYPE_BUTTON_HILO;
	supla_input_cfg[2].gpio_id = B_BTN1_PORT;
	supla_input_cfg[2].relay_gpio_id = B_RELAY1_PORT;

	supla_input_cfg[3].type = INPUT_TYPE_BUTTON_HILO;
	supla_input_cfg[3].gpio_id = B_BTN2_PORT;
	supla_input_cfg[3].relay_gpio_id = B_RELAY2_PORT;

	// ---------------------------------------
	// ---------------------------------------

    supla_relay_cfg[0].gpio_id = B_RELAY1_PORT;
    supla_relay_cfg[0].channel = 0;
    
    supla_relay_cfg[1].gpio_id = B_RELAY2_PORT;
    supla_relay_cfg[1].channel = 0;

    supla_rs_cfg[0].up = &supla_relay_cfg[0];
    supla_rs_cfg[0].down = &supla_relay_cfg[1];

}

void supla_esp_board_set_channels(TDS_SuplaDeviceChannel_B *channels, unsigned char *channel_count) {
	
    *channel_count = 3;

	channels[0].Number = 0;
	channels[0].Type = SUPLA_CHANNELTYPE_RELAY;
	channels[0].FuncList =  SUPLA_BIT_RELAYFUNC_CONTROLLINGTHEROLLERSHUTTER;
	channels[0].Default = SUPLA_CHANNELFNC_CONTROLLINGTHEROLLERSHUTTER;
	channels[0].value[0] = (*supla_rs_cfg[0].position)-1;

	channels[1].Number = 1;
	channels[1].Type = SUPLA_CHANNELTYPE_SENSORNO;


	channels[2].Number = 2;
	channels[2].Type = SUPLA_CHANNELTYPE_THERMOMETERDS18B20;
	channels[2].FuncList = 0;
	channels[2].Default = 0;

	supla_get_temperature(channels[2].value);


}

void supla_esp_board_send_channel_values_with_delay(void *srpc) {

}
