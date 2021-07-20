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
}

#define RS_DIRECTION_NONE 0
#define RS_DIRECTION_UP 2
#define RS_DIRECTION_DOWN 1

#define UP_GPIO 1
#define DOWN_GPIO 2

// method will be called by supla_esp_gpio_init method in order to initialize
// gpio input/outputs board configuration (supla_esb_board_gpio_init)
void gpioCallback1() {
  // up
  supla_relay_cfg[0].gpio_id = UP_GPIO;
  supla_relay_cfg[0].channel = 0;

  // down
  supla_relay_cfg[1].gpio_id = DOWN_GPIO;
  supla_relay_cfg[1].channel = 0;

  supla_rs_cfg[0].up = &supla_relay_cfg[0];
  supla_rs_cfg[0].down = &supla_relay_cfg[1];
  *supla_rs_cfg[0].position = 0;
  *supla_rs_cfg[0].full_closing_time = 0;
  *supla_rs_cfg[0].full_opening_time = 0;
  supla_rs_cfg[0].delayed_trigger.value = 0;
}

class RollerShutterTestsF : public ::testing::Test {
public:
  TimeMock time;
  EagleSocStub eagleStub;

  void SetUp() override {
    supla_esp_gpio_init_time = 0;
    gpioInitCb = *gpioCallback1;
  }

  void TearDown() override {
    supla_esp_gpio_init_time = 0;

    *supla_rs_cfg[0].position = 0;
    *supla_rs_cfg[0].full_closing_time = 0;
    *supla_rs_cfg[0].full_opening_time = 0;
    gpioInitCb = nullptr;
  }
};

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::SaveArg;

TEST(RollerShutterTests, RsGetValueWithNullCfg) {
  EXPECT_EQ(supla_esp_gpio_rs_get_value(nullptr), RS_RELAY_OFF);
}

TEST(RollerShutterTests, RsGetValueOff) {
  EagleSocMock eagleMock;

  supla_relay_cfg_t up = {};
  up.gpio_id = 1;
  supla_relay_cfg_t down = {};
  down.gpio_id = 2;
  supla_roller_shutter_cfg_t rsCfg = {};
  rsCfg.up = &up;
  rsCfg.down = &down;

  EXPECT_CALL(eagleMock, gpioRegRead(0)).WillRepeatedly(Return(0b0000));

  EXPECT_EQ(supla_esp_gpio_rs_get_value(&rsCfg), RS_RELAY_OFF);
}

TEST(RollerShutterTests, RsGetValueOff2) {
  EagleSocMock eagleMock;

  supla_relay_cfg_t up = {};
  up.gpio_id = 1;
  supla_relay_cfg_t down = {};
  down.gpio_id = 2;
  supla_roller_shutter_cfg_t rsCfg = {};
  rsCfg.up = &up;
  rsCfg.down = &down;

  // gpio status is returned as bit map, so gpio 1 value is on 0b0010, etc
  EXPECT_CALL(eagleMock, gpioRegRead(0)).WillRepeatedly(Return(0b1001));

  EXPECT_EQ(supla_esp_gpio_rs_get_value(&rsCfg), RS_RELAY_OFF);
}

TEST(RollerShutterTests, RsGetValueUp) {
  EagleSocMock eagleMock;

  supla_relay_cfg_t up = {};
  up.gpio_id = 1;
  supla_relay_cfg_t down = {};
  down.gpio_id = 2;
  supla_roller_shutter_cfg_t rsCfg = {};
  rsCfg.up = &up;
  rsCfg.down = &down;

  // gpio status is returned as bit map, so gpio 1 value is on 0b0010, etc
  EXPECT_CALL(eagleMock, gpioRegRead(0)).WillRepeatedly(Return(0b0010));

  EXPECT_EQ(supla_esp_gpio_rs_get_value(&rsCfg), RS_RELAY_UP);
}

TEST(RollerShutterTests, RsGetValueDown) {
  EagleSocMock eagleMock;

  supla_relay_cfg_t up = {};
  up.gpio_id = 1;
  supla_relay_cfg_t down = {};
  down.gpio_id = 2;
  supla_roller_shutter_cfg_t rsCfg = {};
  rsCfg.up = &up;
  rsCfg.down = &down;

  // gpio status is returned as bit map, so gpio 1 value is on 0b0010, etc
  EXPECT_CALL(eagleMock, gpioRegRead(0)).WillRepeatedly(Return(0b0100));

  EXPECT_EQ(supla_esp_gpio_rs_get_value(&rsCfg), RS_RELAY_DOWN);
}

TEST_F(RollerShutterTestsF, gpioInitWithRsAndTasks) {
  const int InitTime = 123;
  EXPECT_CALL(time, system_get_time()).WillOnce(Return(InitTime));

  supla_esp_gpio_init();

  EXPECT_EQ(supla_rs_cfg[0].stop_time, InitTime);
  // second RS is not used, so time should be default
  EXPECT_EQ(supla_rs_cfg[1].stop_time, 0);

  EXPECT_NE(supla_esp_gpio_get_rs__cfg(1), nullptr);
  EXPECT_NE(supla_esp_gpio_get_rs__cfg(2), nullptr);
  EXPECT_EQ(supla_esp_gpio_get_rs__cfg(3), nullptr);
  EXPECT_EQ(supla_esp_gpio_get_rs__cfg(0), nullptr);

  EXPECT_EQ(supla_esp_gpio_get_rs__cfg(1), supla_esp_gpio_get_rs__cfg(2));

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);
  EXPECT_EQ(*rsCfg->position, 0);

  EXPECT_EQ(rsCfg->task.percent, 0);
  EXPECT_EQ(rsCfg->task.direction, RS_DIRECTION_NONE);
  EXPECT_EQ(rsCfg->task.active, 0);

  supla_esp_gpio_rs_add_task(0, 10);

  EXPECT_EQ(rsCfg->task.percent, 10);
  EXPECT_EQ(rsCfg->task.direction, RS_DIRECTION_NONE);
  EXPECT_EQ(rsCfg->task.active, 1);

  // make sure that nothing happens when we call non existing RS (1 and 2)
  supla_esp_gpio_rs_add_task(1, 20);
  supla_esp_gpio_rs_add_task(2, 30);

  EXPECT_EQ(rsCfg->task.percent, 10);
  EXPECT_EQ(rsCfg->task.direction, RS_DIRECTION_NONE);
  EXPECT_EQ(rsCfg->task.active, 1);

  supla_esp_gpio_rs_add_task(0, 20);

  EXPECT_EQ(rsCfg->task.percent, 20);
  EXPECT_EQ(rsCfg->task.direction, RS_DIRECTION_NONE);
  EXPECT_EQ(rsCfg->task.active, 1);

  supla_esp_gpio_rs_cancel_task(rsCfg);

  EXPECT_EQ(rsCfg->task.percent, 0);
  EXPECT_EQ(rsCfg->task.direction, RS_DIRECTION_NONE);
  EXPECT_EQ(rsCfg->task.active, 0);

  supla_esp_gpio_rs_cancel_task(nullptr);

  // check if task is not executed when current possition is the same as
  // requested
  *rsCfg->position = 1600; // 15 * 100 + 100
  supla_esp_gpio_rs_add_task(0, 15);

  EXPECT_EQ(rsCfg->task.percent, 0);
  EXPECT_EQ(rsCfg->task.direction, RS_DIRECTION_NONE);
  EXPECT_EQ(rsCfg->task.active, 0);

  supla_esp_gpio_rs_add_task(0, 16);

  EXPECT_EQ(rsCfg->task.percent, 16);
  EXPECT_EQ(rsCfg->task.direction, RS_DIRECTION_NONE);
  EXPECT_EQ(rsCfg->task.active, 1);
}

TEST_F(RollerShutterTestsF, MoveDownNotCalibrated) {
  int curTime = 123;
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  os_timer_func_t *rsTimerCb = lastTimerCb;

  EXPECT_EQ(rsCfg->last_time, 0);
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);

  curTime = 2000000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 2000000);
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);

  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 2000000);
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // Move RS down. Not calirbated. No stop. It should timeout after > 60s
  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_DOWN, 0, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +100.000 ms
  curTime += 100000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 2100000);
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 100);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +100.000 ms
  curTime += 100000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 2200000);
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 200);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +59100.000 ms
  curTime += 59100000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 61300000);
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 59300);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +9 min
  curTime += 9 * 60 * 1000000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 601300000);
  EXPECT_EQ(rsCfg->last_comm_time, 601300000);
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 599300);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +1s
  curTime += 1000000;
  rsTimerCb(rsCfg);
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 602300000);
  EXPECT_EQ(rsCfg->last_comm_time, 602300000);
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 600300);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // position should not change, because RS is not calibrated
  EXPECT_EQ(*supla_rs_cfg[0].position, 0);

  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 602300000);
  EXPECT_EQ(rsCfg->last_comm_time, 602300000);
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  curTime += 1000000;
  rsTimerCb(rsCfg);
  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_DOWN, 0, 0);

  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +100.000 ms
  curTime += 100000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 100);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +1000.000 ms
  curTime += 1000000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 1100);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_OFF, 0, 0);

  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // position should not change, because RS is not calibrated
  EXPECT_EQ(*supla_rs_cfg[0].position, 0);
}

TEST_F(RollerShutterTestsF, MoveUpNotCalibrated) {
  int curTime = 123;
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  os_timer_func_t *rsTimerCb = lastTimerCb;

  EXPECT_EQ(rsCfg->last_time, 0);
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);

  curTime = 2000000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 2000000);
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);

  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 2000000);
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // Move RS up. Not calirbated. No stop. It should timeout after > 60s
  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_UP, 0, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +100.000 ms
  curTime += 100000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 2100000);
  EXPECT_EQ(rsCfg->up_time, 100);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +100.000 ms
  curTime += 100000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 2200000);
  EXPECT_EQ(rsCfg->up_time, 200);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +59100.000 ms
  curTime += 59100000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 61300000);
  EXPECT_EQ(rsCfg->up_time, 59300);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +9 min
  curTime += 9 * 60 * 1000000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 601300000);
  EXPECT_EQ(rsCfg->last_comm_time, 601300000);
  EXPECT_EQ(rsCfg->up_time, 599300);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +1s
  curTime += 1000000;
  rsTimerCb(rsCfg);
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 602300000);
  EXPECT_EQ(rsCfg->last_comm_time, 602300000);
  EXPECT_EQ(rsCfg->up_time, 600300);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // position should not change, because RS is not calibrated
  EXPECT_EQ(*supla_rs_cfg[0].position, 0);

  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 602300000);
  EXPECT_EQ(rsCfg->last_comm_time, 602300000);
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  curTime += 1000000;
  rsTimerCb(rsCfg);
  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_UP, 0, 0);

  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +100.000 ms
  curTime += 100000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->up_time, 100);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +1000.000 ms
  curTime += 1000000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->up_time, 1100);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_OFF, 0, 0);

  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // position should not change, because RS is not calibrated
  EXPECT_EQ(*supla_rs_cfg[0].position, 0);
}

TEST_F(RollerShutterTestsF, MoveUpAndDownNotCalibrated) {
  int curTime = 123;
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  os_timer_func_t *rsTimerCb = lastTimerCb;

  EXPECT_EQ(rsCfg->last_time, 0);
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);

  curTime = 2000000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 2000000);
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);

  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 2000000);
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // Move RS up. Not calirbated. No stop. It should timeout after > 60s
  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_UP, 0, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +100.000 ms
  curTime += 100000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->last_time, 2100000);
  EXPECT_EQ(rsCfg->up_time, 100);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +100.000 ms
  curTime += 100000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->up_time, 200);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +59100.000 ms
  curTime += 59100000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->up_time, 59300);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_DOWN, 0, 0);
  // change to opposite direction should be executed with delay on timer
  // Here we only check if it was set properly and trigger timer action manually
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->delayed_trigger.value, RS_RELAY_DOWN);

  curTime += 1000000;
  rsTimerCb(rsCfg);
  supla_esp_gpio_rs_set_relay(rsCfg, rsCfg->delayed_trigger.value, 0, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->delayed_trigger.value, RS_RELAY_DOWN); // TODO: fix

  rsTimerCb(rsCfg);

  // +1s
  curTime += 1000000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 1000);
  EXPECT_EQ(rsCfg->delayed_trigger.value, RS_RELAY_DOWN); // TODO: fix
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +1s
  curTime += 1000000;
  rsTimerCb(rsCfg);
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 2000);
  EXPECT_EQ(rsCfg->delayed_trigger.value, RS_RELAY_DOWN); // TODO: fix
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // position should not change, because RS is not calibrated
  EXPECT_EQ(*supla_rs_cfg[0].position, 0);

  curTime += 1000000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 3000);
  EXPECT_EQ(rsCfg->delayed_trigger.value, RS_RELAY_DOWN); // TODO: fix
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +1000.000 ms
  curTime += 1000000;
  rsTimerCb(rsCfg);

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 4000);
  EXPECT_EQ(rsCfg->delayed_trigger.value, RS_RELAY_DOWN); // TODO: fix
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_OFF, 0, 0);

  EXPECT_EQ(rsCfg->delayed_trigger.value, RS_RELAY_DOWN); // TODO: fix

  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // position should not change, because RS is not calibrated
  EXPECT_EQ(*supla_rs_cfg[0].position, 0);
}

TEST_F(RollerShutterTestsF, CalibrationWithRelayDown) {
  int curTime = 10000; // start at +10 ms
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  os_timer_func_t *rsTimerCb = lastTimerCb;

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +2000 ms 
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  *rsCfg->full_opening_time = 3000; // 3 s
  *rsCfg->full_closing_time = 3000; 

  // Move RS down. 
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_DOWN, 1, 1);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 3000);
  EXPECT_EQ(*rsCfg->full_closing_time, 3000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +2000 ms 
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  // after 2 s we are in the middle of calibration, so no change in position
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 2000);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 3000);
  EXPECT_EQ(*rsCfg->full_closing_time, 3000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +1300 ms 
  for (int i = 0; i < 130; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  // Calibration done
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 3300); // TODO fix
  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(*rsCfg->full_opening_time, 3000);
  EXPECT_EQ(*rsCfg->full_closing_time, 3000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(RollerShutterTestsF, CalibrationWithRelayUp) {
  int curTime = 10000; // start at +10 ms
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  os_timer_func_t *rsTimerCb = lastTimerCb;

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +2000 ms 
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  *rsCfg->full_opening_time = 2000; // 2 s
  *rsCfg->full_closing_time = 2000; 

  // Move RS down. 
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_UP, 1, 1);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 2000);
  EXPECT_EQ(*rsCfg->full_closing_time, 2000);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +1500 ms 
  for (int i = 0; i < 150; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  // after 2 s we are in the middle of calibration, so no change in position
  EXPECT_EQ(rsCfg->up_time, 1500);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 2000);
  EXPECT_EQ(*rsCfg->full_closing_time, 2000);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +700 ms 
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  // Calibration done
  EXPECT_EQ(rsCfg->up_time, 2200); // TODO fix
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 2000);
  EXPECT_EQ(*rsCfg->full_closing_time, 2000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(RollerShutterTestsF, CalibrationWithTargetPosition) {
  int curTime = 10000; // start at +10 ms
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  os_timer_func_t *rsTimerCb = lastTimerCb;

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +2000 ms 
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  *rsCfg->full_opening_time = 2000; // 2 s
  *rsCfg->full_closing_time = 2000; 

  // Move RS down. 
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  supla_esp_gpio_rs_add_task(0, 40);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 2000);
  EXPECT_EQ(*rsCfg->full_closing_time, 2000);
  // add task doesn't change relay state. At least one callback has to be called
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  rsTimerCb(rsCfg);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));


  // +1500 ms 
  for (int i = 0; i < 150; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  // after 2 s we are in the middle of calibration, so no change in position
  EXPECT_EQ(rsCfg->up_time, 1500);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 2000);
  EXPECT_EQ(*rsCfg->full_closing_time, 2000);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +700 ms 
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  // Calibration done
  EXPECT_EQ(rsCfg->up_time, 2200); // TODO fix
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 2000);
  EXPECT_EQ(*rsCfg->full_closing_time, 2000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // RS should now execute task to move to target position 40
  EXPECT_EQ(rsCfg->delayed_trigger.value, RS_RELAY_DOWN);

  curTime += 1000000;
  rsTimerCb(rsCfg);
  supla_esp_gpio_rs_set_relay(rsCfg, rsCfg->delayed_trigger.value, 0, 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->delayed_trigger.value, RS_RELAY_DOWN); // TODO: fix
  EXPECT_EQ(*rsCfg->position, 100);

  rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);

  // +100 ms 
  for (int i = 0; i < 10; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, (100+(5*100)));
  EXPECT_EQ(*rsCfg->full_opening_time, 2000);
  EXPECT_EQ(*rsCfg->full_closing_time, 2000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +700 ms 
  for (int i = 0; i < 70; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, (100+(40*100)));
  EXPECT_EQ(*rsCfg->full_opening_time, 2000);
  EXPECT_EQ(*rsCfg->full_closing_time, 2000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

}

TEST_F(RollerShutterTestsF, MoveDownWithUnfortunateTiming) {
  uint32_t curTime = 100;
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  os_timer_func_t *rsTimerCb = lastTimerCb;

  *rsCfg->full_opening_time = 2000; // 2 s
  *rsCfg->full_closing_time = 2000; 
  *rsCfg->position = 100; // actual position: (x - 100)/100 = 0%

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 2000);
  EXPECT_EQ(*rsCfg->full_closing_time, 2000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +2000 ms 
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  // set timer close to max of uint32_t (5 cycles of 10 ms lower)
  curTime = 4294967295 - 50*10000;
  rsTimerCb(rsCfg); // rs timer cb is called every 10 ms

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 2000);
  EXPECT_EQ(*rsCfg->full_closing_time, 2000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // Move RS down. 
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  supla_esp_gpio_rs_add_task(0, 100);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 2000);
  EXPECT_EQ(*rsCfg->full_closing_time, 2000);
  // add task doesn't change relay state. At least one callback has to be called
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  rsTimerCb(rsCfg);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +500 ms - everything should be fine
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, (100+(25*100)));
  EXPECT_EQ(*rsCfg->full_opening_time, 2000);
  EXPECT_EQ(*rsCfg->full_closing_time, 2000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +100 ms - here time goes crazy and flips to 0
  for (int i = 0; i < 10; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, (100+(30*100)));
  EXPECT_EQ(*rsCfg->full_opening_time, 2000);
  EXPECT_EQ(*rsCfg->full_closing_time, 2000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +500 ms - means total 1.1s of movement -> 55% 
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, (100+(55*100)));
  EXPECT_EQ(*rsCfg->full_opening_time, 2000);
  EXPECT_EQ(*rsCfg->full_closing_time, 2000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +900 ms - means total 2s of movement -> 100% 
  for (int i = 0; i < 900; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, (100+(100*100)));
  EXPECT_EQ(*rsCfg->full_opening_time, 2000);
  EXPECT_EQ(*rsCfg->full_closing_time, 2000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

}

TEST_F(RollerShutterTestsF, MoveDownWithUnfortunateTiming2) {
  uint32_t curTime = 100;
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  os_timer_func_t *rsTimerCb = lastTimerCb;

  *rsCfg->full_opening_time = 25000; // 25 s
  *rsCfg->full_closing_time = 25000; 
  *rsCfg->position = 100; // actual position: (x - 100)/100 = 0%

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +2000 ms 
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  // set timer close to max of uint32_t (5 cycles of 10 ms lower)
  curTime = 4294967295 - 50*10000;
  rsTimerCb(rsCfg); // rs timer cb is called every 10 ms

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // Move RS down. 
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  supla_esp_gpio_rs_add_task(0, 10);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  // add task doesn't change relay state. At least one callback has to be called
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  rsTimerCb(rsCfg);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +500 ms - everything should be fine
  for (int i = 0; i < 50; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, (100+(2*100)));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +300 ms - here time goes crazy and flips to 0
  for (int i = 0; i < 30; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  // for 25 s of opening/closing time, 800 ms means 3.2% position change
  EXPECT_EQ(*rsCfg->position, (100+(3.2*100)));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  // +2000 ms - it should complete the movement
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    rsTimerCb(rsCfg); // rs timer cb is called every 10 ms
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, (100+(10*100)));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

}
