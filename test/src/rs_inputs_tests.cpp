/* Copyright (C) AC SOFTWARE SP. Z O.O.

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

#include <eagle_soc_mock.h>
#include <eagle_soc_stub.h>
#include <gtest/gtest.h>
#include <time_mock.h>
#include <supla_esp_mock.h>
#include <srpc_mock.h>

extern "C" {
#include "board_stub.h"
#include <osapi.h>
#include <supla_esp_cfg.h>
#include <supla_esp_gpio.h>
#include <supla_esp_devconn.h>
#include <supla_esp_cfgmode.h>
}

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::SaveArg;
using ::testing::Invoke;
using ::testing::DoAll;
using ::testing::ElementsAreArray;

static int gpioConfigId = 0;

// method will be called by supla_esp_gpio_init method in order to initialize
// gpio input/outputs board configuration (supla_esb_board_gpio_init)
void gpioCallbackRsInputs() {
  supla_input_cfg[0].flags = 0;
  supla_input_cfg[0].gpio_id = 1;
  supla_input_cfg[0].type = 0;
  supla_input_cfg[0].channel = 255;
  supla_input_cfg[0].relay_gpio_id = 3;

  supla_input_cfg[1].type = 0;
  supla_input_cfg[1].gpio_id = 2;
  supla_input_cfg[1].flags = 0;
  supla_input_cfg[1].channel = 255;
  supla_input_cfg[1].relay_gpio_id = 4;

  // up
  supla_relay_cfg[0].gpio_id = 3;
  supla_relay_cfg[0].channel = 0;
  supla_relay_cfg[0].flags = 0;
  supla_relay_cfg[0].channel_flags = 0;

  // down
  supla_relay_cfg[1].gpio_id = 4;
  supla_relay_cfg[1].channel = 0;
  supla_relay_cfg[1].flags = 0;
  supla_relay_cfg[1].channel_flags = 0;

  supla_rs_cfg[0].up = &supla_relay_cfg[0];
  supla_rs_cfg[0].down = &supla_relay_cfg[1];
  *supla_rs_cfg[0].position = 0;
  *supla_rs_cfg[0].full_closing_time = 0;
  *supla_rs_cfg[0].full_opening_time = 0;
  supla_rs_cfg[0].delayed_trigger.value = 0;

  if (gpioConfigId == 0) {
  } else if (gpioConfigId == 1) {
  } else if (gpioConfigId == 2) {
  } else if (gpioConfigId == 3) {
  } else if (gpioConfigId == 4) {
  } else if (gpioConfigId == 5) {
  } else if (gpioConfigId == 6) {
  } else if (gpioConfigId == 7) {
  } else if (gpioConfigId == 8) {
  } else if (gpioConfigId == 9) {
  } else if (gpioConfigId == 10) {
  } else if (gpioConfigId == 11) {
  } else if (gpioConfigId == 12) {
  } else {
    assert(false);
  }
}

TSD_SuplaRegisterDeviceResult regRsInputs;

char custom_srpc_getdata_rs_inputs(void *_srpc, TsrpcReceivedData *rd,
    unsigned _supla_int_t rr_id) {
  rd->call_type = SUPLA_SD_CALL_REGISTER_DEVICE_RESULT;
  rd->data.sd_register_device_result = &regRsInputs;
  return 1;
}

class RsInputsFixture : public ::testing::Test {
  public:
    TimeMock time;
    EagleSocStub eagleStub;
    BoardMock board;
    int curTime;
    SrpcMock srpc;

    void SetUp() override {
      curTime = 10000; 
      currentDeviceState = STATE_UNKNOWN;
      EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));
      memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
      memset(&supla_esp_state, 0, sizeof(SuplaEspState));
      memset(&supla_relay_cfg, 0, sizeof(supla_relay_cfg));
      memset(&supla_rs_cfg, 0, sizeof(supla_rs_cfg));
      gpioInitCb = gpioCallbackRsInputs;
      supla_esp_gpio_init_time = 0;
      EXPECT_CALL(srpc, srpc_params_init(_));
      EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return((void *)1));
      EXPECT_CALL(srpc, srpc_set_proto_version(_, 16));

      regRsInputs.result_code = SUPLA_RESULTCODE_TRUE;

      EXPECT_CALL(srpc, srpc_getdata(_, _, _))
        .WillOnce(DoAll(Invoke(custom_srpc_getdata_rs_inputs), Return(1)));
      EXPECT_CALL(srpc, srpc_rd_free(_));
      EXPECT_CALL(srpc, srpc_free(_));
      EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
      EXPECT_CALL(srpc, srpc_dcs_async_set_activity_timeout(_, _));

      supla_esp_devconn_init();
      supla_esp_srpc_init();
      ASSERT_NE(srpc.on_remote_call_received, nullptr);

      srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
      EXPECT_EQ(supla_esp_devconn_is_registered(), 1);

    }

    void TearDown() override {
      currentDeviceState = STATE_UNKNOWN;
      memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
      memset(&supla_esp_state, 0, sizeof(SuplaEspState));
      memset(&supla_relay_cfg, 0, sizeof(supla_relay_cfg));
      memset(&supla_rs_cfg, 0, sizeof(supla_rs_cfg));
      supla_esp_gpio_init_time = 0;
      gpioConfigId = 0;
      supla_esp_cfgmode_clear_vars();
      supla_esp_gpio_clear_vars();
      supla_esp_restart_on_cfg_press = 0;
      ets_clear_isr();
      cleanupTimers();
      gpioInitCb = nullptr;
      supla_esp_devconn_release();
    }
};


TEST_F(RsInputsFixture, Dummy) {
  EXPECT_TRUE(true);
}

// TODO add tests:
// - RS inputs for mono and bistable with and without AT
