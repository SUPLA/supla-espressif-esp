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
void gpioCallbackInput() {
  supla_input_cfg[0].gpio_id = 1;
  supla_input_cfg[0].flags = 0;
  supla_input_cfg[0].channel = 255; // used only for sensor input
  supla_input_cfg[0].relay_gpio_id = 2;

  supla_relay_cfg[0].gpio_id = 2;
  supla_relay_cfg[0].channel = 0;
  supla_relay_cfg[0].flags = RELAY_FLAG_RESTORE_FORCE;
  supla_relay_cfg[0].channel_flags =
    SUPLA_CHANNEL_FLAG_COUNTDOWN_TIMER_SUPPORTED;
  if (gpioConfigId == 0) {
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
  } else if (gpioConfigId == 1) {
    supla_input_cfg[0].type = INPUT_TYPE_BTN_BISTABLE;
  } else if (gpioConfigId == 2) {
    supla_input_cfg[0].flags = INPUT_FLAG_PULLUP | INPUT_FLAG_CFG_BTN;
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
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
  } else if (gpioConfigId == 13) {
    supla_input_cfg[0].flags = INPUT_FLAG_TRIGGER_ON_PRESS |
                               INPUT_FLAG_CFG_BTN |
                               INPUT_FLAG_CFG_ON_TOGGLE |
                               INPUT_FLAG_CFG_ON_HOLD;
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
  } else if (gpioConfigId == 14) {
    supla_input_cfg[0].flags = INPUT_FLAG_CFG_BTN |
                               INPUT_FLAG_FACTORY_RESET;
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[0].relay_gpio_id = 255;

    supla_input_cfg[1].gpio_id = 3;
    supla_input_cfg[1].flags = 0;
    supla_input_cfg[1].channel = 255;
    supla_input_cfg[1].relay_gpio_id = 2;
    supla_input_cfg[1].flags = INPUT_FLAG_CFG_BTN;
    supla_input_cfg[1].type = INPUT_TYPE_BTN_MONOSTABLE;
  } else if (gpioConfigId == 15) {
    supla_input_cfg[0].type = INPUT_TYPE_MOTION_SENSOR;
  } else {
    assert(false);
  }
}


class InputsFixture : public ::testing::Test {
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
    gpioInitCb = gpioCallbackInput;
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
      curTime += 10000; // +1s
      executeTimers();
    }
  }
};

TSD_SuplaRegisterDeviceResult regInputs;

char custom_srpc_getdata_inputs(void *_srpc, TsrpcReceivedData *rd,
    unsigned _supla_int_t rr_id) {
  rd->call_type = SUPLA_SD_CALL_REGISTER_DEVICE_RESULT;
  rd->data.sd_register_device_result = &regInputs;
  return 1;
}

class InputsRegisteredFixture: public InputsFixture {
public:
  SrpcMock srpc;

  void SetUp() override {
    InputsFixture::SetUp();
    EXPECT_CALL(srpc, srpc_params_init(_));
    EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return((void *)1));
    EXPECT_CALL(srpc, srpc_set_proto_version(_, 17));

    regInputs.result_code = SUPLA_RESULTCODE_TRUE;

    EXPECT_CALL(srpc, srpc_getdata(_, _, _))
      .WillOnce(DoAll(Invoke(custom_srpc_getdata_inputs), Return(1)));
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
    InputsFixture::TearDown();
    supla_esp_devconn_release();
  }
};

class InputsFactoryDefaultFixture: public InputsFixture {
public:
  void SetUp() override {
    InputsFixture::SetUp();
    memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
    supla_esp_cfgmode_start();
  }

  void setButton(int gpio, int state, int timeMs) {
    eagleStub.gpioOutputSet(gpio, state);
    ets_gpio_intr_func(NULL);

    for (int i = 0; i < timeMs / 10; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }

  }

};


TEST_F(InputsFixture, MonostableButtonWithRelay) {
  gpioConfigId = 0;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  // GPIO 2 - relay
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

}

TEST_F(InputsFixture, BistableButtonWithRelay) {
  gpioConfigId = 1;

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +300 ms
  for (int i = 0; i < 60; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

}

TEST_F(InputsFixture, BistableButtonWithRelayInvertedInitState) {
  gpioConfigId = 1;

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  supla_esp_gpio_relay_hi(2, 1, 0); // turn gpio on

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);

  // +300 ms
  for (int i = 0; i < 60; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

}

TEST_F(InputsFixture, MonostableCfgPullupButtonWithRelay) {
  gpioConfigId = 2;
  eagleStub.gpioOutputSet(1, 1);

  // GPIO 1 - input button
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  // GPIO 2 - relay
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
 
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
 
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 700; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
 
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 700; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

}

TEST_F(InputsFixture, MonostableCfgButtonWithRelay) {
  gpioConfigId = 3;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  // GPIO 2 - relay
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 700; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 700; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_UNKNOWN);
  // enter cfg mode
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +6 s
  for (int i = 0; i < 600; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // Button click >3s after enter cfg mode should trigger cfgmode exit
  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // only 900 ms passed, so we should still be in cfgmode
  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // +2500 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);
 
  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_CALL(board, supla_system_restart()).Times(1);
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }


  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);
}

TEST_F(InputsFixture, MonostablePullupButtonWithRelay) {
  gpioConfigId = 4;
  eagleStub.gpioOutputSet(1, 1);

  // GPIO 1 - input button
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  // GPIO 2 - relay
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
 
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
 
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 700; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
 
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 700; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

}

TEST_F(InputsFixture, BistablePullupCfgButtonWithRelay) {
  gpioConfigId = 5;

  eagleStub.gpioOutputSet(1, 1);

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
  
  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
 
  // +500 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
 
  // +300 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
 
  // +300 ms
  for (int i = 0; i < 60; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

}

TEST_F(InputsFixture, BistablePullupButtonWithRelay) {
  gpioConfigId = 7;

  eagleStub.gpioOutputSet(1, 1);

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
  
  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
 
  // +500 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
 
  // +300 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
 
  // +300 ms
  for (int i = 0; i < 60; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

}

TEST_F(InputsFixture, BistableCfgButtonWithRelay) {
  gpioConfigId = 6;

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +500 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +300 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +3 s
  for (int i = 0; i < 300; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_UNKNOWN);
  // enter cfg mode
  for (int cycle = 0; cycle < 5; cycle++) {
    EXPECT_EQ(currentDeviceState, STATE_UNKNOWN);
    // simulate button press on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);
  
    // +0.5 s
    for (int i = 0; i < 50; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
    // simulate button reelase on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);
  
    // +0.5 s
    for (int i = 0; i < 50; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);
}

TEST_F(InputsRegisteredFixture, SensorInput) {
  gpioConfigId = 9;

  EXPECT_CALL(srpc, 
      valueChanged(_, 1, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})));
  EXPECT_CALL(srpc, 
      valueChanged(_, 1, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})));

  // GPIO 1 - input sensor
  EXPECT_FALSE(eagleStub.getGpioValue(1));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // simulate active input on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // simulate inactive sensor on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +700 ms
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
}

TEST_F(InputsFixture, MonostableCfgButtonFactoryReset) {
  gpioConfigId = 10;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  // GPIO 2 - relay
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 700; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 700; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_UNKNOWN);
  // enter cfg mode
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +6 s
  for (int i = 0; i < 600; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // Hold of monostable input in cfgmode should trigger factory reset
  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // +2500 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  EXPECT_CALL(board, factory_reset()).Times(1);
  EXPECT_CALL(board, supla_system_restart()).Times(1);
  // +5100 ms
  for (int i = 0; i < 550; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);
 
}

TEST_F(InputsFixture, MonostableCfgButtonWithResetOnPress) {
  gpioConfigId = 10;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  // GPIO 2 - relay
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 700; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 700; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_UNKNOWN);
  // enter cfg mode
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +6 s
  for (int i = 0; i < 600; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  supla_esp_restart_on_cfg_press = 1;
  EXPECT_CALL(board, factory_reset()).Times(0);
  EXPECT_CALL(board, supla_system_restart()).Times(1);

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

}

TEST_F(InputsFixture, MonostableButtonWithResetOnPress) {
  gpioConfigId = 10;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  // GPIO 2 - relay
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_EQ(currentDeviceState, STATE_UNKNOWN);
  supla_esp_restart_on_cfg_press = 1;
  EXPECT_CALL(board, factory_reset()).Times(0);
  EXPECT_CALL(board, supla_system_restart()).Times(1);

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

}

TEST_F(InputsFixture, MonostableCfgButtonWithLongHold) {
  gpioConfigId = 10;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  // GPIO 2 - relay
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_CALL(board, factory_reset()).Times(0);
  EXPECT_CALL(board, supla_system_restart()).Times(0);

  EXPECT_EQ(currentDeviceState, STATE_UNKNOWN);
  // enter cfg mode
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +6 s
  for (int i = 0; i < 1000; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));

}

TEST_F(InputsFixture, MonostableButtonTriggerOnPress) {
  gpioConfigId = 11;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  // GPIO 2 - relay
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_CALL(board, factory_reset()).Times(0);
  EXPECT_CALL(board, supla_system_restart()).Times(0);

  // enter cfg mode
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

}

TEST_F(InputsFixture, MonostableCfgOnToggle) {
  gpioConfigId = 12;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  // GPIO 2 - relay
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_CALL(board, supla_system_restart()).Times(0);

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 700; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  EXPECT_EQ(currentDeviceState, STATE_UNKNOWN);
  // enter cfg mode - on hold is disabled, so it shouldn't enter cfg mode
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +6 s
  for (int i = 0; i < 600; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_UNKNOWN);

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +3200 ms
  for (int i = 0; i < 320; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  for (int cycle = 0; cycle < 10; cycle++) {
    EXPECT_EQ(currentDeviceState, STATE_UNKNOWN);
    // simulate button press on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +0.5 s
    for (int i = 0; i < 50; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
    // simulate button reelase on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +0.5 s
    for (int i = 0; i < 50; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);
}

TEST_F(InputsFixture, MonostableCfgOnToggleAndOnHold1) {
  gpioConfigId = 13;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  // GPIO 2 - relay
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_CALL(board, supla_system_restart()).Times(0);

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 700; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  EXPECT_EQ(currentDeviceState, STATE_UNKNOWN);
  // enter cfg mode - 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +6 s
  for (int i = 0; i < 600; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +3200 ms
  for (int i = 0; i < 320; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  
  // toggle button to leave cfg mode
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +0.6 s
  for (int i = 0; i < 60; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

}

TEST_F(InputsFixture, MonostableCfgOnToggleAndOnHold2) {
  gpioConfigId = 13;

  // GPIO 1 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  // GPIO 2 - relay
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_CALL(board, supla_system_restart()).Times(0);

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 700; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  EXPECT_EQ(currentDeviceState, STATE_UNKNOWN);
  // enter cfg mode - 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +4 s
  for (int i = 0; i < 400; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(currentDeviceState, STATE_UNKNOWN);

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  // +3200 ms
  for (int i = 0; i < 320; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  for (int cycle = 0; cycle < 10; cycle++) {
    EXPECT_EQ(currentDeviceState, STATE_UNKNOWN);
    // simulate button press on gpio 1
    eagleStub.gpioOutputSet(1, 1);
    ets_gpio_intr_func(NULL);

    // +0.5 s
    for (int i = 0; i < 50; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
    // simulate button reelase on gpio 1
    eagleStub.gpioOutputSet(1, 0);
    ets_gpio_intr_func(NULL);

    // +0.5 s
    for (int i = 0; i < 50; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);
}

TEST_F(InputsFactoryDefaultFixture, CfgButtonTriggersReset) {
  gpioConfigId = 14;

  // GPIO 1, 3 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  // GPIO 2 - relay
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  setButton(1, 1, 300);
  setButton(1, 0, 700);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  EXPECT_CALL(board, supla_system_restart()).Times(1);

  // +5000 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  setButton(1, 1, 300);
  setButton(1, 0, 700);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
}

// In this scenario, device is in config mode but it doesn't have complete
// configuration (i.e. with factory defaults). In this case, input buttons
// associated with control over other elements (like relays), should
// control those elements and should not cause exit from cfg mode
TEST_F(InputsFactoryDefaultFixture, InputButtonShouldNotTriggersReset) {
  gpioConfigId = 14;

  // GPIO 1, 3 - input button
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  // GPIO 2 - relay
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  EXPECT_EQ(currentDeviceState, STATE_CFGMODE);
  EXPECT_CALL(board, supla_system_restart()).Times(0);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  setButton(3, 1, 300);
  setButton(3, 0, 700);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  // relay
  EXPECT_TRUE(eagleStub.getGpioValue(2));


  // +5000 ms
  for (int i = 0; i < 500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  setButton(3, 1, 300);
  setButton(3, 0, 700);

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  setButton(3, 1, 300);
  setButton(3, 0, 700);

  EXPECT_TRUE(eagleStub.getGpioValue(2));
}

TEST_F(InputsFixture, MotionSensorWithRelay) {
  gpioConfigId = 15;

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  moveTime(1000);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  moveTime(500);
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);

  moveTime(500);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  moveTime(300);
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);

  moveTime(300);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  moveTime(300);
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);

  moveTime(600);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
}

TEST_F(InputsFixture, MotionSensorWithRelayInvertedInitState) {
  gpioConfigId = 15;

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  supla_esp_gpio_init();
  ASSERT_NE(ets_gpio_intr_func, nullptr);

  supla_esp_gpio_relay_hi(2, 1, 0); // turn gpio on

  moveTime(1000);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  moveTime(500);
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);

  moveTime(500);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  moveTime(300);
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);

  moveTime(300);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);

  moveTime(300);
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);

  moveTime(600);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
}
