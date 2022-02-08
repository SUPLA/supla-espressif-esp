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
#include <srpc_mock.h>
#include <supla_esp_mock.h>
#include <time_mock.h>

extern "C" {
#include "board_stub.h"
#include <osapi.h>
#include <supla_esp_cfg.h>
#include <supla_esp_cfgmode.h>
#include <supla_esp_devconn.h>
#include <supla_esp_gpio.h>
#include <espconn.h>
}

using ::testing::_;
using ::testing::DoAll;
using ::testing::ElementsAreArray;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::SaveArg;


#define BUTTON_UP 1
#define BUTTON_DOWN 2
#define RELAY_UP 3
#define RELAY_DOWN 4

static int gpioConfigId = 0;

// method will be called by supla_esp_gpio_init method in order to initialize
// gpio input/outputs board configuration (supla_esb_board_gpio_init)
void gpioCallbackCalCfg() {
  supla_input_cfg[0].flags = INPUT_FLAG_CFG_BTN;
  supla_input_cfg[0].gpio_id = BUTTON_UP;
  supla_input_cfg[0].type = 0;
  supla_input_cfg[0].channel = 1;
  supla_input_cfg[0].relay_gpio_id = RELAY_UP;

  supla_input_cfg[1].type = 0;
  supla_input_cfg[1].gpio_id = BUTTON_DOWN;
  supla_input_cfg[1].flags = 0;  // INPUT_FLAG_CFG_BTN;
  supla_input_cfg[1].channel = 2;
  supla_input_cfg[1].relay_gpio_id = RELAY_DOWN;

  // up
  supla_relay_cfg[0].gpio_id = RELAY_UP;
  supla_relay_cfg[0].channel = 0;
  supla_relay_cfg[0].flags = 0;
  supla_relay_cfg[0].channel_flags = 0;

  // down
  supla_relay_cfg[1].gpio_id = RELAY_DOWN;
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
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[1].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[0].action_trigger_cap = SUPLA_ACTION_CAP_HOLD |
      SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x3;
    supla_input_cfg[1].action_trigger_cap = SUPLA_ACTION_CAP_HOLD |
      SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x3;
  } else {
    assert(false);
  }
}

TSD_SuplaRegisterDeviceResult regCalCfg;

char custom_srpc_getdata_calcfg(void *_srpc, TsrpcReceivedData *rd,
    unsigned _supla_int_t rr_id) {
  rd->call_type = SUPLA_SD_CALL_REGISTER_DEVICE_RESULT;
  rd->data.sd_register_device_result = &regCalCfg;
  return 1;
}

class CalCfgFixture : public ::testing::Test {
  public:
    TimeMock time;
    EagleSocStub eagleStub;
    BoardMock board;
    int curTime;
    SrpcMock srpc;

    void SetUp() override {
      cleanupTimers();
      supla_esp_gpio_clear_vars();
      curTime = 10000;
      currentDeviceState = STATE_UNKNOWN;
      EXPECT_CALL(time, system_get_time())
        .WillRepeatedly(ReturnPointee(&curTime));
      memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
      memset(&supla_esp_state, 0, sizeof(SuplaEspState));
      memset(&supla_relay_cfg, 0, sizeof(supla_relay_cfg));
      memset(&supla_rs_cfg, 0, sizeof(supla_rs_cfg));

      strncpy(supla_esp_cfg.Server, "test", 4);
      strncpy(supla_esp_cfg.Email, "test", 4);
      strncpy(supla_esp_cfg.WIFI_SSID, "test", 4);
      strncpy(supla_esp_cfg.WIFI_PWD, "test", 4);

      gpioInitCb = gpioCallbackCalCfg;
      supla_esp_gpio_init_time = 0;
      EXPECT_CALL(srpc, srpc_params_init(_));
      EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return((void *)1));
      EXPECT_CALL(srpc, srpc_set_proto_version(_, 17));

      regCalCfg.result_code = SUPLA_RESULTCODE_TRUE;

      EXPECT_CALL(srpc, srpc_getdata(_, _, _))
        .WillOnce(DoAll(Invoke(custom_srpc_getdata_calcfg), Return(1)));
      EXPECT_CALL(srpc, srpc_rd_free(_));
      EXPECT_CALL(srpc, srpc_free(_));
      EXPECT_CALL(srpc, srpc_iterate(_))
        .WillRepeatedly(Return(SUPLA_RESULT_TRUE));
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
      supla_esp_restart_on_cfg_press = 0;
      ets_clear_isr();
      cleanupTimers();
      gpioInitCb = nullptr;
      supla_esp_devconn_release();
      supla_esp_gpio_clear_vars();
    }

  void moveTime(const int timeMs) {
    for (int i = 0; i < timeMs / 1000; i++) {
      curTime += 1000000; // +1s
      executeTimers();
    }
  }
};

TEST_F(CalCfgFixture, RequestNotAuthorized) {
  gpioConfigId = 0;

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  {
    InSequence seq;

    EXPECT_CALL(
        srpc, valueChanged(_, 0, ElementsAreArray({255, 0, 0, 0, 0, 0, 0, 0})));

    EXPECT_CALL(srpc, srpc_ds_async_device_calcfg_result(
          _, 0, -1, SUPLA_CALCFG_CMD_ENTER_CFG_MODE,
          SUPLA_CALCFG_RESULT_UNAUTHORIZED, 0));
  }

  // +1000 ms
  moveTime(10000);

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  TSD_DeviceCalCfgRequest request = {};
  request.ChannelNumber = -1;
  request.Command = SUPLA_CALCFG_CMD_ENTER_CFG_MODE;
  request.SuperUserAuthorized = 0;

  supla_esp_calcfg_request(&request);

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

}

TEST_F(CalCfgFixture, EnterCfgModeAndTimeoutExit) {
  gpioConfigId = 0;

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  {
    InSequence seq;

    EXPECT_CALL(
        srpc, valueChanged(_, 0, ElementsAreArray({255, 0, 0, 0, 0, 0, 0, 0})));

    EXPECT_CALL(srpc, srpc_ds_async_device_calcfg_result(
          _, 0, -1, SUPLA_CALCFG_CMD_ENTER_CFG_MODE,
          SUPLA_CALCFG_RESULT_DONE, 0));
  }

  moveTime(10000);

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  TSD_DeviceCalCfgRequest request = {};
  request.ChannelNumber = -1;
  request.Command = SUPLA_CALCFG_CMD_ENTER_CFG_MODE;
  request.SuperUserAuthorized = 1;

  supla_esp_calcfg_request(&request);

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  moveTime(4*60*1000 + 1000);

  EXPECT_CALL(board, supla_system_restart()).Times(1);
  moveTime(1*60*1000);
}

TEST_F(CalCfgFixture, EnterCfgModeAndClientConnected) {
  gpioConfigId = 0;

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  {
    InSequence seq;

    EXPECT_CALL(
        srpc, valueChanged(_, 0, ElementsAreArray({255, 0, 0, 0, 0, 0, 0, 0})));

    EXPECT_CALL(srpc, srpc_ds_async_device_calcfg_result(
          _, 0, -1, SUPLA_CALCFG_CMD_ENTER_CFG_MODE,
          SUPLA_CALCFG_RESULT_DONE, 0));
  }

  moveTime(10000);

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  TSD_DeviceCalCfgRequest request = {};
  request.ChannelNumber = -1;
  request.Command = SUPLA_CALCFG_CMD_ENTER_CFG_MODE;
  request.SuperUserAuthorized = 1;

  supla_esp_calcfg_request(&request);

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  moveTime(4*60*1000 + 1000);

  EXPECT_CALL(board, supla_system_restart()).Times(0);

  // simluate connection
  struct espconn conn = {};
  last_espconn_connectcb(&conn);
  free(conn.reverse);

  moveTime(1*60*1000);
}

