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

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::SaveArg;


// method will be called by supla_esp_gpio_init method in order to initialize
// gpio input/outputs board configuration (supla_esb_board_gpio_init)
void gpioCallbackRsSbsAndStop() {
  // up
  supla_relay_cfg[0].gpio_id = UP_GPIO;
  supla_relay_cfg[0].channel = 0;
  supla_relay_cfg[0].channel_flags = SUPLA_CHANNEL_FLAG_RS_AUTO_CALIBRATION |
    SUPLA_CHANNEL_FLAG_CALCFG_RECALIBRATE;

  // down
  supla_relay_cfg[1].gpio_id = DOWN_GPIO;
  supla_relay_cfg[1].channel = 0;
  supla_relay_cfg[1].channel_flags = SUPLA_CHANNEL_FLAG_RS_AUTO_CALIBRATION |
    SUPLA_CHANNEL_FLAG_CALCFG_RECALIBRATE;

  supla_rs_cfg[0].up = &supla_relay_cfg[0];
  supla_rs_cfg[0].down = &supla_relay_cfg[1];
  supla_rs_cfg[0].delayed_trigger.value = 0;
}

class RollerShutterSbsAndStopFixture : public ::testing::Test {
public:
  TimeMock time;
  EagleSocStub eagleStub;
  int curTime;

  void SetUp() override {
    curTime = 10000; // start at +10 ms
    EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));
    startupTimeDelay = 0;
    upTime = 1100;
    downTime = 1200;
    memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
    memset(&supla_esp_state, 0, sizeof(SuplaEspState));
    memset(&supla_relay_cfg, 0, sizeof(supla_relay_cfg));
    memset(&supla_rs_cfg, 0, sizeof(supla_rs_cfg));
    supla_esp_gpio_init_time = 0;
    gpioInitCb = *gpioCallbackRsSbsAndStop;
  }

  void TearDown() override {
    startupTimeDelay = 0;
    upTime = 1100;
    downTime = 1200;
    *supla_rs_cfg[0].position = 0;
    *supla_rs_cfg[0].full_closing_time = 0;
    *supla_rs_cfg[0].full_opening_time = 0;
    memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
    memset(&supla_esp_state, 0, sizeof(SuplaEspState));
    memset(&supla_relay_cfg, 0, sizeof(supla_relay_cfg));
    memset(&supla_rs_cfg, 0, sizeof(supla_rs_cfg));
    supla_esp_gpio_init_time = 0;
    supla_esp_gpio_clear_vars();

    gpioInitCb = nullptr;
  }
  void moveTime(int ms) {
    for (int i = 0; i < (ms / 10); i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }
};


TEST_F(RollerShutterSbsAndStopFixture, MoveDownOrStop) {

  supla_esp_cfg.Time1[0] = 1000;
  supla_esp_cfg.Time2[0] = 1000;
  supla_esp_cfg.AutoCalCloseTime[0] = 0;
  supla_esp_cfg.AutoCalOpenTime[0] = 0;
  supla_esp_state.rs_position[0] = 100;
  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  moveTime(1000);

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  // closing and opening time is coded in 0.1s units on 2x16 bit blocks of
  // DurationMS
  reqValue.DurationMS = (10) | (10 << 16);
  reqValue.value[0] = 3; // move down or stop

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  moveTime(500);

  EXPECT_EQ(*rsCfg->position, 50*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // simulate request from server
  supla_esp_channel_set_value(&reqValue); // move down or stop

  moveTime(1500);

  EXPECT_EQ(*rsCfg->position, 50*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // simulate request from server
  supla_esp_channel_set_value(&reqValue); // move down or stop

  moveTime(400);

  EXPECT_EQ(*rsCfg->position, 90*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // simulate request from server
  supla_esp_channel_set_value(&reqValue); // move down or stop

  moveTime(500);

  EXPECT_EQ(*rsCfg->position, 90*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(RollerShutterSbsAndStopFixture, MoveUpOrStop) {

  supla_esp_cfg.Time1[0] = 1000;
  supla_esp_cfg.Time2[0] = 1000;
  supla_esp_cfg.AutoCalCloseTime[0] = 0;
  supla_esp_cfg.AutoCalOpenTime[0] = 0;
  supla_esp_state.rs_position[0] = 100*100+100;
  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  moveTime(1000);

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  // closing and opening time is coded in 0.1s units on 2x16 bit blocks of
  // DurationMS
  reqValue.DurationMS = (10) | (10 << 16);
  reqValue.value[0] = 4; // move up or stop

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  moveTime(500);

  EXPECT_EQ(*rsCfg->position, 50*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // simulate request from server
  supla_esp_channel_set_value(&reqValue); // move up or stop

  moveTime(1500);

  EXPECT_EQ(*rsCfg->position, 50*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // simulate request from server
  supla_esp_channel_set_value(&reqValue); // move up or stop

  moveTime(400);

  EXPECT_EQ(*rsCfg->position, 10*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // simulate request from server
  supla_esp_channel_set_value(&reqValue); // move up or stop

  moveTime(500);

  EXPECT_EQ(*rsCfg->position, 10*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(RollerShutterSbsAndStopFixture, MoveUpAndDownOrStop) {

  supla_esp_cfg.Time1[0] = 1000;
  supla_esp_cfg.Time2[0] = 1000;
  supla_esp_cfg.AutoCalCloseTime[0] = 0;
  supla_esp_cfg.AutoCalOpenTime[0] = 0;
  supla_esp_state.rs_position[0] = 100*100+100;
  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  moveTime(1000);

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  // closing and opening time is coded in 0.1s units on 2x16 bit blocks of
  // DurationMS
  reqValue.DurationMS = (10) | (10 << 16);
  reqValue.value[0] = 4; // move up or stop

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  moveTime(500);

  EXPECT_EQ(*rsCfg->position, 50*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // simulate request from server
  reqValue.value[0] = 3; // move down or stop -> should stop
  supla_esp_channel_set_value(&reqValue);

  moveTime(1500);

  EXPECT_EQ(*rsCfg->position, 50*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // simulate request from server
  supla_esp_channel_set_value(&reqValue); // move down or stop

  moveTime(400);

  EXPECT_EQ(*rsCfg->position, 90*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // simulate request from server
  reqValue.value[0] = 4; // move up or stop -> should stop
  supla_esp_channel_set_value(&reqValue);

  moveTime(1500);

  EXPECT_EQ(*rsCfg->position, 90*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_channel_set_value(&reqValue); // move up or stop
  moveTime(500);

  EXPECT_EQ(*rsCfg->position, 40*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
}


TEST_F(RollerShutterSbsAndStopFixture, Sbs) {

  supla_esp_cfg.Time1[0] = 1000;
  supla_esp_cfg.Time2[0] = 1000;
  supla_esp_cfg.AutoCalCloseTime[0] = 0;
  supla_esp_cfg.AutoCalOpenTime[0] = 0;
  supla_esp_state.rs_position[0] = 100*100+100;
  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  moveTime(1000);

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  // closing and opening time is coded in 0.1s units on 2x16 bit blocks of
  // DurationMS
  reqValue.DurationMS = (10) | (10 << 16);
  reqValue.value[0] = 5; // sbs -> up

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  moveTime(500);

  EXPECT_EQ(*rsCfg->position, 50*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // simulate request from server
  supla_esp_channel_set_value(&reqValue); // sbs -> stop

  moveTime(1500);

  EXPECT_EQ(*rsCfg->position, 50*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // simulate request from server
  supla_esp_channel_set_value(&reqValue); // sbs -> down

  moveTime(400);

  EXPECT_EQ(*rsCfg->position, 90*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // simulate request from server
  supla_esp_channel_set_value(&reqValue); // sbs -> stop

  moveTime(1500);

  EXPECT_EQ(*rsCfg->position, 90*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_channel_set_value(&reqValue); // move up or stop
  moveTime(500);

  EXPECT_EQ(*rsCfg->position, 40*100+100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
}
