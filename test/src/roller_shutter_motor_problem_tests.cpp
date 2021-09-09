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

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::SaveArg;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::Invoke;
using ::testing::Pointee;
using ::testing::ElementsAreArray;

// method will be called by supla_esp_gpio_init method in order to initialize
// gpio input/outputs board configuration (supla_esb_board_gpio_init)
void gpioCallbackMotor() {
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

TSD_SuplaRegisterDeviceResult regResultMotor;

char custom_srpc_getdata_motor_problem(void *_srpc, TsrpcReceivedData *rd,
    unsigned _supla_int_t rr_id) {
  rd->call_type = SUPLA_SD_CALL_REGISTER_DEVICE_RESULT;
  rd->data.sd_register_device_result = &regResultMotor;
  return 1;
}

class RollerShutterMotorProblem : public ::testing::Test {
public:
  SrpcMock srpc;
  TimeMock time;
  EagleSocStub eagleStub;
  int curTime;

  void SetUp() override {
    startupTimeDelay = 0;
    upTime = 1100;
    downTime = 1200;
    memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
    memset(&supla_esp_state, 0, sizeof(SuplaEspState));
    memset(&supla_relay_cfg, 0, sizeof(supla_relay_cfg));
    cleanupTimers();
    supla_esp_gpio_init_time = 0;
    gpioInitCb = *gpioCallbackMotor;
    curTime = 10000; // start at +10 ms
    EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

    EXPECT_CALL(srpc, srpc_params_init(_));
    EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return((void *)1));
    EXPECT_CALL(srpc, srpc_set_proto_version(_, 16));

    regResultMotor.result_code = SUPLA_RESULTCODE_TRUE;

    EXPECT_CALL(srpc, srpc_getdata(_, _, _))
      .WillOnce(DoAll(Invoke(custom_srpc_getdata_motor_problem), Return(1)));
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
    supla_esp_devconn_release();
    startupTimeDelay = 0;
    upTime = 1100;
    downTime = 1200;
    memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
    memset(&supla_esp_state, 0, sizeof(SuplaEspState));
    supla_esp_gpio_init_time = 0;
    cleanupTimers();
    supla_esp_gpio_clear_vars();

    gpioInitCb = nullptr;
  }
};


TEST_F(RollerShutterMotorProblem, MotorProblemMoveDown) {

  // Set how long [ms] "is_in_move" method will return true
  upTime = 0;
  downTime = 0;

  EXPECT_CALL(srpc, 
      valueChanged(_, 0, ElementsAreArray({50, 0, 0, 0, 0, 0, 0, 0})))
    .Times(2)
    .WillRepeatedly(Return(0));

  EXPECT_CALL(srpc, 
      valueChanged(_, 0, ElementsAreArray({54, 0, 0, 
          RS_VALUE_FLAG_MOTOR_PROBLEM, 0, 0, 0, 0})))
    .WillRepeatedly(Return(0));

  EXPECT_CALL(srpc, 
      valueChanged(_, 0, ElementsAreArray({100, 0, 0, 
          RS_VALUE_FLAG_MOTOR_PROBLEM, 0, 0, 0, 0})))
    .WillOnce(Return(0));

  supla_esp_cfg.Time1[0] = 0;
  supla_esp_cfg.Time2[0] = 0;
  supla_esp_cfg.AutoCalCloseTime[0] = 1000;
  supla_esp_cfg.AutoCalOpenTime[0] = 1000;
  supla_esp_state.rs_position[0] = 100 + (50*100);
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
  EXPECT_EQ(*rsCfg->position, 100 + (50*100));
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  // closing and opening time is coded in 0.1s units on 2x16 bit blocks of
  // DurationMS
  // In autocalibration, server sends ct/ot == 0
  reqValue.DurationMS = (0) | (0 << 16);
  reqValue.value[0] = 1; // move down

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  for (int i = 0; i < 800; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(RollerShutterMotorProblem, MotorProblemMoveUp) {

  // Set how long [ms] "is_in_move" method will return true
  upTime = 0;
  downTime = 0;

  EXPECT_CALL(srpc, 
      valueChanged(_, 0, ElementsAreArray({50, 0, 0, 0, 0, 0, 0, 0})))
    .Times(2)
    .WillRepeatedly(Return(0));

  EXPECT_CALL(srpc, 
      valueChanged(_, 0, ElementsAreArray({46, 0, 0, 
          RS_VALUE_FLAG_MOTOR_PROBLEM, 0, 0, 0, 0})))
    .WillRepeatedly(Return(0));

  EXPECT_CALL(srpc,
      valueChanged(_, 0,
        ElementsAreArray({0, 0, 0, RS_VALUE_FLAG_MOTOR_PROBLEM,
          0, 0, 0, 0})))
    .WillOnce(Return(0));

  supla_esp_cfg.Time1[0] = 0;
  supla_esp_cfg.Time2[0] = 0;
  supla_esp_cfg.AutoCalCloseTime[0] = 1000;
  supla_esp_cfg.AutoCalOpenTime[0] = 1000;
  supla_esp_state.rs_position[0] = 100 + (50*100);
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
  EXPECT_EQ(*rsCfg->position, 100 + (50*100));
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  // closing and opening time is coded in 0.1s units on 2x16 bit blocks of
  // DurationMS
  // In autocalibration, server sends ct/ot == 0
  reqValue.DurationMS = (0) | (0 << 16);
  reqValue.value[0] = 2; // move up

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  for (int i = 0; i < 800; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(RollerShutterMotorProblem, MotorProblemMoveUpCloseToFullyOpen) {

  // Set how long [ms] "is_in_move" method will return true
  upTime = 0;
  downTime = 0;

  EXPECT_CALL(srpc, 
      valueChanged(_, 0, ElementsAreArray({5, 0, 0, 0, 0, 0, 0, 0})))
    .Times(2)
    .WillRepeatedly(Return(0));

  EXPECT_CALL(srpc, 
      valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
    .WillOnce(Return(0));

  EXPECT_CALL(srpc, 
      valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
    .WillOnce(Return(0));

  supla_esp_cfg.Time1[0] = 0;
  supla_esp_cfg.Time2[0] = 0;
  supla_esp_cfg.AutoCalCloseTime[0] = 1000;
  supla_esp_cfg.AutoCalOpenTime[0] = 1000;
  supla_esp_state.rs_position[0] = 100 + (5*100);
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
  EXPECT_EQ(*rsCfg->position, 100 + (5*100));
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  // closing and opening time is coded in 0.1s units on 2x16 bit blocks of
  // DurationMS
  // In autocalibration, server sends ct/ot == 0
  reqValue.DurationMS = (0) | (0 << 16);
  reqValue.value[0] = 2; // move up

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  for (int i = 0; i < 800; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(RollerShutterMotorProblem, MotorProblemMoveDownCloseToFullyClosed) {

  // Set how long [ms] "is_in_move" method will return true
  upTime = 0;
  downTime = 0;

  EXPECT_CALL(srpc, 
      valueChanged(_, 0, ElementsAreArray({95, 0, 0, 0, 0, 0, 0, 0})))
    .Times(2)
    .WillRepeatedly(Return(0));

  EXPECT_CALL(srpc, 
      valueChanged(_, 0, ElementsAreArray({99, 0, 0, 0, 0, 0, 0, 0})))
    .WillOnce(Return(0));

  EXPECT_CALL(srpc, 
      valueChanged(_, 0, ElementsAreArray({100, 0, 0, 0, 0, 0, 0, 0})))
    .WillOnce(Return(0));

  supla_esp_cfg.Time1[0] = 0;
  supla_esp_cfg.Time2[0] = 0;
  supla_esp_cfg.AutoCalCloseTime[0] = 1000;
  supla_esp_cfg.AutoCalOpenTime[0] = 1000;
  supla_esp_state.rs_position[0] = 100 + (95*100);
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
  EXPECT_EQ(*rsCfg->position, 100 + (95*100));
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  // closing and opening time is coded in 0.1s units on 2x16 bit blocks of
  // DurationMS
  // In autocalibration, server sends ct/ot == 0
  reqValue.DurationMS = (0) | (0 << 16);
  reqValue.value[0] = 1; // move down

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  for (int i = 0; i < 800; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(RollerShutterMotorProblem, MotorProblemByTask) {

  // Set how long [ms] "is_in_move" method will return true
  upTime = 0;
  downTime = 0;

  EXPECT_CALL(srpc, 
      valueChanged(_, 0, ElementsAreArray({50, 0, 0, 0, 0, 0, 0, 0})))
    .Times(2)
    .WillRepeatedly(Return(0));

  EXPECT_CALL(srpc, 
      valueChanged(_, 0, ElementsAreArray({53, 0, 0, 
          RS_VALUE_FLAG_MOTOR_PROBLEM, 0, 0, 0, 0})))
    .WillOnce(Return(0));

  EXPECT_CALL(srpc, valueChanged(_, 0,
        ElementsAreArray({90, 0, 0,
          RS_VALUE_FLAG_MOTOR_PROBLEM, 0,
          0, 0, 0})))
    .WillOnce(Return(0));

  supla_esp_cfg.Time1[0] = 0;
  supla_esp_cfg.Time2[0] = 0;
  supla_esp_cfg.AutoCalCloseTime[0] = 1000;
  supla_esp_cfg.AutoCalOpenTime[0] = 1000;
  supla_esp_state.rs_position[0] = 100 + (50*100);
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
  EXPECT_EQ(*rsCfg->position, 100 + (50*100));
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  // closing and opening time is coded in 0.1s units on 2x16 bit blocks of
  // DurationMS
  // In autocalibration, server sends ct/ot == 0
  reqValue.DurationMS = (0) | (0 << 16);
  reqValue.value[0] = 100; // postion 90

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  for (int i = 0; i < 800; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 100 + (90*100));
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(RollerShutterMotorProblem, MotorProblemWithLongStartupTime) {
  // Set how long [ms] "is_in_move" method will return true
  // auto cal times < 500 ms are considered as errors
  startupTimeDelay = 1000; // it takes 1s to start RS movement power consumption
  upTime = 0;
  downTime = 0;


  // Set how long [ms] "is_in_move" method will return true
  upTime = 0;
  downTime = 0;

  EXPECT_CALL(srpc, 
      valueChanged(_, 0, ElementsAreArray({50, 0, 0, 0, 0, 0, 0, 0})))
    .Times(2)
    .WillRepeatedly(Return(0));

  EXPECT_CALL(srpc, valueChanged(_, 0,
        ElementsAreArray({53, 0, 0,
          RS_VALUE_FLAG_MOTOR_PROBLEM, 0,
          0, 0, 0})))
    .WillOnce(Return(0));

  EXPECT_CALL(srpc, valueChanged(_, 0,
        ElementsAreArray({90, 0, 0,
          RS_VALUE_FLAG_MOTOR_PROBLEM, 0,
          0, 0, 0})))
    .WillOnce(Return(0));

  supla_esp_cfg.Time1[0] = 0;
  supla_esp_cfg.Time2[0] = 0;
  supla_esp_cfg.AutoCalCloseTime[0] = 1000;
  supla_esp_cfg.AutoCalOpenTime[0] = 1000;
  supla_esp_state.rs_position[0] = 100 + (50*100);
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
  EXPECT_EQ(*rsCfg->position, 100 + (50*100));
  EXPECT_EQ(*rsCfg->full_opening_time, 0);
  EXPECT_EQ(*rsCfg->full_closing_time, 0);
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  // closing and opening time is coded in 0.1s units on 2x16 bit blocks of
  // DurationMS
  // In autocalibration, server sends ct/ot == 0
  reqValue.DurationMS = (0) | (0 << 16);
  reqValue.value[0] = 100; // postion 90

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  for (int i = 0; i < 800; i++) {
    curTime += 10000; // +10ms
    executeTimers();
  }

  EXPECT_EQ(*rsCfg->position, 100 + (90*100));
  EXPECT_EQ(supla_esp_cfg.Time1[0], 0);
  EXPECT_EQ(supla_esp_cfg.Time2[0], 0);
  EXPECT_EQ(supla_esp_cfg.AutoCalOpenTime[0], 1000);
  EXPECT_EQ(supla_esp_cfg.AutoCalCloseTime[0], 1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
}

