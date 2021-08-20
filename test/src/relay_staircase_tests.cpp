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

/* Note:
 * Channel function with staircase duration time is read on device startup
 * and stored in supla_esp_cfg.Time1[channelNumber].
 * Implementation depends on an assumption that if supla_esp_cfg.Time1[] > 0
 * then it is staircase timer. In all other cases it will work as relay
 * with countdown timer.
 *
 * Thus all tests below have Time1 set and they ignore DurationMs send from
 * server.
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

// method will be called by supla_esp_gpio_init method in order to initialize
// gpio input/outputs board configuration (supla_esb_board_gpio_init)
void gpioCallbackRelayStaircase() {
  supla_relay_cfg[0].gpio_id = 1;
  supla_relay_cfg[0].channel = 0;
  supla_relay_cfg[0].flags = RELAY_FLAG_RESTORE;
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
}

TSD_SuplaRegisterDeviceResult regResultRelayStaircase;

char custom_srpc_getdata_relay_staircase(void *_srpc, TsrpcReceivedData *rd,
    unsigned _supla_int_t rr_id) {
  rd->call_type = SUPLA_SD_CALL_REGISTER_DEVICE_RESULT;
  rd->data.sd_register_device_result = &regResultRelayStaircase;
  return 1;
}

class StaircaseTests : public ::testing::Test {
public:
  SrpcMock srpc;
  TimeMock time;
  EagleSocStub eagleStub;
  int curTime;

  void SetUp() override {
    supla_esp_countdown_timer_init();
    startupTimeDelay = 0;
    upTime = 1100;
    downTime = 1200;
    memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
    memset(&supla_esp_state, 0, sizeof(SuplaEspState));
    memset(&supla_relay_cfg, 0, sizeof(supla_relay_cfg));
    supla_esp_cfg.Time1[0] = 3000; // 3 s staircase timer on channel 0
    cleanupTimers();
    supla_esp_gpio_init_time = 0;
    gpioInitCb = *gpioCallbackRelayStaircase;
    curTime = 10000; // start at +10 ms
    EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));

    EXPECT_CALL(srpc, srpc_params_init(_));
    EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return((void *)1));
    EXPECT_CALL(srpc, srpc_set_proto_version(_, 16));

    regResultRelayStaircase.result_code = SUPLA_RESULTCODE_TRUE;

    EXPECT_CALL(srpc, srpc_getdata(_, _, _))
      .WillOnce(DoAll(Invoke(custom_srpc_getdata_relay_staircase), Return(1)));
    EXPECT_CALL(srpc, srpc_rd_free(_));
    EXPECT_CALL(srpc, srpc_free(_));
    EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
    EXPECT_CALL(srpc, srpc_dcs_async_set_activity_timeout(_, _));


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
    set_reset_reason(0);

    gpioInitCb = nullptr;
  }
};


TEST_F(StaircaseTests, TurnOnWithDifferentDuration) {
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

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +1000 ms
  for (int i = 0; i < 10; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
}

TEST_F(StaircaseTests, TurnOnNoDuration) {
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
  reqValue.DurationMS = 0;
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

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +1000 ms
  for (int i = 0; i < 10; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
}

TEST_F(StaircaseTests, TurnOnByButton) {
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

  supla_esp_gpio_relay_switch(1, 1);

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

  EXPECT_TRUE(eagleStub.getGpioValue(1));

  // +1000 ms
  for (int i = 0; i < 10; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
}

TEST_F(StaircaseTests, TurnOnFor2sAfterRestart) {
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
  supla_esp_state.Time1Left[0] = 2000;

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
  EXPECT_EQ(supla_esp_state.Time1Left[0], 500);
 
  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }
  EXPECT_EQ(supla_esp_state.Time1Left[0], 0);

  EXPECT_FALSE(eagleStub.getGpioValue(1));

}

TEST_F(StaircaseTests, TurnOffFor3sAfterRestart) {
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


  }

  supla_esp_state.Relay[0] = 0;
  supla_esp_state.Time1Left[0] = 2000;

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
  EXPECT_EQ(supla_esp_state.Time1Left[0], 0);
 
  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));

  for (int i = 0; i < 60; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_EQ(supla_esp_state.Time1Left[0], 0);
}

TEST_F(StaircaseTests, TurnOnFor2sAfterRestartThenTriggerByButton) {
  EXPECT_FALSE(eagleStub.getGpioValue(1));

  EXPECT_CALL(srpc, srpc_ds_async_channel_extendedvalue_changed(_, 0, _))
    .Times(2);

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

    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({1, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

    EXPECT_CALL(srpc, 
        valueChanged(_, 0, ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0})))
      .WillOnce(Return(0));

  }

  supla_esp_state.Relay[0] = 1;
  supla_esp_state.Time1Left[0] = 2000;

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
  EXPECT_EQ(supla_esp_state.Time1Left[0], 500);
 
  // +600 ms
  for (int i = 0; i < 6; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
  EXPECT_EQ(supla_esp_state.Time1Left[0], 0);

  supla_esp_gpio_relay_switch(1, 1);
  EXPECT_TRUE(eagleStub.getGpioValue(1));
  // +2800 ms
  for (int i = 0; i < 28; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_TRUE(eagleStub.getGpioValue(1));
  EXPECT_EQ(supla_esp_state.Time1Left[0], 200);
  //
  // +300 ms
  for (int i = 0; i < 3; i++) {
    curTime += 100000; // +100ms
    executeTimers();
  }

  EXPECT_FALSE(eagleStub.getGpioValue(1));
}

