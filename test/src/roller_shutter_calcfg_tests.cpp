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
void gpioCallback3() {
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

class RollerShutterCommonFixture : public ::testing::Test {
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
};


class RollerShutterCalCfgManualCalF : public RollerShutterCommonFixture {
public:

  void SetUp() override {
    RollerShutterCommonFixture::SetUp();
    gpioInitCb = *gpioCallback3;
  }

  void TearDown() override {
    RollerShutterCommonFixture::TearDown();
  }
};

// method will be called by supla_esp_gpio_init method in order to initialize
// gpio input/outputs board configuration (supla_esb_board_gpio_init)
void gpioCallback4() {
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

class RollerShutterCalCfgAutoCalF : public RollerShutterCommonFixture {
public:
  void SetUp() override {
    RollerShutterCommonFixture::SetUp();
    gpioInitCb = *gpioCallback4;
  }

  void TearDown() override {
    RollerShutterCommonFixture::TearDown();
  }
};

// method will be called by supla_esp_gpio_init method in order to initialize
// gpio input/outputs board configuration (supla_esb_board_gpio_init)
void gpioCallback5() {
  // up
  supla_relay_cfg[0].gpio_id = UP_GPIO;
  supla_relay_cfg[0].channel = 0;

  // down
  supla_relay_cfg[1].gpio_id = DOWN_GPIO;
  supla_relay_cfg[1].channel = 0;

  supla_rs_cfg[0].up = &supla_relay_cfg[0];
  supla_rs_cfg[0].down = &supla_relay_cfg[1];
  supla_rs_cfg[0].delayed_trigger.value = 0;
}

class RollerShutterCalCfgNotSupportedF : public RollerShutterCommonFixture {
public:
  void SetUp() override {
    RollerShutterCommonFixture::SetUp();
    gpioInitCb = *gpioCallback5;
  }

  void TearDown() override {
    RollerShutterCommonFixture::TearDown();
  }
};

TEST_F(RollerShutterCalCfgNotSupportedF, CalCfgRecalibrateNotSupported) {

  supla_esp_cfg.Time1[0] = 1000;
  supla_esp_cfg.Time2[0] = 1000;
  supla_esp_cfg.AutoCalCloseTime[0] = 0;
  supla_esp_cfg.AutoCalOpenTime[0] = 0;
  supla_esp_state.rs_position[0] = 200;
  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 200);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 1000);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->autoCal_step, 0);

  TSD_DeviceCalCfgRequest request = {};
  request.ChannelNumber = 0;
  request.Command = SUPLA_CALCFG_CMD_RECALIBRATE;
  request.SuperUserAuthorized = 1;
  request.DataType = SUPLA_CALCFG_DATATYPE_RS_SETTINGS;
  request.DataSize = sizeof(TCalCfg_RollerShutterSettings);

  TCalCfg_RollerShutterSettings rsSettings = {};
  rsSettings.FullClosingTimeMS = 1000;
  rsSettings.FullOpeningTimeMS = 1000;
  memcpy(request.Data, &rsSettings, sizeof(TCalCfg_RollerShutterSettings));

  supla_esp_calcfg_request(&request);

  // nothing should change, recalibrate not supported
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 200);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 1000);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->autoCal_step, 0);
}

TEST_F(RollerShutterCalCfgAutoCalF, IncorrectChannel) {

  supla_esp_cfg.Time1[0] = 1000;
  supla_esp_cfg.Time2[0] = 1000;
  supla_esp_cfg.AutoCalCloseTime[0] = 0;
  supla_esp_cfg.AutoCalOpenTime[0] = 0;
  supla_esp_state.rs_position[0] = 200;
  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 200);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 1000);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->autoCal_step, 0);

  TSD_DeviceCalCfgRequest request = {};
  request.ChannelNumber = 1;
  request.Command = SUPLA_CALCFG_CMD_RECALIBRATE;
  request.SuperUserAuthorized = 1;

  supla_esp_calcfg_request(&request);

  // nothing should change, recalibrate not supported
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 200);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 1000);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->autoCal_step, 0);
}

TEST_F(RollerShutterCalCfgAutoCalF, NotAuthorized) {

  supla_esp_cfg.Time1[0] = 1000;
  supla_esp_cfg.Time2[0] = 1000;
  supla_esp_cfg.AutoCalCloseTime[0] = 0;
  supla_esp_cfg.AutoCalOpenTime[0] = 0;
  supla_esp_state.rs_position[0] = 200;
  supla_esp_gpio_init();

  supla_roller_shutter_cfg_t *rsCfg = supla_esp_gpio_get_rs__cfg(1);
  ASSERT_NE(rsCfg, nullptr);

  // +1000 ms
  for (int i = 0; i < 100; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 200);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 1000);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->autoCal_step, 0);

  TSD_DeviceCalCfgRequest request = {};
  request.ChannelNumber = 0;
  request.Command = SUPLA_CALCFG_CMD_RECALIBRATE;
  request.SuperUserAuthorized = 0;

  supla_esp_calcfg_request(&request);

  // nothing should change, recalibrate not supported
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 200);
  EXPECT_EQ(*rsCfg->full_opening_time, 1000);
  EXPECT_EQ(*rsCfg->full_closing_time, 1000);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 1000);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->autoCal_step, 0);
}

TEST_F(RollerShutterCalCfgAutoCalF, RsAutoCalibrated_CalCfgRecalibrate) {

  supla_esp_cfg.Time1[0] = 0;
  supla_esp_cfg.Time2[0] = 0;
  supla_esp_cfg.AutoCalCloseTime[0] = 1000;
  supla_esp_cfg.AutoCalOpenTime[0] = 1000;
  supla_esp_state.rs_position[0] = 200;
  supla_esp_gpio_init();

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
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_DeviceCalCfgRequest request = {};
  request.ChannelNumber = 0;
  request.Command = SUPLA_CALCFG_CMD_RECALIBRATE;
  request.SuperUserAuthorized = 1;
  request.DataType = SUPLA_CALCFG_DATATYPE_RS_SETTINGS;
  request.DataSize = sizeof(TCalCfg_RollerShutterSettings);

  TCalCfg_RollerShutterSettings rsSettings = {};
  rsSettings.FullClosingTimeMS = 0;
  rsSettings.FullOpeningTimeMS = 0;
  memcpy(request.Data, &rsSettings, sizeof(TCalCfg_RollerShutterSettings));

  supla_esp_calcfg_request(&request);

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->autoCal_step, 0);
 
  // +2000 ms
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_GT(rsCfg->autoCal_step, 0);
 
  // +8000 ms
  for (int i = 0; i < 800; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 1100);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 1200);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->autoCal_step, 0);
}

TEST_F(RollerShutterCalCfgAutoCalF, RsNotCalibrated_CalCfgRecalibrate) {

  supla_esp_cfg.Time1[0] = 0;
  supla_esp_cfg.Time2[0] = 0;
  supla_esp_cfg.AutoCalCloseTime[0] = 0;
  supla_esp_cfg.AutoCalOpenTime[0] = 0;
  supla_esp_state.rs_position[0] = 0;
  supla_esp_gpio_init();

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
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_DeviceCalCfgRequest request = {};
  request.ChannelNumber = 0;
  request.Command = SUPLA_CALCFG_CMD_RECALIBRATE;
  request.SuperUserAuthorized = 1;
  request.DataType = SUPLA_CALCFG_DATATYPE_RS_SETTINGS;
  request.DataSize = sizeof(TCalCfg_RollerShutterSettings);

  TCalCfg_RollerShutterSettings rsSettings = {};
  rsSettings.FullClosingTimeMS = 0;
  rsSettings.FullOpeningTimeMS = 0;
  memcpy(request.Data, &rsSettings, sizeof(TCalCfg_RollerShutterSettings));

  supla_esp_calcfg_request(&request);

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->autoCal_step, 0);
 
  // +2000 ms
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_GT(rsCfg->autoCal_step, 0);
 
  // +8000 ms
  for (int i = 0; i < 800; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 1100);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 1200);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->autoCal_step, 0);
}

TEST_F(RollerShutterCalCfgAutoCalF, RsDuringCalibration_CalCfgRecalibrate) {

  supla_esp_cfg.Time1[0] = 0;
  supla_esp_cfg.Time2[0] = 0;
  supla_esp_cfg.AutoCalCloseTime[0] = 0;
  supla_esp_cfg.AutoCalOpenTime[0] = 0;
  supla_esp_state.rs_position[0] = 0;
  supla_esp_gpio_init();

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
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  
  // trigger autocalibration
  supla_esp_gpio_rs_add_task(0, 10);

  // +2000 ms
  for (int i = 0; i < 250; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_GT(rsCfg->autoCal_step, 0);

  // request from client
  TSD_DeviceCalCfgRequest request = {};
  request.ChannelNumber = 0;
  request.Command = SUPLA_CALCFG_CMD_RECALIBRATE;
  request.SuperUserAuthorized = 1;
  request.DataType = SUPLA_CALCFG_DATATYPE_RS_SETTINGS;
  request.DataSize = sizeof(TCalCfg_RollerShutterSettings);

  TCalCfg_RollerShutterSettings rsSettings = {};
  rsSettings.FullClosingTimeMS = 0;
  rsSettings.FullOpeningTimeMS = 0;
  memcpy(request.Data, &rsSettings, sizeof(TCalCfg_RollerShutterSettings));


  supla_esp_calcfg_request(&request);

  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_EQ(rsCfg->autoCal_step, 0);
 
  // +2000 ms
  for (int i = 0; i < 200; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_GT(rsCfg->autoCal_step, 0);
 
  // +8000 ms
  for (int i = 0; i < 800; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 1100);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 1200);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->autoCal_step, 0);
}

TEST_F(RollerShutterCalCfgAutoCalF, RsManuallyCalibrated_CalCfgRecalibrate) {

  supla_esp_cfg.Time1[0] = 1300;
  supla_esp_cfg.Time2[0] = 1400;
  supla_esp_cfg.AutoCalCloseTime[0] = 0;
  supla_esp_cfg.AutoCalOpenTime[0] = 0;
  supla_esp_state.rs_position[0] = 400;
  supla_esp_gpio_init();

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
  EXPECT_EQ(*rsCfg->position, 400);
  EXPECT_EQ(*rsCfg->full_opening_time, 1300);
  EXPECT_EQ(*rsCfg->full_closing_time, 1400);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  
  // request from client
  TSD_DeviceCalCfgRequest request = {};
  request.ChannelNumber = 0;
  request.Command = SUPLA_CALCFG_CMD_RECALIBRATE;
  request.SuperUserAuthorized = 1;
  request.DataType = SUPLA_CALCFG_DATATYPE_RS_SETTINGS;
  request.DataSize = sizeof(TCalCfg_RollerShutterSettings);

  TCalCfg_RollerShutterSettings rsSettings = {};
  rsSettings.FullClosingTimeMS = 1400;
  rsSettings.FullOpeningTimeMS = 1300;
  memcpy(request.Data, &rsSettings, sizeof(TCalCfg_RollerShutterSettings));


  supla_esp_calcfg_request(&request);

  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 1300);
  EXPECT_EQ(*rsCfg->full_closing_time, 1400);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_EQ(rsCfg->autoCal_step, 0);
 
  // +8000 ms
  for (int i = 0; i < 800; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1300);
  EXPECT_EQ(*rsCfg->full_closing_time, 1400);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->autoCal_step, 0);
}

TEST_F(RollerShutterCalCfgManualCalF, RsCalibrated_CalCfgRecalibrate) {

  supla_esp_cfg.Time1[0] = 1300;
  supla_esp_cfg.Time2[0] = 1400;
  supla_esp_cfg.AutoCalCloseTime[0] = 0;
  supla_esp_cfg.AutoCalOpenTime[0] = 0;
  supla_esp_state.rs_position[0] = 400;
  supla_esp_gpio_init();

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
  EXPECT_EQ(*rsCfg->position, 400);
  EXPECT_EQ(*rsCfg->full_opening_time, 1300);
  EXPECT_EQ(*rsCfg->full_closing_time, 1400);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  
  // request from client
  TSD_DeviceCalCfgRequest request = {};
  request.ChannelNumber = 0;
  request.Command = SUPLA_CALCFG_CMD_RECALIBRATE;
  request.SuperUserAuthorized = 1;
  request.DataType = SUPLA_CALCFG_DATATYPE_RS_SETTINGS;
  request.DataSize = sizeof(TCalCfg_RollerShutterSettings);

  TCalCfg_RollerShutterSettings rsSettings = {};
  rsSettings.FullClosingTimeMS = 1400;
  rsSettings.FullOpeningTimeMS = 1300;
  memcpy(request.Data, &rsSettings, sizeof(TCalCfg_RollerShutterSettings));

  supla_esp_calcfg_request(&request);

  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 1300);
  EXPECT_EQ(*rsCfg->full_closing_time, 1400);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_EQ(rsCfg->autoCal_step, 0);
 
  // +8000 ms
  for (int i = 0; i < 800; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1300);
  EXPECT_EQ(*rsCfg->full_closing_time, 1400);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->autoCal_step, 0);
}

TEST_F(RollerShutterCalCfgManualCalF, RsNotCalibrated_CalCfgRecalibrate) {

  supla_esp_cfg.Time1[0] = 1300;
  supla_esp_cfg.Time2[0] = 1400;
  supla_esp_cfg.AutoCalCloseTime[0] = 0;
  supla_esp_cfg.AutoCalOpenTime[0] = 0;
  supla_esp_state.rs_position[0] = 0;
  supla_esp_gpio_init();

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
  EXPECT_EQ(*rsCfg->full_opening_time, 1300);
  EXPECT_EQ(*rsCfg->full_closing_time, 1400);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  
  // request from client
  TSD_DeviceCalCfgRequest request = {};
  request.ChannelNumber = 0;
  request.Command = SUPLA_CALCFG_CMD_RECALIBRATE;
  request.SuperUserAuthorized = 1;
  request.DataType = SUPLA_CALCFG_DATATYPE_RS_SETTINGS;
  request.DataSize = sizeof(TCalCfg_RollerShutterSettings);

  TCalCfg_RollerShutterSettings rsSettings = {};
  rsSettings.FullClosingTimeMS = 1400;
  rsSettings.FullOpeningTimeMS = 1300;
  memcpy(request.Data, &rsSettings, sizeof(TCalCfg_RollerShutterSettings));

  supla_esp_calcfg_request(&request);

  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 1300);
  EXPECT_EQ(*rsCfg->full_closing_time, 1400);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_EQ(rsCfg->autoCal_step, 0);
 
  // +8000 ms
  for (int i = 0; i < 800; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1300);
  EXPECT_EQ(*rsCfg->full_closing_time, 1400);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->autoCal_step, 0);
}

TEST_F(RollerShutterCalCfgManualCalF, RsDuringCalibration_CalCfgRecalibrate) {

  supla_esp_cfg.Time1[0] = 1300;
  supla_esp_cfg.Time2[0] = 1400;
  supla_esp_cfg.AutoCalCloseTime[0] = 0;
  supla_esp_cfg.AutoCalOpenTime[0] = 0;
  supla_esp_state.rs_position[0] = 0;
  supla_esp_gpio_init();

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
  EXPECT_EQ(*rsCfg->full_opening_time, 1300);
  EXPECT_EQ(*rsCfg->full_closing_time, 1400);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  
  // trigger autocalibration
  supla_esp_gpio_rs_add_task(0, 10);

  for (int i = 0; i < 80; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_GT(rsCfg->up_time, 0);
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));

  // request from client
  TSD_DeviceCalCfgRequest request = {};
  request.ChannelNumber = 0;
  request.Command = SUPLA_CALCFG_CMD_RECALIBRATE;
  request.SuperUserAuthorized = 1;
  request.DataType = SUPLA_CALCFG_DATATYPE_RS_SETTINGS;
  request.DataSize = sizeof(TCalCfg_RollerShutterSettings);

  TCalCfg_RollerShutterSettings rsSettings = {};
  rsSettings.FullClosingTimeMS = 1400;
  rsSettings.FullOpeningTimeMS = 1300;
  memcpy(request.Data, &rsSettings, sizeof(TCalCfg_RollerShutterSettings));

  supla_esp_calcfg_request(&request);

  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 1300);
  EXPECT_EQ(*rsCfg->full_closing_time, 1400);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_EQ(rsCfg->autoCal_step, 0);
 
  // +8000 ms
  for (int i = 0; i < 800; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->full_opening_time, 1300);
  EXPECT_EQ(*rsCfg->full_closing_time, 1400);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->autoCal_step, 0);
}

TEST_F(RollerShutterCalCfgAutoCalF, RsManuallyCalibrated_AutoCalCfgRecalibrate) {

  supla_esp_cfg.Time1[0] = 1300;
  supla_esp_cfg.Time2[0] = 1400;
  supla_esp_cfg.AutoCalCloseTime[0] = 0;
  supla_esp_cfg.AutoCalOpenTime[0] = 0;
  supla_esp_state.rs_position[0] = 400;
  supla_esp_gpio_init();

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
  EXPECT_EQ(*rsCfg->position, 400);
  EXPECT_EQ(*rsCfg->full_opening_time, 1300);
  EXPECT_EQ(*rsCfg->full_closing_time, 1400);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  
  // request from client
  TSD_DeviceCalCfgRequest request = {};
  request.ChannelNumber = 0;
  request.Command = SUPLA_CALCFG_CMD_RECALIBRATE;
  request.SuperUserAuthorized = 1;
  request.DataType = SUPLA_CALCFG_DATATYPE_RS_SETTINGS;
  request.DataSize = sizeof(TCalCfg_RollerShutterSettings);

  TCalCfg_RollerShutterSettings rsSettings = {};
  rsSettings.FullClosingTimeMS = 0;
  rsSettings.FullOpeningTimeMS = 0;
  memcpy(request.Data, &rsSettings, sizeof(TCalCfg_RollerShutterSettings));


  supla_esp_calcfg_request(&request);

  EXPECT_EQ(*rsCfg->position, 0);
  EXPECT_EQ(*rsCfg->auto_opening_time, 0);
  EXPECT_EQ(*rsCfg->auto_closing_time, 0);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 0);
  EXPECT_EQ(rsCfg->autoCal_step, 0);
 
  // +8000 ms
  for (int i = 0; i < 800; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->auto_opening_time, 1100);
  EXPECT_EQ(*rsCfg->auto_closing_time, 1200);
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 1100);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 1200);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
  EXPECT_EQ(rsCfg->autoCal_step, 0);
}

