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
#include <srpc_mock.h>

extern "C" {
#include "board_stub.h"
#include <osapi.h>
#include <supla_esp_cfg.h>
#include <supla_esp_gpio.h>
#include <supla_esp_devconn.h>
}

#define RS_DIRECTION_NONE 0
#define RS_DIRECTION_UP 2
#define RS_DIRECTION_DOWN 1

#define UP_GPIO 1
#define DOWN_GPIO 2

// method will be called by supla_esp_gpio_init method in order to initialize
// gpio input/outputs board configuration (supla_esb_board_gpio_init)
void gpioCallbackFacadeBlind() {
  // up
  supla_relay_cfg[0].gpio_id = UP_GPIO;
  supla_relay_cfg[0].channel = 0;

  // down
  supla_relay_cfg[1].gpio_id = DOWN_GPIO;
  supla_relay_cfg[1].channel = 0;

  supla_rs_cfg[0].up = &supla_relay_cfg[0];
  supla_rs_cfg[0].down = &supla_relay_cfg[1];
  *supla_rs_cfg[0].position = 0;
  *supla_rs_cfg[0].tilt = 0;
  *supla_rs_cfg[0].full_closing_time = 0;
  *supla_rs_cfg[0].full_opening_time = 0;
  *supla_rs_cfg[0].tilt_change_time = 0;
  supla_rs_cfg[0].delayed_trigger.value = 0;
}

class FacadeBlindTestsF : public ::testing::Test {
public:
  TimeMock time;
  EagleSocStub eagleStub;

  void SetUp() override {
    supla_esp_gpio_rs_set_time_margin(110);
    memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
    memset(&supla_esp_state, 0, sizeof(SuplaEspState));
    memset(&supla_relay_cfg, 0, sizeof(supla_relay_cfg));
    supla_esp_gpio_init_time = 0;
    gpioInitCb = *gpioCallbackFacadeBlind;
  }

  void TearDown() override {
    supla_esp_gpio_rs_set_time_margin(110);
    cleanupTimers();
    memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
    memset(&supla_esp_state, 0, sizeof(SuplaEspState));
    supla_esp_gpio_init_time = 0;

    *supla_rs_cfg[0].position = 0;
    *supla_rs_cfg[0].full_closing_time = 0;
    *supla_rs_cfg[0].full_opening_time = 0;
    gpioInitCb = nullptr;
    supla_esp_gpio_clear_vars();
  }
};

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::SaveArg;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::Invoke;

TEST_F(FacadeBlindTestsF, FacadeBlindKeepPositionWhileTilting) {
  int curTime = 10000; // start at +10 ms
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  os_timer_func_t *rsTimerCb = lastTimerCb;

  // full opening and closing times are 10s, while tilt change takes 2s
  // facade blind is fully open
  *rsCfg->full_opening_time = 10000;
  *rsCfg->full_closing_time = 10000;
  *rsCfg->position = 100;
  *rsCfg->tilt_change_time = 2000;
  *rsCfg->tilt = 100;

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 10000);
  EXPECT_EQ(*rsCfg->full_closing_time, 10000);
  EXPECT_EQ(*rsCfg->tilt_change_time, 2000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +2000 ms
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 10000);
  EXPECT_EQ(*rsCfg->full_closing_time, 10000);
  EXPECT_EQ(*rsCfg->tilt_change_time, 2000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // Move RS down.
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_DOWN, 1, 1);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 100); // position shouldn't chnage
  EXPECT_EQ(*rsCfg->tilt, 100 + (50 * 100)); // tilt should be at 50%
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100)); // tilt should be at 100%
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  for (int i = 0; i < 1; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_NE(*rsCfg->position, 100);

  // +800 ms
  for (int i = 0; i < 79; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_EQ(*rsCfg->position, 100 + (10 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +7.2 s
  for (int i = 0; i < 720; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 100 + (100 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +15 s
  for (int i = 0; i < 1500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_UP, 1, 1);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (75 * 100));
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_DOWN, 1, 1);

  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (75 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  
  for (int i = 0; i < 60; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(FacadeBlindTestsF, FacadeBlindChangePositionWhileTilting) {
  int curTime = 10000; // start at +10 ms
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  rsCfg->tilt_type = 1;

  os_timer_func_t *rsTimerCb = lastTimerCb;

  // full opening and closing times are 10s, while tilt change takes 2s
  // facade blind is fully open
  *rsCfg->full_opening_time = 10000;
  *rsCfg->full_closing_time = 10000;
  *rsCfg->position = 100;
  *rsCfg->tilt_change_time = 2000;
  *rsCfg->tilt = 100;

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 10000);
  EXPECT_EQ(*rsCfg->full_closing_time, 10000);
  EXPECT_EQ(*rsCfg->tilt_change_time, 2000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +2000 ms
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 10000);
  EXPECT_EQ(*rsCfg->full_closing_time, 10000);
  EXPECT_EQ(*rsCfg->tilt_change_time, 2000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // Move RS down.
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_DOWN, 1, 1);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 100 + (10 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (50 * 100)); // tilt should be at 50%
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_EQ(*rsCfg->position, 100 + (20 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100)); // tilt should be at 100%
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  for (int i = 0; i < 1; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_NE(*rsCfg->position, 100 + (20 * 100));

  // +800 ms
  for (int i = 0; i < 99; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_EQ(*rsCfg->position, 100 + (30 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  for (int i = 0; i < 700; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 100 + (100 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +15 s
  for (int i = 0; i < 1500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_UP, 1, 1);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 100 + (95 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (75 * 100));
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_DOWN, 1, 1);

  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 100 + (95 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (75 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  
  for (int i = 0; i < 60; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(FacadeBlindTestsF, FacadeBlindFixedTilt) {
  // this time of facade blinds has fixed tilt at any position other than fully
  // closed. Tilting is possible only at fully closed

  int curTime = 10000; // start at +10 ms
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  rsCfg->tilt_type = FB_TILT_TYPE_TILTING_ONLY_AT_FULLY_CLOSED;

  os_timer_func_t *rsTimerCb = lastTimerCb;

  // full opening and closing times are 10s, while tilt change takes 2s
  // facade blind is fully open
  *rsCfg->full_opening_time = 10000;
  *rsCfg->full_closing_time = 10000;
  *rsCfg->position = 100;
  *rsCfg->tilt_change_time = 2000;
  *rsCfg->tilt = 100;

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 10000);
  EXPECT_EQ(*rsCfg->full_closing_time, 10000);
  EXPECT_EQ(*rsCfg->tilt_change_time, 2000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +2000 ms
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 10000);
  EXPECT_EQ(*rsCfg->full_closing_time, 10000);
  EXPECT_EQ(*rsCfg->tilt_change_time, 2000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // Move RS down.
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_DOWN, 1, 1);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +800 ms
  for (int i = 0; i < 80; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 100 + (10 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100); // tilt should not change
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +800 ms
  for (int i = 0; i < 80; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_EQ(*rsCfg->position, 100 + (20 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  for (int i = 0; i < 8*80; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_EQ(*rsCfg->position, 100 + (100 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }
  EXPECT_EQ(*rsCfg->position, 100 + (100 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (50 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 100 + (100 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +15 s
  for (int i = 0; i < 1500; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_UP, 1, 1);

  // +500 ms
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (75 * 100));
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_DOWN, 1, 1);

  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (75 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  
  for (int i = 0; i < 60; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));
}
