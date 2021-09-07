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
    supla_input_cfg[0].supported_actions = SUPLA_ACTION_CAP_SHORT_PRESS_x2;
    supla_input_cfg[0].flags = INPUT_FLAG_TRIGGER_ON_PRESS;
  } else if (gpioConfigId == 1) {
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[0].supported_actions = SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x3 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x4 | 
      SUPLA_ACTION_CAP_SHORT_PRESS_x5;
    supla_input_cfg[0].flags = INPUT_FLAG_TRIGGER_ON_PRESS;
  } else if (gpioConfigId == 2) {
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[0].supported_actions = SUPLA_ACTION_CAP_HOLD |
      SUPLA_ACTION_CAP_SHORT_PRESS_x3 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x4 | 
      SUPLA_ACTION_CAP_SHORT_PRESS_x5;
    supla_input_cfg[0].flags = INPUT_FLAG_TRIGGER_ON_PRESS;
  } else if (gpioConfigId == 3) {
    supla_input_cfg[0].flags = INPUT_FLAG_CFG_BTN;
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
  } else if (gpioConfigId == 4) {
    supla_input_cfg[0].flags = INPUT_FLAG_PULLUP;
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
  } else if (gpioConfigId == 5) {
    supla_input_cfg[0].flags = INPUT_FLAG_PULLUP | INPUT_FLAG_CFG_BTN;
    supla_input_cfg[0].type = INPUT_TYPE_BTN_BISTABLE;
  } else if (gpioConfigId == 6) {
    supla_input_cfg[0].flags = INPUT_FLAG_CFG_BTN;
    supla_input_cfg[0].type = INPUT_TYPE_BTN_BISTABLE;
  } else if (gpioConfigId == 7) {
    supla_input_cfg[0].flags = INPUT_FLAG_PULLUP;
    supla_input_cfg[0].type = INPUT_TYPE_BTN_BISTABLE;
  } else if (gpioConfigId == 8) {
    supla_input_cfg[0].flags = INPUT_FLAG_CFG_BTN;
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE_RS;
    supla_input_cfg[0].relay_gpio_id = 3;

    supla_input_cfg[1].type = INPUT_TYPE_BTN_MONOSTABLE_RS;
    supla_input_cfg[1].gpio_id = 2;
    supla_input_cfg[1].flags = 0;
    supla_input_cfg[1].channel = 255; // used only for sensor input
    supla_input_cfg[1].relay_gpio_id = 4;

    // up
    supla_relay_cfg[0].gpio_id = 3;
    supla_relay_cfg[0].channel = 0;
    supla_relay_cfg[0].flags = 0;
    supla_relay_cfg[0].channel_flags = 0;

    // down
    supla_relay_cfg[1].gpio_id = 4;
    supla_relay_cfg[1].channel = 0;

    supla_rs_cfg[0].up = &supla_relay_cfg[0];
    supla_rs_cfg[0].down = &supla_relay_cfg[1];
    *supla_rs_cfg[0].position = 0;
    *supla_rs_cfg[0].full_closing_time = 0;
    *supla_rs_cfg[0].full_opening_time = 0;
    supla_rs_cfg[0].delayed_trigger.value = 0;
  } else if (gpioConfigId == 9) {
    supla_input_cfg[0].flags = 0;
    supla_input_cfg[0].type = INPUT_TYPE_SENSOR;
    supla_input_cfg[0].channel = 1;
  } else if (gpioConfigId == 10) {
    supla_input_cfg[0].flags = INPUT_FLAG_CFG_BTN | INPUT_FLAG_FACTORY_RESET;
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
  } else if (gpioConfigId == 11) {
    supla_input_cfg[0].flags = INPUT_FLAG_TRIGGER_ON_PRESS;
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
  } else if (gpioConfigId == 12) {
    supla_input_cfg[0].flags = INPUT_FLAG_TRIGGER_ON_PRESS | 
                               INPUT_FLAG_CFG_BTN | 
                               INPUT_FLAG_CFG_ON_TOGGLE;
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
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
    curTime = 10000; 
    currentDeviceState = STATE_UNKNOWN;
    EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));
    memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
    memset(&supla_esp_state, 0, sizeof(SuplaEspState));
    memset(&supla_relay_cfg, 0, sizeof(supla_relay_cfg));
    memset(&supla_rs_cfg, 0, sizeof(supla_rs_cfg));
    gpioInitCb = gpioCallbackAt;
    supla_esp_gpio_init_time = 0;
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
};

TSD_SuplaRegisterDeviceResult regAt;

char custom_srpc_getdata_at(void *_srpc, TsrpcReceivedData *rd,
    unsigned _supla_int_t rr_id) {
  rd->call_type = SUPLA_SD_CALL_REGISTER_DEVICE_RESULT;
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
    EXPECT_CALL(srpc, srpc_set_proto_version(_, 16));

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

    // Expected channel 1 (ActionTrigger) state changes
    char value[SUPLA_CHANNELVALUE_SIZE] = {};
    int action = SUPLA_ACTION_CAP_SHORT_PRESS_x2;
    memcpy(value, &action, sizeof(action));
    EXPECT_CALL(srpc, valueChanged(_, 1, ElementsAreArray(value)));

    // RELAY OFF
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

  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is not enabled yet, so monostable button should change relay on press
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate inactive sensor on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +700 ms
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(supla_input_cfg[0].active_actions, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TSD_ChannelConfig_ActionTrigger);

  TSD_ChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x2;
  memcpy(configResult.Config, &atSettings,
      sizeof(TSD_ChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_actions, SUPLA_ACTION_CAP_SHORT_PRESS_x2);

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

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // AT is enabled, so monostable button shouldn't change relay 
  EXPECT_TRUE(eagleStub.getGpioValue(2));

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

TEST_F(ATRegisteredFixture, MonostablePressMultipleWithout3x) {
  gpioConfigId = 1;

  {
    InSequence seq;
    // Expected channel 1 (ActionTrigger) state changes
    char value[SUPLA_CHANNELVALUE_SIZE] = {};
    int action = SUPLA_ACTION_CAP_SHORT_PRESS_x2;
    memcpy(value, &action, sizeof(action));
    EXPECT_CALL(srpc, valueChanged(_, 1, ElementsAreArray(value)));

    action = SUPLA_ACTION_CAP_SHORT_PRESS_x5;
    memcpy(value, &action, sizeof(action));
    EXPECT_CALL(srpc, valueChanged(_, 1, ElementsAreArray(value)));

    action = SUPLA_ACTION_CAP_SHORT_PRESS_x4;
    memcpy(value, &action, sizeof(action));
    EXPECT_CALL(srpc, valueChanged(_, 1, ElementsAreArray(value)));

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

  EXPECT_EQ(supla_input_cfg[0].active_actions, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TSD_ChannelConfig_ActionTrigger);

  TSD_ChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x5;
  memcpy(configResult.Config, &atSettings,
      sizeof(TSD_ChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_actions,
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
    char value[SUPLA_CHANNELVALUE_SIZE] = {};
    int action = SUPLA_ACTION_CAP_SHORT_PRESS_x3;
    memcpy(value, &action, sizeof(action));
    EXPECT_CALL(srpc, valueChanged(_, 1, ElementsAreArray(value)));

    action = SUPLA_ACTION_CAP_HOLD;
    memcpy(value, &action, sizeof(action));
    EXPECT_CALL(srpc, valueChanged(_, 1, ElementsAreArray(value)));

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

  EXPECT_EQ(supla_input_cfg[0].active_actions, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TSD_ChannelConfig_ActionTrigger);

  TSD_ChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x3 |
    SUPLA_ACTION_CAP_HOLD;

  memcpy(configResult.Config, &atSettings,
      sizeof(TSD_ChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_actions,
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


  // 4x press - not activated
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
    char value[SUPLA_CHANNELVALUE_SIZE] = {};
    int action = SUPLA_ACTION_CAP_SHORT_PRESS_x5;
    memcpy(value, &action, sizeof(action));
    EXPECT_CALL(srpc, valueChanged(_, 1, ElementsAreArray(value)));

    EXPECT_CALL(srpc, valueChanged(_, 1, ElementsAreArray(value)));

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

  EXPECT_EQ(supla_input_cfg[0].active_actions, 0);

  TSD_ChannelConfig configResult = {};
  configResult.ChannelNumber = 1; // AT channel number
  configResult.Func = SUPLA_CHANNELFNC_ACTIONTRIGGER;
  configResult.ConfigType = 0;
  configResult.ConfigSize = sizeof(TSD_ChannelConfig_ActionTrigger);

  TSD_ChannelConfig_ActionTrigger atSettings = {};
  atSettings.ActiveActions = SUPLA_ACTION_CAP_SHORT_PRESS_x2 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
    SUPLA_ACTION_CAP_SHORT_PRESS_x5;
  memcpy(configResult.Config, &atSettings,
      sizeof(TSD_ChannelConfig_ActionTrigger));

  supla_esp_channel_config_result(&configResult);

  EXPECT_EQ(supla_input_cfg[0].active_actions,
      SUPLA_ACTION_CAP_SHORT_PRESS_x2 | SUPLA_ACTION_CAP_SHORT_PRESS_x4 |
      SUPLA_ACTION_CAP_SHORT_PRESS_x5);

  // 8x press
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
