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
void gpioCallback2() {
  // up
  supla_relay_cfg[0].gpio_id = UP_GPIO;
  supla_relay_cfg[0].channel = 0;
  supla_relay_cfg[0].channel_flags = SUPLA_CHANNEL_FLAG_RS_AUTO_CALIBRATION;

  // down
  supla_relay_cfg[1].gpio_id = DOWN_GPIO;
  supla_relay_cfg[1].channel = 0;
  supla_relay_cfg[1].channel_flags = SUPLA_CHANNEL_FLAG_RS_AUTO_CALIBRATION;

  supla_rs_cfg[0].up = &supla_relay_cfg[0];
  supla_rs_cfg[0].down = &supla_relay_cfg[1];
  supla_rs_cfg[0].delayed_trigger.value = 0;
}

class RollerShutterAutoCalF : public ::testing::Test {
public:
  TimeMock time;
  EagleSocStub eagleStub;

  void SetUp() override {
    memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
    memset(&supla_esp_state, 0, sizeof(SuplaEspState));
    cleanupTimers();
    supla_esp_gpio_init_time = 0;
    gpioInitCb = *gpioCallback2;
  }

  void TearDown() override {
    memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
    memset(&supla_esp_state, 0, sizeof(SuplaEspState));
    supla_esp_gpio_init_time = 0;
    cleanupTimers();

    gpioInitCb = nullptr;
  }
};

TEST_F(RollerShutterAutoCalF, RsNotCalibrated_TriggerAutoCalWithRelayDown) {
  int curTime = 10000; // start at +10 ms
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  supla_esp_gpio_init();

  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_ENABLED);

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  os_timer_func_t *rsTimerCb = lastTimerCb;
  ASSERT_NE(rsTimerCb, nullptr);

  // +2000 ms
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_ENABLED);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  // closing and opening time is coded in 0.1s units on 2x16 bit blocks of
  // DurationMS
  // In autocalibration, server sends ct/ot == 0
  reqValue.DurationMS = (0) | (0 << 16);
  reqValue.value[0] = 1; //RS_RELAY_DOWN

  // Move RS down.
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  // simulate request from server
  supla_esp_channel_set_value(&reqValue);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // Perform actual calibration procedure
  // Roller shutter ct == 1200
  // Roller shutter ot == 1100
  // +7000 ms 
  // time to move up (ot), + ct, +ot, +move down(ct*1.1) + 3s(time
  // between change direction of RS) = 3620-3650 ms
  for (int i = 0; i < 800; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1100);
  EXPECT_EQ(*rsCfg->full_closing_time, 1200);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->auto_calibration_step, 0);

}

TEST_F(RollerShutterAutoCalF, AutoCalibrationDisabled) {
  int curTime = 10000; // start at +10 ms
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  // supla_esp_cfg parameters are loaded from flash before supla_esp_gpio_init.
  // When those time are non 0, we don't modify RsAutoCalibrationFlag 
  supla_esp_cfg.Time1[0] = 1000;
  supla_esp_cfg.Time2[0] = 1000;
  supla_esp_gpio_init();

  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_DISABLED);

}

TEST_F(RollerShutterAutoCalF, AutoCalibrationEnabled) {
  int curTime = 10000; // start at +10 ms
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  // supla_esp_cfg parameters are loaded from flash before supla_esp_gpio_init.
  // When those time are non 0, we don't modify RsAutoCalibrationFlag 
  supla_esp_cfg.Time1[0] = 1000;
  supla_esp_cfg.Time2[0] = 1000;
  supla_esp_cfg.RsAutoCalibrationFlag[0] = RS_AUTOCALIBRATION_ENABLED;
  supla_esp_gpio_init();

  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_ENABLED);

}

TEST_F(RollerShutterAutoCalF, RsNotCalibrated_DisableAutoCal) {
  int curTime = 10000; // start at +10 ms
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  supla_esp_gpio_init();

  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_ENABLED);

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  // +2000 ms
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_ENABLED);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  // closing and opening time is coded in 0.1s units on 2x16 bit blocks of
  // DurationMS
  // In autocalibration, server sends ct/ot == 0
  reqValue.DurationMS = (100) | (0 << 16);
  reqValue.value[0] = 1; //RS_RELAY_DOWN

  // Move RS down.
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  // simulate request from server
  supla_esp_channel_set_value(&reqValue);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 10000);
  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_DISABLED);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(RollerShutterAutoCalF, RsManuallyCalibrated_EnableAutoCal) {
  int curTime = 10000; // start at +10 ms
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  supla_esp_cfg.Time1[0] = 1000;
  supla_esp_cfg.Time2[0] = 1000;
  supla_esp_cfg.RsAutoCalibrationFlag[0] = RS_AUTOCALIBRATION_DISABLED;
  supla_esp_state.rs_position[0] = 200;
  supla_esp_gpio_init();

  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_DISABLED);

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  // +2000 ms
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 200);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_DISABLED);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  // closing and opening time is coded in 0.1s units on 2x16 bit blocks of
  // DurationMS
  // In autocalibration, server sends ct/ot == 0
  reqValue.DurationMS = (0) | (0 << 16);
  reqValue.value[0] = 1; //RS_RELAY_DOWN

  // Move RS down.
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  // simulate request from server
  supla_esp_channel_set_value(&reqValue);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_ENABLED);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(RollerShutterAutoCalF, RsManuallyCalibrated_ApplyAnotherManualCalibration) {
  int curTime = 10000; // start at +10 ms
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  supla_esp_cfg.Time1[0] = 1000;
  supla_esp_cfg.Time2[0] = 1000;
  supla_esp_cfg.RsAutoCalibrationFlag[0] = RS_AUTOCALIBRATION_DISABLED;
  supla_esp_state.rs_position[0] = 200;
  supla_esp_gpio_init();

  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_DISABLED);

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  // +2000 ms
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 200);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_DISABLED);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  // closing and opening time is coded in 0.1s units on 2x16 bit blocks of
  // DurationMS
  // In autocalibration, server sends ct/ot == 0
  reqValue.DurationMS = (10) | (0 << 16);
  reqValue.value[0] = 1; //RS_RELAY_DOWN

  // Move RS down.
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  // simulate request from server
  supla_esp_channel_set_value(&reqValue);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_DISABLED);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(RollerShutterAutoCalF, RsAutoCalibrated_DisableAutoCal) {
  int curTime = 10000; // start at +10 ms
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  supla_esp_cfg.Time1[0] = 1000;
  supla_esp_cfg.Time2[0] = 1000;
  supla_esp_cfg.RsAutoCalibrationFlag[0] = RS_AUTOCALIBRATION_ENABLED;
  supla_esp_state.rs_position[0] = 200;
  supla_esp_gpio_init();

  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_ENABLED);

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  // +2000 ms
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 200);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_ENABLED);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  // closing and opening time is coded in 0.1s units on 2x16 bit blocks of
  // DurationMS
  // In autocalibration, server sends ct/ot == 0
  reqValue.DurationMS = (0) | (10 << 16);
  reqValue.value[0] = 1; //RS_RELAY_DOWN

  // Move RS down.
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  // simulate request from server
  supla_esp_channel_set_value(&reqValue);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_DISABLED);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(RollerShutterAutoCalF, RsAutoCalibrated_SetNewPosition) {
  int curTime = 10000; // start at +10 ms
  EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

  supla_esp_cfg.Time1[0] = 1000;
  supla_esp_cfg.Time2[0] = 1000;
  supla_esp_cfg.RsAutoCalibrationFlag[0] = RS_AUTOCALIBRATION_ENABLED;
  supla_esp_state.rs_position[0] = 200;
  supla_esp_gpio_init();

  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_ENABLED);

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  // +2000 ms
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 200);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_ENABLED);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  // closing and opening time is coded in 0.1s units on 2x16 bit blocks of
  // DurationMS
  // In autocalibration, server sends ct/ot == 0
  reqValue.DurationMS = (0) | (0 << 16);
  reqValue.value[0] = 30; // target posiotion = 20

  // Move RS down.
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);
  // simulate request from server
  supla_esp_channel_set_value(&reqValue);
  EXPECT_EQ(rsCfg->delayed_trigger.value, 0);

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 200); // postion 10
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_EQ(supla_esp_cfg.RsAutoCalibrationFlag[0], RS_AUTOCALIBRATION_ENABLED);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
}
