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
#include <user_interface.h>
#include <supla_esp_countdown_timer.h>
}

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

bool configureInputs = false;

// method will be called by supla_esp_gpio_init method in order to initialize
// gpio input/outputs board configuration (supla_esb_board_gpio_init)
void gpioCallbackRelay() {
  supla_relay_cfg[0].gpio_id = 1;
  supla_relay_cfg[0].channel = 0;
  supla_relay_cfg[0].flags = RELAY_FLAG_RESTORE_FORCE;
  supla_relay_cfg[0].channel_flags = 
    SUPLA_CHANNEL_FLAG_COUNTDOWN_TIMER_SUPPORTED;

  supla_relay_cfg[1].gpio_id = 2;
  supla_relay_cfg[1].channel = 1;
  supla_relay_cfg[1].flags = RELAY_FLAG_RESTORE;

  supla_relay_cfg[2].gpio_id = 3;
  supla_relay_cfg[2].channel = 2;
  supla_relay_cfg[2].flags = RELAY_FLAG_RESTORE_FORCE;

  supla_relay_cfg[3].gpio_id = 4;
  supla_relay_cfg[3].channel = 3;
  supla_relay_cfg[3].flags = RELAY_FLAG_RESET;

  supla_relay_cfg[4].gpio_id = 5;
  supla_relay_cfg[4].channel = 4;
  supla_relay_cfg[4].flags =
    RELAY_FLAG_LO_LEVEL_TRIGGER | RELAY_FLAG_RESTORE_FORCE;

  supla_relay_cfg[5].gpio_id = 6;
  supla_relay_cfg[5].channel = 5;
  supla_relay_cfg[5].flags =
    RELAY_FLAG_LO_LEVEL_TRIGGER | RELAY_FLAG_RESET;

  if (configureInputs) {
    supla_input_cfg[0].gpio_id = 7;
    supla_input_cfg[0].relay_gpio_id = 1;
    supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[0].channel = 6;

    supla_input_cfg[1].gpio_id = 8;
    supla_input_cfg[1].relay_gpio_id = 2;
    supla_input_cfg[1].type = INPUT_TYPE_MOTION_SENSOR;
    supla_input_cfg[1].action_trigger_cap = SUPLA_ACTION_CAP_TURN_ON |
      SUPLA_ACTION_CAP_TURN_OFF;
    supla_input_cfg[1].channel = 7;

    supla_input_cfg[2].gpio_id = 9;
    supla_input_cfg[2].relay_gpio_id = 3;
    supla_input_cfg[2].type = INPUT_TYPE_MOTION_SENSOR;
    supla_input_cfg[2].action_trigger_cap = SUPLA_ACTION_CAP_TURN_ON |
      SUPLA_ACTION_CAP_TURN_OFF;
    supla_input_cfg[2].channel = 8;

    supla_input_cfg[3].gpio_id = 10;
    supla_input_cfg[3].relay_gpio_id = 4;
    supla_input_cfg[3].type = INPUT_TYPE_BTN_MONOSTABLE;
    supla_input_cfg[3].channel = 9;
  }
}

TSD_SuplaRegisterDeviceResult regResultRelay;

char custom_srpc_getdata_relay(void *_srpc, TsrpcReceivedData *rd,
    unsigned _supla_int_t rr_id) {
  rd->call_type = SUPLA_SD_CALL_REGISTER_DEVICE_RESULT;
  rd->data.sd_register_device_result = &regResultRelay;
  return 1;
}

class RelayTests : public ::testing::Test {
public:
  SrpcMock srpc;
  TimeMock time;
  EagleSocStub eagleStub;
  int curTime;

  void SetUp() override {
    configureInputs = false;
    supla_esp_countdown_timer_init();
    startupTimeDelay = 0;
    upTime = 1100;
    downTime = 1200;
    memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
    memset(&supla_esp_state, 0, sizeof(SuplaEspState));
    memset(&supla_relay_cfg, 0, sizeof(supla_relay_cfg));
    cleanupTimers();
    supla_esp_gpio_init_time = 0;
    gpioInitCb = *gpioCallbackRelay;
    curTime = 10000; // start at +10 ms
    EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

    EXPECT_CALL(srpc, srpc_params_init(_));
    EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return((void *)1));
    EXPECT_CALL(srpc, srpc_set_proto_version(_, 17));

    regResultRelay.result_code = SUPLA_RESULTCODE_TRUE;

    EXPECT_CALL(srpc, srpc_getdata(_, _, _))
      .WillOnce(DoAll(Invoke(custom_srpc_getdata_relay), Return(1)));
    EXPECT_CALL(srpc, srpc_rd_free(_));
    EXPECT_CALL(srpc, srpc_free(_));
    EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
    EXPECT_CALL(srpc, srpc_dcs_async_set_activity_timeout(_, _));

  }

  void TearDown() override {
    configureInputs = false;
    supla_esp_devconn_release();
    startupTimeDelay = 0;
    upTime = 1100;
    downTime = 1200;
    memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
    memset(&supla_esp_state, 0, sizeof(SuplaEspState));
    supla_esp_gpio_init_time = 0;
    cleanupTimers();
    set_reset_reason(0);
    supla_esp_gpio_clear_vars();

    gpioInitCb = nullptr;
  }

  void moveTime(const int timeMs) {
    for (int i = 0; i < timeMs / 10; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }
};


TEST_F(RelayTests, StartupRestoreOff) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_FALSE(eagleStub.getGpioValue(5));
  EXPECT_FALSE(eagleStub.getGpioValue(6));
  supla_esp_state.Relay[0] = 0;
  supla_esp_state.Relay[1] = 0;
  supla_esp_state.Relay[2] = 0;
  supla_esp_state.Relay[3] = 0;
  supla_esp_state.Relay[4] = 0;
  supla_esp_state.Relay[5] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(1);

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_TRUE(eagleStub.getGpioValue(5)); // ivnerted logic
  EXPECT_TRUE(eagleStub.getGpioValue(6)); // ivnerted logic
}

TEST_F(RelayTests, StartupRestoreOn) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_FALSE(eagleStub.getGpioValue(5));
  EXPECT_FALSE(eagleStub.getGpioValue(6));
  supla_esp_state.Relay[0] = 1;
  supla_esp_state.Relay[1] = 1;
  supla_esp_state.Relay[2] = 1;
  supla_esp_state.Relay[3] = 1;
  supla_esp_state.Relay[4] = 1;
  supla_esp_state.Relay[5] = 1;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(1);

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  EXPECT_TRUE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_FALSE(eagleStub.getGpioValue(5)); // inverted logic
  EXPECT_TRUE(eagleStub.getGpioValue(6)); // inverted logic

}

TEST_F(RelayTests, ChangeState) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_FALSE(eagleStub.getGpioValue(5));
  EXPECT_FALSE(eagleStub.getGpioValue(6));
  supla_esp_state.Relay[0] = 0;
  supla_esp_state.Relay[1] = 0;
  supla_esp_state.Relay[2] = 0;
  supla_esp_state.Relay[3] = 0;
  supla_esp_state.Relay[4] = 0;
  supla_esp_state.Relay[5] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(1);

  {
    InSequence seq;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;


    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;


    // turn on relay on channel 1
    EXPECT_CALL(srpc, 
        valueChanged(_, 1, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 1, 0, 1))
      .Times(1)
      ;

    // turn on relay on channel 4
    EXPECT_CALL(srpc, 
        valueChanged(_, 4, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 4, 0, 1))
      .Times(1)
      ;

    // turn off by "button" relay on channel 4
    EXPECT_CALL(srpc, 
        valueChanged(_, 4, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    // turn on by "button" relay on channel 4
    EXPECT_CALL(srpc, 
        valueChanged(_, 4, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    // toggle relay by "button" on channel 4
    EXPECT_CALL(srpc, 
        valueChanged(_, 4, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_TRUE(eagleStub.getGpioValue(5)); // ivnerted logic
  EXPECT_TRUE(eagleStub.getGpioValue(6)); // ivnerted logic


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 0;
  reqValue.value[0] = 1; // turn on

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_TRUE(eagleStub.getGpioValue(5)); // ivnerted logic
  EXPECT_TRUE(eagleStub.getGpioValue(6)); // ivnerted logic

  reqValue.value[0] = 1;
  supla_esp_channel_set_value(&reqValue);

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_TRUE(eagleStub.getGpioValue(5)); // ivnerted logic
  EXPECT_TRUE(eagleStub.getGpioValue(6)); // ivnerted logic

  reqValue.value[0] = 0;
  supla_esp_channel_set_value(&reqValue);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_TRUE(eagleStub.getGpioValue(5)); // ivnerted logic
  EXPECT_TRUE(eagleStub.getGpioValue(6)); // ivnerted logic

  reqValue.ChannelNumber = 1;
  reqValue.value[0] = 1;
  supla_esp_channel_set_value(&reqValue);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_TRUE(eagleStub.getGpioValue(5)); // ivnerted logic
  EXPECT_TRUE(eagleStub.getGpioValue(6)); // ivnerted logic

  reqValue.ChannelNumber = 4;
  reqValue.value[0] = 1;
  supla_esp_channel_set_value(&reqValue);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_FALSE(eagleStub.getGpioValue(5)); // ivnerted logic
  EXPECT_TRUE(eagleStub.getGpioValue(6)); // ivnerted logic

  // turn off relay on gpio 5 (ch 4)
  supla_esp_gpio_relay_switch(5, 0);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_TRUE(eagleStub.getGpioValue(5)); // ivnerted logic
  EXPECT_TRUE(eagleStub.getGpioValue(6)); // ivnerted logic


  // turn on relay on gpio 5 (ch 4)
  supla_esp_gpio_relay_switch(5, 1);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_FALSE(eagleStub.getGpioValue(5)); // ivnerted logic
  EXPECT_TRUE(eagleStub.getGpioValue(6)); // ivnerted logic


  // toggle relay on gpio 5 (ch 4)
  supla_esp_gpio_relay_switch(5, 255);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_TRUE(eagleStub.getGpioValue(5)); // ivnerted logic
  EXPECT_TRUE(eagleStub.getGpioValue(6)); // ivnerted logic

}

TEST_F(RelayTests, Scenario1) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  supla_esp_state.Relay[0] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(1);

  {
    InSequence seq;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;
    
    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;
    
  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 0;
  reqValue.value[0] = 0; 

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
 
  // +5 s
  for (int i = 0; i < 50; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  reqValue.value[0] = 1; 

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  // +5 s
  for (int i = 0; i < 50; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));

  reqValue.value[0] = 1; 

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  // +5 s
  for (int i = 0; i < 50; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }
  EXPECT_TRUE(eagleStub.getGpioValue(1));

  reqValue.value[0] = 0; 

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  // +5 s
  for (int i = 0; i < 50; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }
  EXPECT_FALSE(eagleStub.getGpioValue(1));
}

TEST_F(RelayTests, CountdownTimerTurnOnFor2s) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  supla_esp_state.Relay[0] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(2);

  {
    InSequence seq;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 1; // turn on

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
 
  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));

}


TEST_F(RelayTests, CountdownTimerTurnOnAfter2s) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  supla_esp_state.Relay[0] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(2);

  {
    InSequence seq;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 0; // turn off for 2s, then turn on

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
 
  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +5 s
  for (int i = 0; i < 50; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
}

TEST_F(RelayTests, CountdownTimerTurnedOnTurnOnFor2s) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  supla_esp_state.Relay[0] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(2);

  {
    InSequence seq;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 0;
  reqValue.value[0] = 1; // turn on
  supla_esp_channel_set_value(&reqValue);

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +3500 ms
  for (int i = 0; i < 35; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
 

  reqValue.DurationMS = 2000;
  reqValue.value[0] = 1; // turn on

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
 
  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));

}

TEST_F(RelayTests, CountdownTimerTurnedOnTurnOffandTurnOnAfter2s) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  supla_esp_state.Relay[0] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(2);

  {
    InSequence seq;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 0;
  reqValue.value[0] = 1; // turn on
  supla_esp_channel_set_value(&reqValue);

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +3500 ms
  for (int i = 0; i < 35; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
 

  reqValue.DurationMS = 2000;
  reqValue.value[0] = 0; // turn off

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
 
  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));

}

TEST_F(RelayTests, CountdownTimerTurnOnAfter2sWithCancel) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  supla_esp_state.Relay[0] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(3);

  {
    InSequence seq;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;


  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 0; // turn off for 2s, then turn on

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
 
  reqValue.DurationMS = 0;
  reqValue.value[0] = 0; // turn off for 2s, then turn on

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +5 s
  for (int i = 0; i < 50; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
}

TEST_F(RelayTests, CountdownTimerTurnOnAfter2sWithInstantTurnOn) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  supla_esp_state.Relay[0] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(3);

  {
    InSequence seq;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;


  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 0; // turn off for 2s, then turn on

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
 
  reqValue.DurationMS = 0;
  reqValue.value[0] = 1; // turn off for 2s, then turn on

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +5 s
  for (int i = 0; i < 50; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
}

TEST_F(RelayTests, CountdownTimerTurnOnAfter2sWithResetTimer) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  supla_esp_state.Relay[0] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(4);

  {
    InSequence seq;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 0; // turn off for 2s, then turn on

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
 
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 0; // turn off for 2s, then turn on

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +1.5 s
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +15 s
  for (int i = 0; i < 150; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
}

TEST_F(RelayTests, CountdownTimerTurnOnAfter2sWithTurnOnFor2s) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  supla_esp_state.Relay[0] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(4);

  {
    InSequence seq;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 0; // turn off for 2s, then turn on

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
 
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 1; // turn on for 2s, then turn off

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +1.5 s
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +15 s
  for (int i = 0; i < 150; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
}

TEST_F(RelayTests, CountdownTimerTurnedOnFor2sThenTurnOff) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  supla_esp_state.Relay[0] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(3);

  {
    InSequence seq;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;


  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 1; // turn on for 2s, then turn off

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
 
  reqValue.DurationMS = 0;
  reqValue.value[0] = 0; // turn off

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);
  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +5 s
  for (int i = 0; i < 50; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
}

TEST_F(RelayTests, CountdownTimerTurnedOnFor2sThenTurnOn) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  supla_esp_state.Relay[0] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(3);

  {
    InSequence seq;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;


  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 1; // turn on for 2s, then turn off

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
 
  reqValue.DurationMS = 0;
  reqValue.value[0] = 1; // turn on

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);
  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +5 s
  for (int i = 0; i < 50; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
}

TEST_F(RelayTests, CountdownTimerTurnedOnFor2sThenTurnOffFor2s) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  supla_esp_state.Relay[0] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(4);

  {
    InSequence seq;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));


  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 1; // turn on for 2s, then turn off

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
 
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 0; // turn on

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);
  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +5 s
  for (int i = 0; i < 50; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
}

TEST_F(RelayTests, CountdownTimerTurnedOnFor2sThenResetTimer) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  supla_esp_state.Relay[0] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(4);

  {
    InSequence seq;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));


  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 1; // turn on for 2s, then turn off

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
 
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 1; // turn on

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);
  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +1.3 s
  for (int i = 0; i < 13; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +0.2 s
  for (int i = 0; i < 2; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
}

TEST_F(RelayTests, CountdownTimerTurnOnFor2sThenTurnOnByButton) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  supla_esp_state.Relay[0] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(3);

  {
    InSequence seq;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 1; // turn on for 2s, then turn off

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
 
  // turn off relay on gpio 1 (ch 0)
  supla_esp_gpio_relay_switch(1, 1);

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  for (int i = 0; i < 63; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));

}

TEST_F(RelayTests, CountdownTimerTurnOnFor2sThenTurnOffByButton) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  supla_esp_state.Relay[0] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(3);

  {
    InSequence seq;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 1; // turn on for 2s, then turn off

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
 
  // turn off relay on gpio 1 (ch 0)
  supla_esp_gpio_relay_switch(1, 0);

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  for (int i = 0; i < 63; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));

}

TEST_F(RelayTests, CountdownTimerTurnOnFor2sThenToggleByButton) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  supla_esp_state.Relay[0] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(3);

  {
    InSequence seq;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 1; // turn on for 2s, then turn off

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
 
  // turn off relay on gpio 1 (ch 0)
  supla_esp_gpio_relay_switch(1, 255); // 255=toggle

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  for (int i = 0; i < 63; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
}

TEST_F(RelayTests, CountdownTimerTurnOffFor2sThenToggleByButton) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  supla_esp_state.Relay[0] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(3);

  {
    InSequence seq;

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, srpc_ds_async_set_channel_result(_, 0, 0, 1))
      .Times(1)
      ;

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

  }

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  // +2000 ms
  for (int i = 0; i < 20; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = 2000;
  reqValue.value[0] = 0; // turn off for 2s, then turn on

  // simulate request from server
  supla_esp_channel_set_value(&reqValue);

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
 
  // turn on relay on gpio 1 (ch 0)
  supla_esp_gpio_relay_switch(1, 255); // 255=toggle

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  for (int i = 0; i < 63; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
}

TEST_F(RelayTests, CountdownTimerTurnOnFor2sAfterRestart) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(1);

  {
    InSequence seq;

    // Values send on startup during registration are filled by board methods,
    // so it can't be verified here. However we check gpio state if relay is
    // enabled.
    // turn on relay on channel 0
//    EXPECT_CALL(srpc, 
//        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
//      .WillOnce(Return(0));

    // turn off relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

  }

  supla_esp_state.Relay[0] = 1;
  supla_esp_state.Time2Left[0] = 2000;

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);

  EXPECT_TRUE(eagleStub.getGpioValue(1));


  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_EQ(supla_esp_state.Time2Left[0], 500);
 
  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_EQ(supla_esp_state.Time2Left[0], 0);

}

TEST_F(RelayTests, CountdownTimerTurnOffFor2sAfterRestart) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(1);

  {
    InSequence seq;

    // Values send on startup during registration are filled by board methods,
    // so it can't be verified here. However we check gpio state if relay is
    // enabled.
//    EXPECT_CALL(srpc, 
//        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
//      .WillOnce(Return(0));

    // turn on relay on channel 0
    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

  }

  supla_esp_state.Relay[0] = 0;
  supla_esp_state.Time2Left[0] = 2000;

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);

  EXPECT_FALSE(eagleStub.getGpioValue(1));


  // +1500 ms
  for (int i = 0; i < 15; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_EQ(supla_esp_state.Time2Left[0], 500);
 
  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_EQ(supla_esp_state.Time2Left[0], 0);

  for (int i = 0; i < 60; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_EQ(supla_esp_state.Time2Left[0], 0);
}


TEST_F(RelayTests, StartupRestoreWithMotionSensor) {
  configureInputs = true;
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_FALSE(eagleStub.getGpioValue(5));
  EXPECT_FALSE(eagleStub.getGpioValue(6));
  supla_esp_state.Relay[0] = 1;
  supla_esp_state.Relay[1] = 1;
  supla_esp_state.Relay[2] = 1;
  supla_esp_state.Relay[3] = 1;
  supla_esp_state.Relay[4] = 1;
  supla_esp_state.Relay[5] = 1;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(1);

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2)); // relay with motion sensor
  EXPECT_FALSE(eagleStub.getGpioValue(3)); // relay with motion sensor
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_FALSE(eagleStub.getGpioValue(5)); // inverted logic
  EXPECT_TRUE(eagleStub.getGpioValue(6)); // inverted logic

}

TEST_F(RelayTests, RelayConnectionWithMotionSensor) {
  configureInputs = true;
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_FALSE(eagleStub.getGpioValue(5));
  EXPECT_FALSE(eagleStub.getGpioValue(6));
  supla_esp_state.Relay[0] = 0;
  supla_esp_state.Relay[1] = 0;
  supla_esp_state.Relay[2] = 0;
  supla_esp_state.Relay[3] = 0;
  supla_esp_state.Relay[4] = 0;
  supla_esp_state.Relay[5] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(1);

  // turn on relay on channel 1
  EXPECT_CALL(srpc,
      valueChanged(_, 1, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
    .Times(2)
    .WillRepeatedly(Return(0));

  EXPECT_CALL(srpc,
      valueChanged(_, 1, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
    .Times(3)
    .WillRepeatedly(Return(0));

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2)); // relay with motion sensor
  EXPECT_FALSE(eagleStub.getGpioValue(3)); // relay with motion sensor
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_TRUE(eagleStub.getGpioValue(5)); // inverted logic
  EXPECT_TRUE(eagleStub.getGpioValue(6)); // inverted logic

  supla_input_cfg_t *input = &supla_input_cfg[1];
  EXPECT_TRUE(supla_esp_input_is_relay_connection_enabled(input));

  supla_esp_input_set_active_triggers(input, 0);
  EXPECT_TRUE(supla_esp_input_is_relay_connection_enabled(input));
  supla_esp_gpio_on_input_active(input);
  EXPECT_TRUE(eagleStub.getGpioValue(2)); // notif to server: ON
  supla_esp_gpio_on_input_inactive(input);
  EXPECT_FALSE(eagleStub.getGpioValue(2)); // notif to server: OFF

  supla_esp_input_set_active_triggers(input, SUPLA_ACTION_CAP_TURN_ON);
  EXPECT_TRUE(supla_esp_input_is_relay_connection_enabled(input));
  supla_esp_gpio_on_input_active(input);
  EXPECT_FALSE(eagleStub.getGpioValue(2)); // nothing
  supla_esp_gpio_on_input_inactive(input);
  EXPECT_FALSE(eagleStub.getGpioValue(2)); // notif to server: OFF

  supla_esp_input_set_active_triggers(input, SUPLA_ACTION_CAP_TURN_OFF);
  EXPECT_TRUE(supla_esp_input_is_relay_connection_enabled(input));
  supla_esp_gpio_on_input_active(input);
  EXPECT_TRUE(eagleStub.getGpioValue(2)); // notif to server: ON
  supla_esp_gpio_on_input_inactive(input);
  EXPECT_TRUE(eagleStub.getGpioValue(2)); // nothing

  supla_esp_input_set_active_triggers(input, SUPLA_ACTION_CAP_TURN_OFF |
      SUPLA_ACTION_CAP_TURN_ON);
  EXPECT_FALSE(supla_esp_input_is_relay_connection_enabled(input));
  supla_esp_gpio_on_input_active(input);
  EXPECT_TRUE(eagleStub.getGpioValue(2)); // nothing
  supla_esp_gpio_on_input_inactive(input);
  EXPECT_TRUE(eagleStub.getGpioValue(2)); // nothing

  supla_esp_input_set_active_triggers(input, SUPLA_ACTION_CAP_TURN_ON);
  EXPECT_TRUE(supla_esp_input_is_relay_connection_enabled(input));
  supla_esp_gpio_on_input_active(input);
  EXPECT_TRUE(eagleStub.getGpioValue(2)); // nothing
  supla_esp_gpio_on_input_inactive(input);
  EXPECT_FALSE(eagleStub.getGpioValue(2)); // notif to server: OFF


  supla_esp_input_set_active_triggers(input, SUPLA_ACTION_CAP_TURN_OFF |
      SUPLA_ACTION_CAP_TURN_ON);
  EXPECT_FALSE(supla_esp_input_is_relay_connection_enabled(input));
  supla_esp_gpio_on_input_active(input);
  EXPECT_FALSE(eagleStub.getGpioValue(2)); // nothing
  supla_esp_gpio_on_input_inactive(input);
  EXPECT_FALSE(eagleStub.getGpioValue(2)); // nothing

}

TEST_F(RelayTests, RelayConnectionWithMotionSensorWithAT) {
  configureInputs = true;
  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  EXPECT_FALSE(eagleStub.getGpioValue(3));
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_FALSE(eagleStub.getGpioValue(5));
  EXPECT_FALSE(eagleStub.getGpioValue(6));
  supla_esp_state.Relay[0] = 0;
  supla_esp_state.Relay[1] = 0;
  supla_esp_state.Relay[2] = 0;
  supla_esp_state.Relay[3] = 0;
  supla_esp_state.Relay[4] = 0;
  supla_esp_state.Relay[5] = 0;

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(1);

  // turn on relay on channel 1
  EXPECT_CALL(srpc,
      valueChanged(_, 1, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
    .WillRepeatedly(Return(0));

  EXPECT_CALL(srpc,
      valueChanged(_, 1, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
    .WillRepeatedly(Return(0));

  EXPECT_CALL(srpc,
      valueChanged(_, 2, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
    .WillRepeatedly(Return(0));

  EXPECT_CALL(
      srpc, srpc_ds_async_action_trigger(7, SUPLA_ACTION_CAP_TURN_OFF))
    .Times(3);

  EXPECT_CALL(
      srpc, srpc_ds_async_action_trigger(7, SUPLA_ACTION_CAP_TURN_ON))
    .Times(2);

  supla_esp_gpio_init();

  supla_esp_devconn_init();
  supla_esp_srpc_init();
  ASSERT_NE(srpc.on_remote_call_received, nullptr);
  srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
  EXPECT_EQ(supla_esp_devconn_is_registered(), 1);

  moveTime(1000);

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_FALSE(eagleStub.getGpioValue(2)); // relay with motion sensor
  EXPECT_FALSE(eagleStub.getGpioValue(3)); // relay with motion sensor
  EXPECT_FALSE(eagleStub.getGpioValue(4));
  EXPECT_TRUE(eagleStub.getGpioValue(5)); // inverted logic
  EXPECT_TRUE(eagleStub.getGpioValue(6)); // inverted logic

  supla_input_cfg_t *input = &supla_input_cfg[1];
  EXPECT_TRUE(supla_esp_input_is_relay_connection_enabled(input));

  // button press
  eagleStub.gpioOutputSet(8, 1);
  ets_gpio_intr_func(NULL);
  moveTime(1000);

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // button release
  eagleStub.gpioOutputSet(8, 0);
  ets_gpio_intr_func(NULL);
  moveTime(1000);
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // enabling AT TURN OFF
  supla_esp_input_set_active_triggers(input, SUPLA_ACTION_CAP_TURN_OFF);
  EXPECT_TRUE(supla_esp_input_is_relay_connection_enabled(input));

  // button press
  eagleStub.gpioOutputSet(8, 1);
  ets_gpio_intr_func(NULL);
  moveTime(1000);

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // buton release
  eagleStub.gpioOutputSet(8, 0);
  ets_gpio_intr_func(NULL);
  moveTime(1000);
  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // button press
  eagleStub.gpioOutputSet(8, 1);
  ets_gpio_intr_func(NULL);
  moveTime(1000);

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // disable relay
  supla_esp_gpio_relay_hi(2, 0); // turn gpio off
  EXPECT_FALSE(eagleStub.getGpioValue(2));
  moveTime(1000);

  // button release
  eagleStub.gpioOutputSet(8, 0);
  ets_gpio_intr_func(NULL);
  moveTime(1000);
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // button press
  eagleStub.gpioOutputSet(8, 1);
  ets_gpio_intr_func(NULL);
  moveTime(200);

  // buton release
  eagleStub.gpioOutputSet(8, 0);
  ets_gpio_intr_func(NULL);
  moveTime(1000);

  EXPECT_TRUE(eagleStub.getGpioValue(2));
  supla_esp_gpio_relay_hi(2, 0); // turn gpio off
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // enabling AT TURN ON (option with OFF is disabled)
  supla_esp_input_set_active_triggers(input, SUPLA_ACTION_CAP_TURN_ON);
  EXPECT_TRUE(supla_esp_input_is_relay_connection_enabled(input));

  // button press
  eagleStub.gpioOutputSet(8, 1);
  ets_gpio_intr_func(NULL);
  moveTime(1000);

  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // button release
  eagleStub.gpioOutputSet(8, 0);
  ets_gpio_intr_func(NULL);
  moveTime(1000);
  EXPECT_FALSE(eagleStub.getGpioValue(2));

  // enable relay
  supla_esp_gpio_relay_hi(2, 1);
  EXPECT_TRUE(eagleStub.getGpioValue(2));
  moveTime(1000);

  // button press
  eagleStub.gpioOutputSet(8, 1);
  ets_gpio_intr_func(NULL);
  moveTime(1000);

  EXPECT_TRUE(eagleStub.getGpioValue(2));

  // button release
  eagleStub.gpioOutputSet(8, 0);
  ets_gpio_intr_func(NULL);
  moveTime(1000);
  EXPECT_FALSE(eagleStub.getGpioValue(2));

}

