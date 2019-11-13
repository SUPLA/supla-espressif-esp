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
	
#include "public_key_in_c_code"		// moj klucz


void supla_esp_board_set_device_name(char *buffer, uint8 buffer_size) {

	ets_snprintf(buffer, buffer_size, "YUNSCHAN");

}


void supla_esp_board_gpio_init(void) {
		
	supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
	supla_input_cfg[0].gpio_id = B_CFG_PORT;
	supla_input_cfg[0].flags = INPUT_FLAG_PULLUP | INPUT_FLAG_CFG_BTN;
	supla_input_cfg[0].relay_gpio_id = B_RELAY1_PORT;
	supla_input_cfg[0].channel = 0;

	// ---------------------------------------
	// ---------------------------------------

    supla_relay_cfg[0].gpio_id = B_RELAY1_PORT;
    supla_relay_cfg[0].flags = RELAY_FLAG_RESTORE_FORCE;
    supla_relay_cfg[0].channel = 0;

	//---------------------------------------

	supla_relay_cfg[1].gpio_id = 20;
	supla_relay_cfg[1].channel = 1;
    

}



void supla_esp_board_set_channels(TDS_SuplaDeviceChannel_B *channels, unsigned char *channel_count) {

	*channel_count = 2;

	channels[0].Number = 0;
	channels[0].Type = SUPLA_CHANNELTYPE_RELAY;

	channels[0].FuncList = SUPLA_BIT_RELAYFUNC_POWERSWITCH \
								| SUPLA_BIT_RELAYFUNC_LIGHTSWITCH;

	channels[0].Default = SUPLA_CHANNELFNC_POWERSWITCH;

	channels[0].value[0] = supla_esp_gpio_relay_on(B_RELAY1_PORT);
	//---------------------------------------
	channels[1].Number = 1;
	channels[1].Type = SUPLA_CHANNELTYPE_RELAY;
	channels[1].FuncList = SUPLA_BIT_RELAYFUNC_POWERSWITCH;
	channels[1].Default = 0;
	channels[1].value[0] = supla_esp_gpio_relay_on(20);

}

void supla_esp_board_send_channel_values_with_delay(void *srpc) {

	supla_esp_channel_value_changed(0, supla_esp_gpio_relay_on(B_RELAY1_PORT));
	
}

void ICACHE_FLASH_ATTR supla_esp_board_on_connect(void) {
  
  if ( supla_esp_gpio_output_is_hi(4) == 1 ) { supla_esp_gpio_set_led(1, 0, 0); }
  if ( supla_esp_gpio_output_is_hi(4) == 0 ) { supla_esp_gpio_set_led(0, 0, 0); }
}

void ICACHE_FLASH_ATTR supla_esp_board_gpio_relay_switch(void* _input_cfg,
    char hi)
{

    supla_input_cfg_t* input_cfg = (supla_input_cfg_t*)_input_cfg;

    if (input_cfg->relay_gpio_id != 255) {

        // supla_log(LOG_DEBUG, "RELAY");

        supla_esp_gpio_relay_hi(input_cfg->relay_gpio_id, hi, 0);

        if (input_cfg->channel != 255)
            supla_esp_channel_value_changed(
                input_cfg->channel,
                supla_esp_gpio_relay_is_hi(input_cfg->relay_gpio_id));
    }
}

void ICACHE_FLASH_ATTR supla_esp_board_gpio_on_input_active(void* _input_cfg)
{

    supla_input_cfg_t* input_cfg = (supla_input_cfg_t*)_input_cfg;

  if ( input_cfg->type == INPUT_TYPE_BTN_MONOSTABLE 	//wlaczanie przy zboczu narastajacym
		|| input_cfg->type == INPUT_TYPE_BTN_BISTABLE ) {

		supla_log(LOG_DEBUG, "RELAY");
		supla_esp_board_gpio_relay_switch(input_cfg, 255);
		
		} else if ( input_cfg->type == INPUT_TYPE_SENSOR
				&&  input_cfg->channel != 255 ) {

		supla_esp_channel_value_changed(input_cfg->channel, 1);

	}

    input_cfg->last_state = 1;
}

void ICACHE_FLASH_ATTR
supla_esp_board_gpio_on_input_inactive(void* _input_cfg)
{

    supla_input_cfg_t* input_cfg = (supla_input_cfg_t*)_input_cfg;

    if ( input_cfg->type == INPUT_TYPE_BTN_BISTABLE ) {		//wlaczanie przy zboczu narastajacym

		supla_esp_board_gpio_relay_switch(input_cfg, 255);

    } else if ( input_cfg->type == INPUT_TYPE_SENSOR
			    &&  input_cfg->channel != 255 ) {
		supla_esp_channel_value_changed(input_cfg->channel, 0);

	}

    input_cfg->last_state = 0;
}