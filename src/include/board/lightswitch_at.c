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

void supla_esp_board_set_device_name(char *buffer, uint8 buffer_size) {

  ets_snprintf(buffer, buffer_size, "SUPLA-LIGHTSWITCH-AT");
}

const uint8_t rsa_public_key_bytes[512] = {};

void supla_esp_board_gpio_init(void) {

  supla_input_cfg[0].gpio_id = B_BTN1_PORT;
  supla_input_cfg[0].flags = INPUT_FLAG_PULLUP | INPUT_FLAG_CFG_BTN;
  supla_input_cfg[0].relay_gpio_id = B_RELAY1_PORT;
  supla_input_cfg[0].channel = 1;

  if (supla_esp_cfg.CfgButtonType == BTN_TYPE_MONOSTABLE) {
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[0].action_trigger_cap =
      SUPLA_ACTION_CAP_HOLD |
      SUPLA_ACTION_CAP_SHORT_PRESS_x1 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x3 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x5;
  } else {
    supla_input_cfg[0].type = INPUT_TYPE_BTN_BISTABLE;
    supla_input_cfg[0].action_trigger_cap =
      SUPLA_ACTION_CAP_TURN_ON   |
      SUPLA_ACTION_CAP_TURN_OFF  |
      SUPLA_ACTION_CAP_TOGGLE_x1 |
      SUPLA_ACTION_CAP_TOGGLE_x2 |
      SUPLA_ACTION_CAP_TOGGLE_x3 |
      SUPLA_ACTION_CAP_TOGGLE_x4 |
      SUPLA_ACTION_CAP_TOGGLE_x5;
  }

  supla_relay_cfg[0].gpio_id = B_RELAY1_PORT;
  supla_relay_cfg[0].flags = RELAY_FLAG_RESTORE_FORCE;
  supla_relay_cfg[0].channel = 0;

}

void supla_esp_board_set_channels(TDS_SuplaDeviceChannel_C *channels,
    unsigned char *channel_count) {

  *channel_count = 2;

  channels[0].Number = 0;
  channels[0].Type = SUPLA_CHANNELTYPE_RELAY;
  channels[0].Flags = SUPLA_CHANNEL_FLAG_CHANNELSTATE;
  channels[0].FuncList = SUPLA_BIT_FUNC_LIGHTSWITCH;
  channels[0].Default = SUPLA_CHANNELFNC_LIGHTSWITCH;
  channels[0].value[0] = supla_esp_gpio_relay_on(B_RELAY1_PORT);

  channels[1].Number = 1;
  channels[1].Type = SUPLA_CHANNELTYPE_ACTIONTRIGGER;
  channels[1].FuncList = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  channels[1].Default = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  channels[1].ActionTriggerCaps = supla_input_cfg[0].action_trigger_cap;
  channels[1].Flags = SUPLA_CHANNEL_FLAG_CHANNELSTATE;
  channels[1].actionTriggerProperties.relatedChannelNumber = 1;
  channels[1].actionTriggerProperties.disablesLocalOperation =
    SUPLA_ACTION_CAP_TOGGLE_x1 | SUPLA_ACTION_CAP_SHORT_PRESS_x1;

}

void supla_esp_board_send_channel_values_with_delay(void *srpc) {
  supla_esp_channel_value_changed(0, supla_esp_gpio_relay_on(B_RELAY1_PORT));
}
