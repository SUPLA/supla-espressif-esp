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
#include <gtest/gtest.h>
#include <time_mock.h>

extern "C" {
#include "board_stub.h"
#include <supla_esp_cfg.h>
#include <supla_esp_gpio.h>
}

#define RS_DIRECTION_NONE 0
#define RS_DIRECTION_UP 2
#define RS_DIRECTION_DOWN 1

// method will be called by supla_esp_gpio_init method in order to initialize
// gpio input/outputs board configuration (supla_esb_board_gpio_init)
void gpioCallback1() {
  // up
  supla_relay_cfg[0].gpio_id = 1;
  supla_relay_cfg[0].channel = 0;

  // down
  supla_relay_cfg[1].gpio_id = 2;
  supla_relay_cfg[1].channel = 0;

  supla_rs_cfg[0].up = &supla_relay_cfg[0];
  supla_rs_cfg[0].down = &supla_relay_cfg[1];
  *supla_rs_cfg[0].position = 0;
}

class RollerShutterTestsF : public ::testing::Test {
public:
  TimeMock time;

  void SetUp() override { gpioInitCb = *gpioCallback1; }

  void TearDown() override { 
    *supla_rs_cfg[0].position = 0;
    gpioInitCb = nullptr; 
  }
};

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;

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

