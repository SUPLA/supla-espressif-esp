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

#include "board_stub.h"
#include <os_type.h>
#include <proto.h>
#include <assert.h>

#include <supla_esp.h>

#include <user_interface.h>

const uint8_t rsa_public_key_bytes[RSA_NUM_BYTES];

testBoardGpioInitCb *gpioInitCb = NULL;
int upTime = 1100;
int downTime = 1200;
int startupTimeDelay = 0;

void supla_esp_board_send_channel_values_with_delay(void *srpc){};
void supla_esp_board_set_device_name(char *buffer, uint8 buffer_size){};
void supla_esp_board_set_channels(TDS_SuplaDeviceChannel_C *channels,
                                  unsigned char *channel_count) {};

void supla_esp_board_gpio_init(void) {
  if (gpioInitCb) {
    gpioInitCb();
  }
};

bool supla_esp_board_is_rs_in_move(supla_roller_shutter_cfg_t *rs_cfg) {

  assert(rs_cfg);
  assert(rs_cfg->up_time == 0 || rs_cfg->down_time == 0);
  unsigned int t = system_get_time();
  if (t - rs_cfg->start_time < startupTimeDelay * 1000) {
    return false;
  }
  if (1 == __supla_esp_gpio_relay_is_hi(rs_cfg->up)) {
    if (t - rs_cfg->start_time >= (upTime + startupTimeDelay) * 1000) {
      return false;
    }
  }

  if (1 == __supla_esp_gpio_relay_is_hi(rs_cfg->down)) {
    if (t - rs_cfg->start_time >= (downTime + startupTimeDelay) * 1000) {
      return false;
    }
  }

  if (t - rs_cfg->start_time > 0) {
    return true;
  }

  return false;
}

void supla_esp_board_calcfg_request(TSD_DeviceCalCfgRequest *request) {}
