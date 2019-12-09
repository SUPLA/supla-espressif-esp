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

#define B_CFG_PORT          5
#define B_RELAY1_PORT       4
#define B_RELAY2_PORT      13

#define B_SENSOR_PORT1     12
#define B_SENSOR_PORT2     14

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

void ICACHE_FLASH_ATTR supla_esp_board_set_channels(TDS_SuplaDeviceChannel_B *channels, unsigned char *channel_count) {
	
    *channel_count = 5;

	channels[0].Number = 0;
	channels[0].Type = SUPLA_CHANNELTYPE_RELAY;
	channels[0].FuncList =  SUPLA_BIT_FUNC_CONTROLLINGTHEGATEWAYLOCK \
								| SUPLA_BIT_FUNC_CONTROLLINGTHEGATE \
								| SUPLA_BIT_FUNC_CONTROLLINGTHEGARAGEDOOR \
								| SUPLA_BIT_FUNC_CONTROLLINGTHEDOORLOCK;
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

	#if defined(__BOARD_gate_module_dht11)
		channels[4].Type = SUPLA_CHANNELTYPE_DHT11;
	#elif defined(__BOARD_gate_module_dht22)
		channels[4].Type = SUPLA_CHANNELTYPE_DHT22;
	#else
		channels[4].Type = SUPLA_CHANNELTYPE_THERMOMETERDS18B20;
	#endif


	channels[4].FuncList = 0;
	channels[4].Default = 0;

    #if defined(__BOARD_gate_module_dht11) || defined(__BOARD_gate_module_dht22)
	supla_get_temp_and_humidity(channels[4].value);
	#else
	supla_get_temperature(channels[4].value);
	#endif


}

void ICACHE_FLASH_ATTR supla_esp_board_send_channel_values_with_delay(void *srpc) {

	supla_esp_channel_value_changed(2, gpio__input_get(B_SENSOR_PORT1));
	supla_esp_channel_value_changed(3, gpio__input_get(B_SENSOR_PORT2));

}
