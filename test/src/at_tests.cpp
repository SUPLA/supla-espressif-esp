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
void gpioCallbackAt() {
  supla_input_cfg[0].gpio_id = 1;
  supla_input_cfg[0].flags = 0;
  supla_input_cfg[0].channel = 1; // AT channel
  supla_input_cfg[0].relay_gpio_id = 2;

  supla_relay_cfg[0].gpio_id = 2;
  supla_relay_cfg[0].channel = 0;
  supla_relay_cfg[0].flags = RELAY_FLAG_RESTORE_FORCE;
  supla_relay_cfg[0].channel_flags = 
    SUPLA_CHANNEL_FLAG_COUNTDOWN_TIMER_SUPPORTED;
  if (gpioConfigId == 0) {
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[0].action_trigger_cap = SUPLA_ACTION_CAP_SHORT_PRESS_x2;
    supla_input_cfg[0].flags = INPUT_FLAG_TRIGGER_ON_PRESS;
  } else if (gpioConfigId == 1) {
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[0].action_trigger_cap = SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x3 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x4 | 
      SUPLA_ACTION_CAP_SHORT_PRESS_x5;
    supla_input_cfg[0].flags = INPUT_FLAG_TRIGGER_ON_PRESS;
  } else if (gpioConfigId == 2) {
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[0].action_trigger_cap = SUPLA_ACTION_CAP_HOLD |
      SUPLA_ACTION_CAP_SHORT_PRESS_x3 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x4 | 
      SUPLA_ACTION_CAP_SHORT_PRESS_x5;
    supla_input_cfg[0].flags = INPUT_FLAG_TRIGGER_ON_PRESS;
  } else if (gpioConfigId == 3) {
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[0].action_trigger_cap = SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x3 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x4 | 
      SUPLA_ACTION_CAP_SHORT_PRESS_x5;
    supla_input_cfg[0].flags = INPUT_FLAG_TRIGGER_ON_PRESS | 
      INPUT_FLAG_CFG_BTN |
      INPUT_FLAG_CFG_ON_TOGGLE |
      INPUT_FLAG_CFG_ON_HOLD;
  } else if (gpioConfigId == 4) {
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[0].action_trigger_cap = SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x3 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x4 | 
      SUPLA_ACTION_CAP_SHORT_PRESS_x5;
    supla_input_cfg[0].flags = INPUT_FLAG_TRIGGER_ON_PRESS | 
      INPUT_FLAG_CFG_BTN;
  } else if (gpioConfigId == 5) {
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[0].action_trigger_cap = SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x3 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x4 | 
      SUPLA_ACTION_CAP_SHORT_PRESS_x5 | 
      SUPLA_ACTION_CAP_HOLD;
    supla_input_cfg[0].flags = INPUT_FLAG_TRIGGER_ON_PRESS | 
      INPUT_FLAG_CFG_BTN;
  } else if (gpioConfigId == 6) {
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[0].action_trigger_cap = SUPLA_ACTION_CAP_SHORT_PRESS_x1 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x3 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x4 | 
      SUPLA_ACTION_CAP_SHORT_PRESS_x5 | 
      SUPLA_ACTION_CAP_HOLD;
    supla_input_cfg[0].flags = 0;
    supla_input_cfg[0].relay_gpio_id = 255; // input not related to any relay
  } else if (gpioConfigId == 7) {
    supla_input_cfg[0].flags = 0;
    supla_input_cfg[0].type = INPUT_TYPE_BTN_BISTABLE;
    supla_input_cfg[0].action_trigger_cap = 
      SUPLA_ACTION_CAP_TOGGLE_x2 |
      SUPLA_ACTION_CAP_TOGGLE_x3 |
      SUPLA_ACTION_CAP_TOGGLE_x5;
  } else if (gpioConfigId == 8) {
    supla_input_cfg[0].flags = INPUT_FLAG_CFG_BTN;
    supla_input_cfg[0].type = INPUT_TYPE_BTN_BISTABLE;
    supla_input_cfg[0].action_trigger_cap = 
      SUPLA_ACTION_CAP_TOGGLE_x2 |
      SUPLA_ACTION_CAP_TOGGLE_x3 |
      SUPLA_ACTION_CAP_TOGGLE_x5;
  } else if (gpioConfigId == 9) {
    supla_input_cfg[0].flags = 0;
    supla_input_cfg[0].type = INPUT_TYPE_BTN_BISTABLE;
    supla_input_cfg[0].action_trigger_cap = 
      SUPLA_ACTION_CAP_TOGGLE_x1 |
      SUPLA_ACTION_CAP_TOGGLE_x2 |
      SUPLA_ACTION_CAP_TURN_ON |
      SUPLA_ACTION_CAP_TURN_OFF;
    supla_input_cfg[0].relay_gpio_id = 255;
  } else if (gpioConfigId == 10) {
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[0].action_trigger_cap = SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x3 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x4 | 
      SUPLA_ACTION_CAP_SHORT_PRESS_x5;
    supla_input_cfg[0].flags = INPUT_FLAG_TRIGGER_ON_PRESS | 
      INPUT_FLAG_CFG_BTN |
      INPUT_FLAG_CFG_ON_TOGGLE |
      INPUT_FLAG_CFG_ON_HOLD;
  } else {
    assert(false);
  }
}


class ATFixture : public ::testing::Test {
public:
  TimeMock time;
  EagleSocStub eagleStub;
  BoardMock board;
  int curTime;

  void SetUp() override {
    cleanupTimers();
    curTime = 10000; 
    currentDeviceState = STATE_UNKNOWN;
    EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));
    memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
    memset(&supla_esp_state, 0, sizeof(SuplaEspState));
    memset(&supla_relay_cfg, 0, sizeof(supla_relay_cfg));
    memset(&supla_rs_cfg, 0, sizeof(supla_rs_cfg));
    gpioInitCb = gpioCallbackAt;
    supla_esp_gpio_init_time = 0;

    strncpy(supla_esp_cfg.Server, "test", 4);
    strncpy(supla_esp_cfg.Email, "test", 4);
    strncpy(supla_esp_cfg.WIFI_SSID, "test", 4);
    strncpy(supla_esp_cfg.WIFI_PWD, "test", 4);
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
  }

  void moveTime(const int timeMs) {
    for (int i = 0; i < timeMs / 10; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }
};

TSD_SuplaRegisterDeviceResult regAt;

char custom_srpc_getdata_at(void *_srpc, TsrpcReceivedData *rd,
    unsigned _supla_int_t rr_id) {
  rd->call_id = SUPLA_SD_CALL_REGISTER_DEVICE_RESULT;
  rd->data.sd_register_device_result = &regAt;
  return 1;
}

class ATRegisteredFixture: public ATFixture {
public:
  SrpcMock srpc;

  void SetUp() override {
    ATFixture::SetUp();
    EXPECT_CALL(srpc, srpc_params_init(_));
    EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return((void *)1));
    EXPECT_CALL(srpc, srpc_set_proto_version(_, SUPLA_PROTO_VERSION));

    regAt.result_code = SUPLA_RESULTCODE_TRUE;

    EXPECT_CALL(srpc, srpc_getdata(_, _, _))
      .WillOnce(DoAll(Invoke(custom_srpc_getdata_at), Return(1)));
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
    ATFixture::TearDown();
    supla_esp_devconn_release();
  }
};

TEST_F(ATRegisteredFixture, MonostablePress_x2) {
  gpioConfigId = 0;

  {
    InSequence seq;
    // Expected channel 0 (relay) state changes ON
    EXPECT_CALL(srpc,
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})));

    EXPECT_CALL(
        srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x2));

    // RELAY OFF
    EXPECT_CALL(srpc,
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})));
  }
  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  moveTime(1000);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  moveTime(300);

  // AT is not enabled yet, so monostable button should change relay on press
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate inactive sensor on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);

  moveTime(700);

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x2;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers, SUPLA_ACTION_CAP_SHORT_PRESS_x2);

  for (int i = 0; i < 2; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    moveTime(200);

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_TRUE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    moveTime(200);
  }

  moveTime(500);

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  for (int i = 0; i < 1; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    moveTime(200);

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_TRUE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    moveTime(200);
  }

  moveTime(500);

  // AT is enabled, so monostable button change relay on press_1x
  EXPECT_FALSE(eagleStub.getGpioValue(2));

}

TEST_F(ATRegisteredFixture, MonostablePressMultipleWithout3x) {
  gpioConfigId = 1;

  {
    InSequence seq;

    // Expected channel 1 (ActionTrigger) state changes
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x2));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x5));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x4));

    // Expected channel 0 (relay) state changes ON
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})));

  }

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x5;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_SHORT_PRESS_x2 | SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x5);

  // 2x press
  for (int i = 0; i < 2; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));


  // 3x press - should be ignored since it is not active
  for (int i = 0; i < 3; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));


  // 5x press
  for (int i = 0; i < 5; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));


  // 4x press
  for (int i = 0; i < 4; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));


  // 1x press
  for (int i = 0; i < 1; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button change relay on press_1x
  EXPECT_TRUE(eagleStub.getGpioValue(2));

}

TEST_F(ATRegisteredFixture, MonostableMultipleAndHold) {
  gpioConfigId = 2;

  {
    InSequence seq;
    // Expected channel 1 (ActionTrigger) state changes
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x3));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_HOLD));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x3));

    // Expected channel 0 (relay) state changes ON
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})));

  }

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x3 |
    SUPLA_ACTION_CAP_HOLD;

  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_SHORT_PRESS_x3 | SUPLA_ACTION_CAP_HOLD);

  // 2x press - not activated
  for (int i = 0; i < 2; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));


  // 3x press
  for (int i = 0; i < 3; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // HOLD
  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate inactive input on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }


  // 4x press - 3x is send (as it is currently max configured)
  for (int i = 0; i < 5; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // 1x press
  for (int i = 0; i < 1; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button change relay on press_1x
  EXPECT_TRUE(eagleStub.getGpioValue(2));

}

TEST_F(ATRegisteredFixture, MonostableMoreThan5x) {
  gpioConfigId = 1;

  {
    InSequence seq;
    // Expected channel 1 (ActionTrigger) state changes
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x5));
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x5));

    // Expected channel 0 (relay) state changes ON
    EXPECT_CALL(srpc,
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})));

  }

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x5;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_SHORT_PRESS_x2 | SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x5);

  // 8x press
  for (int i = 0; i < 8; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));


  // 12x press 
  for (int i = 0; i < 12; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // 1x press
  for (int i = 0; i < 1; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button change relay on press_1x
  EXPECT_TRUE(eagleStub.getGpioValue(2));

}

TEST_F(ATRegisteredFixture, MonostableMoreThan5xAndCfgBtn) {
  gpioConfigId = 3;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x5;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_SHORT_PRESS_x2 | SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x5);

  // 8x press - nothing should happen, because we don't support it
  for (int i = 0; i < 8; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // 10x press
  for (int i = 0; i < 10; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }
  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // 1x press
  for (int i = 0; i < 1; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +5000 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, but we are in CFGMODE, so monostable button shouldn't 
  // change relay on press_1x 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  EXPECT_CALL(board, supla_system_restart()).Times(1);

  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  // +200 ms
  for (int i = 0; i < 20; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  // simulate inactive input on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);

  // +200 ms
  for (int i = 0; i < 20; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));
}

TEST_F(ATRegisteredFixture, MonostableMoreThan5xAndCfgBtnOnHold) {
  gpioConfigId = 4;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));

  {                                               
    InSequence seq;
    // Expected channel 1 (ActionTrigger) state changes
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x5));
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x5));

    // Expected channel 0 (relay) state changes ON
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})));

  }


  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x5;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_SHORT_PRESS_x2 | SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x5);

  // 8x press - nothing should happen, because we don't support it
  for (int i = 0; i < 8; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // 12x press 
  for (int i = 0; i < 12; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // 1x press
  for (int i = 0; i < 1; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +5000 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  // +5500 ms
  for (int i = 0; i < 550; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // simulate inactive input on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +5500 ms
  for (int i = 0; i < 550; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_CALL(board, supla_system_restart()).Times(1);

  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
 
  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // simulate inactive input on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
}

TEST_F(ATRegisteredFixture, MonostableCfgBtnOnHoldAndOnHoldAction) {
  gpioConfigId = 5;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));

  {                                               
    InSequence seq;
    // Expected channel 1 (ActionTrigger) state changes
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x5));
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x5));

    // Expected channel 0 (relay) state changes ON
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_HOLD));
  }


  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x5 |
    SUPLA_ACTION_CAP_HOLD;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_SHORT_PRESS_x2 | SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x5 | SUPLA_ACTION_CAP_HOLD);

  // 8x press - nothing should happen, because we don't support it
  for (int i = 0; i < 8; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // 12x press 
  for (int i = 0; i < 12; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // 1x press
  for (int i = 0; i < 1; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +5000 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  // +5500 ms
  for (int i = 0; i < 550; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // simulate inactive input on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +5500 ms
  for (int i = 0; i < 550; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_CALL(board, supla_system_restart()).Times(1);

  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
 
  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // simulate inactive input on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
}

TEST_F(ATRegisteredFixture, MonostableWithoutRelay) {
  gpioConfigId = 6;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  {                                               
    InSequence seq;
    // Expected channel 1 (ActionTrigger) state changes
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x5));
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x5));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x1));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_HOLD));
  }


  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x1 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x3 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x5 |
    SUPLA_ACTION_CAP_HOLD;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_SHORT_PRESS_x1 | SUPLA_ACTION_CAP_SHORT_PRESS_x3 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x5 | SUPLA_ACTION_CAP_HOLD);

  // 8x press - 5x should be send
  for (int i = 0; i < 8; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // 12x press - 5x should be send
  for (int i = 0; i < 12; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // 1x press
  for (int i = 0; i < 1; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +5000 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  // +5500 ms
  for (int i = 0; i < 550; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate inactive input on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

}

TEST_F(ATRegisteredFixture, BistableMulticlick) {
  gpioConfigId = 7;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  {                                               
    InSequence seq;

    // RELAY ON
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})));

    // Expected channel 1 (ActionTrigger) state changes
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TOGGLE_x5));
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TOGGLE_x5));

    // RELAY OFF
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TOGGLE_x2));

    // RELAY ON
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})));
  }


  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_TOGGLE_x2 |
    SUPLA_ACTION_CAP_TOGGLE_x5;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_TOGGLE_x2 | SUPLA_ACTION_CAP_TOGGLE_x5);

  int inputState = 1;

  // 1x toggle - should change relay state
  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, inputState);
  ets_gpio_intr_func(NULL);

  // +200 ms
  for (int i = 0; i < 20; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // 8x press - 5x should be send
  for (int i = 0; i < 8; i++) {
    inputState = !inputState;
    eagleStub.gpioOutputSet(1, inputState);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    EXPECT_TRUE(eagleStub.getGpioValue(2));
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // 12x press - 5x should be send
  for (int i = 0; i < 12; i++) {
    inputState = !inputState;
    eagleStub.gpioOutputSet(1, inputState);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    EXPECT_TRUE(eagleStub.getGpioValue(2));
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // 1x press
  inputState = !inputState;
  eagleStub.gpioOutputSet(1, inputState);
  ets_gpio_intr_func(NULL);

  // +200 ms
  for (int i = 0; i < 20; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // +5000 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // 2x press
  for (int i = 0; i < 2; i++) {
    inputState = !inputState;
    eagleStub.gpioOutputSet(1, inputState);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    EXPECT_FALSE(eagleStub.getGpioValue(2));
  }

  // +5000 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  inputState = !inputState;
  eagleStub.gpioOutputSet(1, inputState);
  ets_gpio_intr_func(NULL);

  // +1500 ms
  for (int i = 0; i < 150; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

}

TEST_F(ATRegisteredFixture, BistableMulticlickWithCfg) {
  gpioConfigId = 8;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  {                                               
    InSequence seq;

    // RELAY ON
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})));

    // RELAY OFF
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TOGGLE_x2));

  }


  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_TOGGLE_x2 |
    SUPLA_ACTION_CAP_TOGGLE_x5;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_TOGGLE_x2 | SUPLA_ACTION_CAP_TOGGLE_x5);

  int inputState = 1;

  // 1x toggle - should change relay state
  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, inputState);
  ets_gpio_intr_func(NULL);

  // +200 ms
  for (int i = 0; i < 20; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // 8x press - nothing should happen
  for (int i = 0; i < 8; i++) {
    inputState = !inputState;
    eagleStub.gpioOutputSet(1, inputState);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    EXPECT_TRUE(eagleStub.getGpioValue(2));
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // 1x press
  inputState = !inputState;
  eagleStub.gpioOutputSet(1, inputState);
  ets_gpio_intr_func(NULL);

  // +200 ms
  for (int i = 0; i < 20; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // +5000 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // 2x press
  for (int i = 0; i < 2; i++) {
    inputState = !inputState;
    eagleStub.gpioOutputSet(1, inputState);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    EXPECT_FALSE(eagleStub.getGpioValue(2));
  }

  // +5000 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // 12x press - we should enter cfg mode
  for (int i = 0; i < 12; i++) {
    inputState = !inputState;
    eagleStub.gpioOutputSet(1, inputState);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    EXPECT_FALSE(eagleStub.getGpioValue(2));
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  EXPECT_CALL(board, supla_system_restart()).Times(1);

  // +3500 ms
  for (int i = 0; i < 350; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  inputState = !inputState;
  eagleStub.gpioOutputSet(1, inputState);
  ets_gpio_intr_func(NULL);

  // +1500 ms
  for (int i = 0; i < 150; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

}

TEST_F(ATRegisteredFixture, BistableTurnOnOffAndToggle) {
  gpioConfigId = 9;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  {                                               
    InSequence seq;

    // Expected channel 1 (ActionTrigger) state changes
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TURN_ON));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TURN_OFF));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TOGGLE_x2));

  }


  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_TOGGLE_x2 |
    SUPLA_ACTION_CAP_TURN_ON | SUPLA_ACTION_CAP_TURN_OFF;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers, SUPLA_ACTION_CAP_TOGGLE_x2 |
      SUPLA_ACTION_CAP_TURN_ON |
      SUPLA_ACTION_CAP_TURN_OFF);

  int inputState = 0;

  // 2x press - toggle 2x should be send and turn on/off
  for (int i = 0; i < 2; i++) {
    inputState = !inputState;
    eagleStub.gpioOutputSet(1, inputState);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    EXPECT_FALSE(eagleStub.getGpioValue(2));
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);


}

TEST_F(ATRegisteredFixture, MonostableLimitMaxClickDetection) {
  gpioConfigId = 1;

  {                                               
    InSequence seq;
    // Expected channel 1 (ActionTrigger) state changes
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x3));
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x3));

    // Expected channel 0 (relay) state changes ON
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})));

  }

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x3;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_SHORT_PRESS_x2 | SUPLA_ACTION_CAP_SHORT_PRESS_x3);

  // 8x press
  for (int i = 0; i < 8; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));


  // 12x press 
  for (int i = 0; i < 12; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // 1x press
  for (int i = 0; i < 1; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button change relay on press_1x
  EXPECT_TRUE(eagleStub.getGpioValue(2));

}

TEST_F(ATRegisteredFixture, MonostableWithoutRelayOnly1xAndHold) {
  gpioConfigId = 6;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  {                                               
    InSequence seq;
    // Expected channel 1 (ActionTrigger) state changes
    EXPECT_CALL(srpc,
        srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x1))
      .Times(8);

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_HOLD));
  }


  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x1 |
    SUPLA_ACTION_CAP_HOLD;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_SHORT_PRESS_x1 | SUPLA_ACTION_CAP_HOLD);

  // 8x press - 1x should be send 8 times!
  for (int i = 0; i < 8; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  // +5500 ms
  for (int i = 0; i < 550; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate inactive input on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

}

TEST_F(ATRegisteredFixture, MonostableWithoutRelayOnlyHold) {
  gpioConfigId = 6;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  {                                               
    InSequence seq;
    // Expected channel 1 (ActionTrigger) state changes
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_HOLD));
  }


  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_HOLD;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers, SUPLA_ACTION_CAP_HOLD);

  // 8x press - nothing
  for (int i = 0; i < 8; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // 12x press - nothing
  for (int i = 0; i < 12; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  // +5500 ms
  for (int i = 0; i < 550; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate inactive input on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

}

TEST_F(ATRegisteredFixture, BistableTurnOnOffAndTogglex1) {
  gpioConfigId = 9;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  {                                               
    InSequence seq;

    // Expected channel 1 (ActionTrigger) state changes
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TURN_ON));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TOGGLE_x1));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TURN_OFF));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TOGGLE_x1));

  }


  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_TOGGLE_x1 |
    SUPLA_ACTION_CAP_TURN_ON | SUPLA_ACTION_CAP_TURN_OFF;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers, SUPLA_ACTION_CAP_TOGGLE_x1 |
      SUPLA_ACTION_CAP_TURN_ON |
      SUPLA_ACTION_CAP_TURN_OFF);

  int inputState = 0;

  // 2x press - toggle 2x should be send and turn on/off
  for (int i = 0; i < 2; i++) {
    inputState = !inputState;
    eagleStub.gpioOutputSet(1, inputState);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    EXPECT_FALSE(eagleStub.getGpioValue(2));
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);
}

TEST_F(ATRegisteredFixture, BistableOnlyTurnOnOff) {
  gpioConfigId = 9;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  {                                               
    InSequence seq;

    // Expected channel 1 (ActionTrigger) state changes
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TURN_ON));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TURN_OFF));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TURN_ON));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TURN_OFF));
  }


  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_TURN_ON | 
    SUPLA_ACTION_CAP_TURN_OFF;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_TURN_ON |
      SUPLA_ACTION_CAP_TURN_OFF);

  int inputState = 0;

  // 4x press - send turn on/off 2x
  for (int i = 0; i < 4; i++) {
    inputState = !inputState;
    eagleStub.gpioOutputSet(1, inputState);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    EXPECT_FALSE(eagleStub.getGpioValue(2));
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);


}

TEST_F(ATRegisteredFixture, MonostableChangeConfigDuringPress) {
  gpioConfigId = 1;

  {                                               
    InSequence seq;
    // Expected channel 0 (relay) state changes ON
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})));

    // Expected channel 1 (ActionTrigger) state changes
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x3));
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x3));

    // Expected channel 0 (relay) state changes OFF
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})));

  }

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  // Button is not yet configured from server
  
  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  // +200 ms
  for (int i = 0; i < 20; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));


  // Receive configuration from server
  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x3;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_SHORT_PRESS_x2 | SUPLA_ACTION_CAP_SHORT_PRESS_x3);

  // simulate inactive input on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);

  // +200 ms
  for (int i = 0; i < 20; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // 8x press
  for (int i = 0; i < 8; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_TRUE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_TRUE(eagleStub.getGpioValue(2));


  // 12x press 
  for (int i = 0; i < 12; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_TRUE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // 1x press
  for (int i = 0; i < 1; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_TRUE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button change relay on press_1x
  EXPECT_FALSE(eagleStub.getGpioValue(2));
}

TEST_F(ATRegisteredFixture, MonostableChangeConfigDuringPress2) {
  gpioConfigId = 1;

  {                                               
    InSequence seq;
    // Expected channel 0 (relay) state changes ON
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})));

    // Expected channel 1 (ActionTrigger) state changes
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_SHORT_PRESS_x3));


    // Expected channel 0 (relay) state changes OFF
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})));

  }

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  // Button is not yet configured from server
  
  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  // +200 ms
  for (int i = 0; i < 20; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate inactive input on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);

  // +20 ms
  for (int i = 0; i < 2; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }


  // Receive configuration from server
  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x3;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_SHORT_PRESS_x2 | SUPLA_ACTION_CAP_SHORT_PRESS_x3);

  // 8x press
  for (int i = 0; i < 8; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_TRUE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_TRUE(eagleStub.getGpioValue(2));


  // 2x press 
  for (int i = 0; i < 2; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_TRUE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  atSettings.ActiveActions = 0;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  // +200 ms
  for (int i = 0; i < 20; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate inactive input on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);

  // +200 ms
  for (int i = 0; i < 20; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // +500 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button change relay on press_1x
  EXPECT_FALSE(eagleStub.getGpioValue(2));
}

TEST_F(ATRegisteredFixture, BistableMulticlickConfigChangeDuringPress) {
  gpioConfigId = 7;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  {                                               
    InSequence seq;

    // RELAY ON
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})));

    // Expected channel 1 (ActionTrigger) state changes
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TOGGLE_x5));
    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TOGGLE_x5));

    // RELAY OFF
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})));

    EXPECT_CALL(srpc, srpc_ds_async_action_trigger(1, SUPLA_ACTION_CAP_TOGGLE_x2));

    // RELAY ON
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})));
  }


  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  int inputState = 1;

  // 1x toggle - should change relay state
  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, inputState);
  ets_gpio_intr_func(NULL);

  // +200 ms
  for (int i = 0; i < 20; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_TOGGLE_x2 |
    SUPLA_ACTION_CAP_TOGGLE_x5;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_TOGGLE_x2 | SUPLA_ACTION_CAP_TOGGLE_x5);


  // 8x press - 5x should be send
  for (int i = 0; i < 8; i++) {
    inputState = !inputState;
    eagleStub.gpioOutputSet(1, inputState);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    EXPECT_TRUE(eagleStub.getGpioValue(2));
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // 12x press - 5x should be send
  for (int i = 0; i < 12; i++) {
    inputState = !inputState;
    eagleStub.gpioOutputSet(1, inputState);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    EXPECT_TRUE(eagleStub.getGpioValue(2));
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // 1x press
  inputState = !inputState;
  eagleStub.gpioOutputSet(1, inputState);
  ets_gpio_intr_func(NULL);

  // +200 ms
  for (int i = 0; i < 20; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // +5000 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // 2x press
  for (int i = 0; i < 2; i++) {
    inputState = !inputState;
    eagleStub.gpioOutputSet(1, inputState);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    EXPECT_FALSE(eagleStub.getGpioValue(2));
  }

  // +5000 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  inputState = !inputState;
  eagleStub.gpioOutputSet(1, inputState);
  ets_gpio_intr_func(NULL);

  // +1500 ms
  for (int i = 0; i < 150; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

}

TEST_F(ATRegisteredFixture, MonostableCfgBtnOnHoldAndOnToggle) {
  gpioConfigId = 10;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(supla_input_cfg[0].active_triggers, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TChannelConfig_ActionTrigger);

  TChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x5;
  memcpy(configResult.Config, &atSettings,
      sizeof(TChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_triggers,
      SUPLA_ACTION_CAP_SHORT_PRESS_x2 | SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x5);

  // 8x press - nothing should happen, because we don't support it
  for (int i = 0; i < 8; i++) {
    // simulate active input on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

    // AT is enabled, so monostable button shouldn't change relay 
    EXPECT_FALSE(eagleStub.getGpioValue(2));

    // simulate inactive input on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +200 ms
    for (int i = 0; i < 20; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CONNECTED);

  // hold -> cfg mode
  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  // +6 s
  for (int i = 0; i < 600; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate inactive input on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);

  // +3400 ms
  for (int i = 0; i < 340; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  EXPECT_CALL(board, supla_system_restart()).Times(1);

  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  // +200 ms
  for (int i = 0; i < 20; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  // simulate inactive input on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);

  // +200 ms
  for (int i = 0; i < 20; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_FALSE(eagleStub.getGpioValue(2));
}
