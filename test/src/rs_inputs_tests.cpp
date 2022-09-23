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
void gpioCallbackRsInputs() {
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
  } else if (gpioConfigId == 1) {
    supla_input_cfg[0].type = INPUT_TYPE_BTN_BISTABLE;
    supla_input_cfg[1].type = INPUT_TYPE_BTN_BISTABLE;
    supla_input_cfg[0].action_trigger_cap =
      SUPLA_ACTION_CAP_TOGGLE_x2 | SUPLA_ACTION_CAP_TOGGLE_x3;
    supla_input_cfg[1].action_trigger_cap =
      SUPLA_ACTION_CAP_TOGGLE_x2 | SUPLA_ACTION_CAP_TOGGLE_x3;
  } else if (gpioConfigId == 2) {
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[1].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[2].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[3].type = INPUT_TYPE_BTN_MONOSTABLE;

    supla_input_cfg[0].channel = 2;
    supla_input_cfg[1].channel = 3;
    supla_input_cfg[2].channel = 4;
    supla_input_cfg[3].channel = 5;

    supla_relay_cfg[0].channel = 0;
    supla_relay_cfg[1].channel = 0;
    supla_relay_cfg[2].channel = 1;
    supla_relay_cfg[3].channel = 1;

    supla_input_cfg[0].relay_gpio_id = 5;
    supla_input_cfg[1].relay_gpio_id = 6;

    supla_input_cfg[2].flags = 0;
    supla_input_cfg[2].gpio_id = 3;
    supla_input_cfg[2].channel = 1;
    supla_input_cfg[2].relay_gpio_id = 7;

    supla_input_cfg[3].gpio_id = 4;
    supla_input_cfg[3].flags = 0;  // INPUT_FLAG_CFG_BTN;
    supla_input_cfg[3].channel = 2;
    supla_input_cfg[3].relay_gpio_id = 8;

    supla_relay_cfg[0].gpio_id = 5;
    supla_relay_cfg[1].gpio_id = 6;
    supla_relay_cfg[2].gpio_id = 7;
    supla_relay_cfg[3].gpio_id = 8;

    // up
    supla_relay_cfg[2].flags = 0;
    supla_relay_cfg[2].channel_flags = 0;

    // down
    supla_relay_cfg[3].flags = 0;
    supla_relay_cfg[3].channel_flags = 0;

    supla_rs_cfg[1].up = &supla_relay_cfg[2];
    supla_rs_cfg[1].down = &supla_relay_cfg[3];
    *supla_rs_cfg[1].position = 0;
    *supla_rs_cfg[1].full_closing_time = 0;
    *supla_rs_cfg[1].full_opening_time = 0;
    supla_rs_cfg[1].delayed_trigger.value = 0;
  } else if (gpioConfigId == 3) {
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[1].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[0].action_trigger_cap = SUPLA_ACTION_CAP_HOLD |
      SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x3;
    supla_input_cfg[1].action_trigger_cap = SUPLA_ACTION_CAP_HOLD |
      SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x3;
    supla_input_cfg[0].flags |= INPUT_FLAG_TRIGGER_ON_PRESS;
    supla_input_cfg[1].flags |= INPUT_FLAG_TRIGGER_ON_PRESS;
  } else {
    assert(false);
  }
}

TSD_SuplaRegisterDeviceResult regRsInputs;

char custom_srpc_getdata_rs_inputs(void *_srpc, TsrpcReceivedData *rd,
    unsigned _supla_int_t rr_id) {
  rd->call_id = SUPLA_SD_CALL_REGISTER_DEVICE_RESULT;
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

      gpioInitCb = gpioCallbackRsInputs;
      supla_esp_gpio_init_time = 0;
      EXPECT_CALL(srpc, srpc_params_init(_));
      EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return((void *)1));
      EXPECT_CALL(srpc, srpc_set_proto_version(_, 17));

      regRsInputs.result_code = SUPLA_RESULTCODE_TRUE;

      EXPECT_CALL(srpc, srpc_getdata(_, _, _))
        .WillOnce(DoAll(Invoke(custom_srpc_getdata_rs_inputs), Return(1)));
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
    for (int i = 0; i < timeMs / 10; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }
};

TEST_F(RsInputsFixture, MonostableRsCfgButton) {
  gpioConfigId = 0;

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  {
    InSequence seq;

    EXPECT_CALL(
        srpc, valueChanged(_, 0, ElementsAreArray({255, 0, 0, 0, 0, 0, 0, 0})));
  }

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(BUTTON_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(2, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 150; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 2
  eagleStub.gpioOutputSet(2, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);
  // enter cfg mode
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +6 s
  for (int i = 0; i < 600; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // Button click >3s after enter cfg mode should trigger cfgmode exit
  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // only 900 ms passed, so we should still be in cfgmode
  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // +2500 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  EXPECT_CALL(board, supla_system_restart()).Times(1);
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);
}

TEST_F(RsInputsFixture, BistableButton) {
  gpioConfigId = 1;

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  {
    InSequence seq;

    EXPECT_CALL(
        srpc, valueChanged(_, 0, ElementsAreArray({255, 0, 0, 0, 0, 0, 0, 0})));
  }

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(BUTTON_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 150; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(2, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 150; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 2
  eagleStub.gpioOutputSet(2, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);
  // enter cfg mode

  for (int click = 0; click < 5; click++) {
    // simulate button press on gpio 1
    eagleStub.gpioOutputSet(BUTTON_UP, 1);
    ets_gpio_intr_func(NULL);
    for (int i = 0; i < 50; i++) {
      curTime += 10000;  // +10ms
      executeTimers();
    }
    eagleStub.gpioOutputSet(BUTTON_UP, 0);
    ets_gpio_intr_func(NULL);
    for (int i = 0; i < 50; i++) {
      curTime += 10000;  // +10ms
      executeTimers();
    }
  }

  for (int i = 0; i < 70; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // Button click >3s after enter cfg mode should trigger cfgmode exit
  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // +2500 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_CALL(board, supla_system_restart()).Times(1);

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
}

TEST_F(RsInputsFixture, MonostableRsCfgButtonWithAT) {
  gpioConfigId = 0;

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  {
    InSequence seq;

    EXPECT_CALL(
        srpc, valueChanged(_, 0, ElementsAreArray({255, 0, 0, 0, 0, 0, 0, 0})));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(2, SUPLA_ACTION_CAP_HOLD));
  }

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(BUTTON_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1;  // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TSD_ChannelConfig_ActionTrigger);

  TSD_ChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x2;
  memcpy(configResult.Config, &atSettings,
      sizeof(TSD_ChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_SHORT_PRESS_x2);

  configResult.ChannelNumber = 2;
  atSettings.ActiveActions = SUPLA_ACTION_CAP_HOLD;
  memcpy(configResult.Config, &atSettings,
      sizeof(TSD_ChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[1].active_triggers, SUPLA_ACTION_CAP_HOLD);

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 20; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(2, 1);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 10; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 2
  eagleStub.gpioOutputSet(2, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // check if HOLD will be send
  eagleStub.gpioOutputSet(2, 1);
  ets_gpio_intr_func(NULL);
  for (int i = 0; i < 120; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  eagleStub.gpioOutputSet(2, 0);
  ets_gpio_intr_func(NULL);
  for (int i = 0; i < 80; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);
  // enter cfg mode
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +6 s
  for (int i = 0; i < 600; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // Button click >3s after enter cfg mode should trigger cfgmode exit
  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // only 900 ms passed, so we should still be in cfgmode
  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // +2500 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  EXPECT_CALL(board, supla_system_restart()).Times(1);
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);
}

TEST_F(RsInputsFixture, BistableButtonWithAT) {
  gpioConfigId = 1;

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  {
    InSequence seq;

    EXPECT_CALL(
        srpc, valueChanged(_, 0, ElementsAreArray({255, 0, 0, 0, 0, 0, 0, 0})));

    EXPECT_CALL(srpc,
        srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TOGGLE_x2));
  }

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(BUTTON_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1;  // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TSD_ChannelConfig_ActionTrigger);

  TSD_ChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_TOGGLE_x2;
  memcpy(configResult.Config, &atSettings,
      sizeof(TSD_ChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers, SUPLA_ACTION_CAP_TOGGLE_x2);

  configResult.ChannelNumber = 2;
  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[1].active_triggers, SUPLA_ACTION_CAP_TOGGLE_x2);

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 150; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 2
  eagleStub.gpioOutputSet(BUTTON_DOWN, 1);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 150; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 2
  eagleStub.gpioOutputSet(BUTTON_DOWN, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(2, 1);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(2, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // check if TOGGLE_2x will be send
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);
  for (int i = 0; i < 20; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);
  for (int i = 0; i < 80; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);
  // enter cfg mode

  for (int click = 0; click < 5; click++) {
    // simulate button press on gpio 1
    eagleStub.gpioOutputSet(BUTTON_UP, 1);
    ets_gpio_intr_func(NULL);
    for (int i = 0; i < 30; i++) {
      curTime += 10000;  // +10ms
      executeTimers();
    }
    eagleStub.gpioOutputSet(BUTTON_UP, 0);
    ets_gpio_intr_func(NULL);
    for (int i = 0; i < 30; i++) {
      curTime += 10000;  // +10ms
      executeTimers();
    }
  }

  for (int i = 0; i < 70; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // Button click >3s after enter cfg mode should trigger cfgmode exit
  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // +2500 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_CALL(board, supla_system_restart()).Times(1);

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
}

TEST_F(RsInputsFixture, MonostableRsWithATOnSingleButton) {
  gpioConfigId = 0;

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  {
    InSequence seq;

    EXPECT_CALL(
        srpc, valueChanged(_, 0, ElementsAreArray({255, 0, 0, 0, 0, 0, 0, 0})));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(2, SUPLA_ACTION_CAP_HOLD));
  }

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);
  EXPECT_EQ(supla_input_cfg[1].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1;  // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TSD_ChannelConfig_ActionTrigger);

  TSD_ChannelConfig_ActionTrigger atSettings = {};

  configResult.ChannelNumber = 2;
  atSettings.ActiveActions = SUPLA_ACTION_CAP_HOLD;
  memcpy(configResult.Config, &atSettings,
      sizeof(TSD_ChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[1].active_triggers, SUPLA_ACTION_CAP_HOLD);
  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 20; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(2, 1);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 10; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 2
  eagleStub.gpioOutputSet(2, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // check if HOLD will be send
  eagleStub.gpioOutputSet(2, 1);
  ets_gpio_intr_func(NULL);
  for (int i = 0; i < 120; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  eagleStub.gpioOutputSet(2, 0);
  ets_gpio_intr_func(NULL);
  for (int i = 0; i < 80; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);
  // enter cfg mode
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +6 s
  for (int i = 0; i < 600; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // Button click >3s after enter cfg mode should trigger cfgmode exit
  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // only 900 ms passed, so we should still be in cfgmode
  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // +2500 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  EXPECT_CALL(board, supla_system_restart()).Times(1);
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);
}

TEST_F(RsInputsFixture, BistableButtonWithATOnSingleInput) {
  gpioConfigId = 1;

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  {
    InSequence seq;

    EXPECT_CALL(
        srpc, valueChanged(_, 0, ElementsAreArray({255, 0, 0, 0, 0, 0, 0, 0})));

    EXPECT_CALL(srpc,
        srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TOGGLE_x2));
  }

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);
  EXPECT_EQ(supla_input_cfg[1].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1;  // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TSD_ChannelConfig_ActionTrigger);

  TSD_ChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_TOGGLE_x2;
  memcpy(configResult.Config, &atSettings,
      sizeof(TSD_ChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers, SUPLA_ACTION_CAP_TOGGLE_x2);
  EXPECT_EQ(supla_input_cfg[1].active_triggers, 0);

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 150; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_DOWN, 1);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 150; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 2
  eagleStub.gpioOutputSet(2, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(2, 1);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(2, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // check if TOGGLE_2x will be send
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);
  for (int i = 0; i < 20; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);
  for (int i = 0; i < 80; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);
  // enter cfg mode

  for (int click = 0; click < 5; click++) {
    // simulate button press on gpio 1
    eagleStub.gpioOutputSet(BUTTON_UP, 1);
    ets_gpio_intr_func(NULL);
    for (int i = 0; i < 30; i++) {
      curTime += 10000;  // +10ms
      executeTimers();
    }
    eagleStub.gpioOutputSet(BUTTON_UP, 0);
    ets_gpio_intr_func(NULL);
    for (int i = 0; i < 30; i++) {
      curTime += 10000;  // +10ms
      executeTimers();
    }
  }

  for (int i = 0; i < 70; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // Button click >3s after enter cfg mode should trigger cfgmode exit
  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // +2500 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_CALL(board, supla_system_restart()).Times(1);

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
}

TEST_F(RsInputsFixture, TwoRs) {
  gpioConfigId = 2;

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  EXPECT_CALL(srpc,
      valueChanged(_, 0, ElementsAreArray({255, 0, 0, 0, 0, 0, 0, 0})));

  EXPECT_CALL(srpc,
      valueChanged(_, 1, ElementsAreArray({255, 0, 0, 0, 0, 0, 0, 0})));

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_FALSE(eagleStub.getGpioValue(5));
  EXPECT_FALSE(eagleStub.getGpioValue(6));
  EXPECT_FALSE(eagleStub.getGpioValue(7));
  EXPECT_FALSE(eagleStub.getGpioValue(8));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(5));
  EXPECT_FALSE(eagleStub.getGpioValue(6));
  EXPECT_FALSE(eagleStub.getGpioValue(7));
  EXPECT_FALSE(eagleStub.getGpioValue(8));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(5));
  EXPECT_FALSE(eagleStub.getGpioValue(6));
  EXPECT_FALSE(eagleStub.getGpioValue(7));
  EXPECT_FALSE(eagleStub.getGpioValue(8));

  // simulate button press on gpio 4
  eagleStub.gpioOutputSet(4, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 150; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(4, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(5));
  EXPECT_FALSE(eagleStub.getGpioValue(6));
  EXPECT_FALSE(eagleStub.getGpioValue(7));
  EXPECT_TRUE(eagleStub.getGpioValue(8));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(5));
  EXPECT_FALSE(eagleStub.getGpioValue(6));
  EXPECT_FALSE(eagleStub.getGpioValue(7));
  EXPECT_TRUE(eagleStub.getGpioValue(8));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(5));
  EXPECT_FALSE(eagleStub.getGpioValue(6));
  EXPECT_FALSE(eagleStub.getGpioValue(7));
  EXPECT_TRUE(eagleStub.getGpioValue(8));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(5));
  EXPECT_FALSE(eagleStub.getGpioValue(6));
  EXPECT_FALSE(eagleStub.getGpioValue(7));
  EXPECT_TRUE(eagleStub.getGpioValue(8));

  // simulate button press on gpio 3
  eagleStub.gpioOutputSet(3, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 150; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  // simulate button release on gpio 3
  eagleStub.gpioOutputSet(3, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 150; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(5));
  EXPECT_FALSE(eagleStub.getGpioValue(6));
  EXPECT_FALSE(eagleStub.getGpioValue(7));
  EXPECT_FALSE(eagleStub.getGpioValue(8));

  // simulate button press on gpio 3
  eagleStub.gpioOutputSet(3, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 150; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  // simulate button release on gpio 3
  eagleStub.gpioOutputSet(3, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 150; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(5));
  EXPECT_FALSE(eagleStub.getGpioValue(6));
  EXPECT_TRUE(eagleStub.getGpioValue(7));
  EXPECT_FALSE(eagleStub.getGpioValue(8));
}

TEST_F(RsInputsFixture, BistableButtonWithNoDelayBetweenInputs) {
  gpioConfigId = 1;

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  {
    InSequence seq;

    EXPECT_CALL(
        srpc, valueChanged(_, 0, ElementsAreArray({255, 0, 0, 0, 0, 0, 0, 0})));
  }

  moveTime(1000);

  EXPECT_FALSE(eagleStub.getGpioValue(BUTTON_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(BUTTON_DOWN));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  moveTime(300);

  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  moveTime(1500);

  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));


  // release up and shortly after that press down
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);
  moveTime(10);
  eagleStub.gpioOutputSet(BUTTON_DOWN, 1);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_DOWN));

  // press up and release down and at the same time
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);
  eagleStub.gpioOutputSet(BUTTON_DOWN, 0);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  // press down and after short time release up
  eagleStub.gpioOutputSet(BUTTON_DOWN, 1);
  ets_gpio_intr_func(NULL);
  moveTime(10);
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_DOWN));

  // press up while down is also pressed
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  // release up, down is still pressed but there should be no down movement
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  // release down
  eagleStub.gpioOutputSet(BUTTON_DOWN, 0);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  // press up
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  // press down, while up is still pressed
  eagleStub.gpioOutputSet(BUTTON_DOWN, 1);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_DOWN));

  // release down
  eagleStub.gpioOutputSet(BUTTON_DOWN, 0);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));
}

TEST_F(RsInputsFixture, BistableButtonWithATNoDelayBetweenInputs) {
  gpioConfigId = 1;

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  {
    InSequence seq;

    EXPECT_CALL(
        srpc, valueChanged(_, 0, ElementsAreArray({255, 0, 0, 0, 0, 0, 0, 0})));
  }

  moveTime(1000);

  EXPECT_FALSE(eagleStub.getGpioValue(BUTTON_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(BUTTON_DOWN));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1;  // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TSD_ChannelConfig_ActionTrigger);

  TSD_ChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_TOGGLE_x2;
  memcpy(configResult.Config, &atSettings,
      sizeof(TSD_ChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers, SUPLA_ACTION_CAP_TOGGLE_x2);

  configResult.ChannelNumber = 2;
  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[1].active_triggers, SUPLA_ACTION_CAP_TOGGLE_x2);

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  moveTime(500);

  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  moveTime(1500);

  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));


  // release up and shortly after that press down
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);
  moveTime(10);
  eagleStub.gpioOutputSet(BUTTON_DOWN, 1);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_DOWN));

  // press up and release down and at the same time
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);
  eagleStub.gpioOutputSet(BUTTON_DOWN, 0);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  // press down and after short time release up
  eagleStub.gpioOutputSet(BUTTON_DOWN, 1);
  ets_gpio_intr_func(NULL);
  moveTime(10);
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_DOWN));

  // press up while down is also pressed
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  // release up, down is still pressed but there should be no down movement
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  // release down
  eagleStub.gpioOutputSet(BUTTON_DOWN, 0);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  // press up
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  // press down, while up is still pressed
  eagleStub.gpioOutputSet(BUTTON_DOWN, 1);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_DOWN));

  // release down
  eagleStub.gpioOutputSet(BUTTON_DOWN, 0);
  ets_gpio_intr_func(NULL);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));
}

TEST_F(RsInputsFixture, BistableButtonWithLessThan1sToggles) {
  gpioConfigId = 1;

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  {
    InSequence seq;
    EXPECT_CALL(
        srpc, valueChanged(_, 0, ElementsAreArray({255, 0, 0, 0, 0, 0, 0, 0})));
  }

  moveTime(1000);
  EXPECT_FALSE(eagleStub.getGpioValue(BUTTON_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(BUTTON_DOWN));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  moveTime(300);

  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  // release up and shortly after that press down
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);
  moveTime(10);
  eagleStub.gpioOutputSet(BUTTON_DOWN, 1);
  ets_gpio_intr_func(NULL);

  moveTime(200);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  eagleStub.gpioOutputSet(BUTTON_DOWN, 0);
  ets_gpio_intr_func(NULL);

  moveTime(2000);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));
}

TEST_F(RsInputsFixture, BistableButtonWithLessThan1sTogglesWithNotify) {
  gpioConfigId = 1;

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  {
    InSequence seq;
    EXPECT_CALL(
        srpc, valueChanged(_, 0, ElementsAreArray({255, 0, 0, 0, 0, 0, 0, 0})));
  }

  moveTime(1000);
  EXPECT_FALSE(eagleStub.getGpioValue(BUTTON_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(BUTTON_DOWN));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // simulate button press on gpio 1
  supla_esp_input_notify_state_change(&supla_input_cfg[0], INPUT_STATE_ACTIVE);

  moveTime(30);

  supla_esp_input_notify_state_change(&supla_input_cfg[0], INPUT_STATE_INACTIVE);
  supla_esp_input_notify_state_change(&supla_input_cfg[1], INPUT_STATE_ACTIVE);

  moveTime(1500);

  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_DOWN));

  supla_esp_input_notify_state_change(&supla_input_cfg[1], INPUT_STATE_INACTIVE);
  moveTime(200);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  moveTime(1500);

  supla_esp_input_notify_state_change(&supla_input_cfg[0], INPUT_STATE_ACTIVE);
  supla_esp_input_notify_state_change(&supla_input_cfg[1], INPUT_STATE_ACTIVE);
  supla_esp_input_notify_state_change(&supla_input_cfg[0], INPUT_STATE_INACTIVE);

  moveTime(1500);

  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_DOWN));

  supla_esp_input_notify_state_change(&supla_input_cfg[1], INPUT_STATE_INACTIVE);
  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  supla_esp_input_notify_state_change(&supla_input_cfg[0], INPUT_STATE_ACTIVE);
  supla_esp_input_notify_state_change(&supla_input_cfg[1], INPUT_STATE_ACTIVE);
  supla_esp_input_notify_state_change(&supla_input_cfg[1], INPUT_STATE_INACTIVE);

  moveTime(1500);

  // Depending on HW in this scenario it may be expected to have RS moving
  // UP, however we may also accept case when RS is not in move
  //EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));


}

TEST_F(RsInputsFixture, BistableButtonWithNoDelayBetweenInputsBasedOnNotify) {
  gpioConfigId = 1;

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  {
    InSequence seq;

    EXPECT_CALL(
        srpc, valueChanged(_, 0, ElementsAreArray({255, 0, 0, 0, 0, 0, 0, 0})));
  }

  moveTime(1000);

  EXPECT_FALSE(eagleStub.getGpioValue(BUTTON_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(BUTTON_DOWN));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // simulate button press on gpio 1
  supla_esp_input_notify_state_change(&supla_input_cfg[0], INPUT_STATE_ACTIVE);

  moveTime(300);

  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  moveTime(1500);

  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));


  // release up and shortly after that press down
  supla_esp_input_notify_state_change(&supla_input_cfg[0], INPUT_STATE_INACTIVE);
  moveTime(10);
  supla_esp_input_notify_state_change(&supla_input_cfg[1], INPUT_STATE_ACTIVE);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_DOWN));

  // press up and release down and at the same time
  supla_esp_input_notify_state_change(&supla_input_cfg[0], INPUT_STATE_ACTIVE);
  supla_esp_input_notify_state_change(&supla_input_cfg[1], INPUT_STATE_INACTIVE);

  moveTime(1500);
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  // press down and after short time release up
  supla_esp_input_notify_state_change(&supla_input_cfg[1], INPUT_STATE_ACTIVE);
  moveTime(10);
  supla_esp_input_notify_state_change(&supla_input_cfg[0], INPUT_STATE_INACTIVE);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_DOWN));

  // press up while down is also pressed
  supla_esp_input_notify_state_change(&supla_input_cfg[0], INPUT_STATE_ACTIVE);

  moveTime(1500);
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  // release up, down is still pressed but there should be no down movement
  supla_esp_input_notify_state_change(&supla_input_cfg[0], INPUT_STATE_INACTIVE);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  // release down
  supla_esp_input_notify_state_change(&supla_input_cfg[1], INPUT_STATE_INACTIVE);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  // press up
  supla_esp_input_notify_state_change(&supla_input_cfg[0], INPUT_STATE_ACTIVE);

  moveTime(1500);
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));

  // press down, while up is still pressed
  supla_esp_input_notify_state_change(&supla_input_cfg[1], INPUT_STATE_ACTIVE);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_TRUE(eagleStub.getGpioValue(RELAY_DOWN));

  // release down
  supla_esp_input_notify_state_change(&supla_input_cfg[1], INPUT_STATE_INACTIVE);

  moveTime(1500);
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(RELAY_DOWN));
}

TEST_F(RsInputsFixture, MonostableRsCfgButtonWithATAndTriggerOnPress) {
  gpioConfigId = 3;

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  {
    InSequence seq;

    EXPECT_CALL(
        srpc, valueChanged(_, 0, ElementsAreArray({255, 0, 0, 0, 0, 0, 0, 0})));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(2, SUPLA_ACTION_CAP_HOLD));
  }

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(BUTTON_UP));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1;  // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TSD_ChannelConfig_ActionTrigger);

  TSD_ChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x2;
  memcpy(configResult.Config, &atSettings,
      sizeof(TSD_ChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_SHORT_PRESS_x2);

  configResult.ChannelNumber = 2;
  atSettings.ActiveActions = SUPLA_ACTION_CAP_HOLD;
  memcpy(configResult.Config, &atSettings,
      sizeof(TSD_ChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[1].active_triggers, SUPLA_ACTION_CAP_HOLD);

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 20; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(2, 1);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 10; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 2
  eagleStub.gpioOutputSet(2, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // check if HOLD will be send
  eagleStub.gpioOutputSet(2, 1);
  ets_gpio_intr_func(NULL);
  for (int i = 0; i < 120; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  eagleStub.gpioOutputSet(2, 0);
  ets_gpio_intr_func(NULL);
  for (int i = 0; i < 80; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_TRUE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);
  // enter cfg mode
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +6 s
  for (int i = 0; i < 600; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // Button click >3s after enter cfg mode should trigger cfgmode exit
  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  // only 900 ms passed, so we should still be in cfgmode
  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // +2500 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(BUTTON_UP, 0);
  ets_gpio_intr_func(NULL);

  EXPECT_CALL(board, supla_system_restart()).Times(1);
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000;  // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);
}
