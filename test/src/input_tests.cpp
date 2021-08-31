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


int gpioConfigId = 0;

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

    ets_clear_isr();
    gpioInitCb = nullptr;
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

  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(2));
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

TEST_F(InputsFixture, MonostableButtonWithRelayInactiveOnTrigger) {
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

  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
 
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));
 
  // simulate button release on gpio 1
  eagleStub.gpioOutputSet(1, 0);
  ets_gpio_intr_func(NULL);
  
  // +300 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // simulate button press on gpio 1
  eagleStub.gpioOutputSet(1, 1);
  ets_gpio_intr_func(NULL);
 
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(2));
 
  // simulate button release on gpio 1
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
 
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  // +700 ms
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(2));

}
