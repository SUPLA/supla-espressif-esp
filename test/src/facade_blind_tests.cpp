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
using ::testing::AnyNumber;

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

TSD_SuplaRegisterDeviceResult regResultFacadeBlinds;

char custom_srpc_getdata_facade_blinds(void *_srpc, TsrpcReceivedData *rd,
    unsigned _supla_int_t rr_id) {
  rd->call_id = SUPLA_SD_CALL_REGISTER_DEVICE_RESULT;
  rd->data.sd_register_device_result = &regResultFacadeBlinds;
  return 1;
}
class FacadeBlindTestsF : public ::testing::Test {
public:
  SrpcMock srpc;
  TimeMock time;
  EagleSocStub eagleStub;
  int curTime = 10000; // start at +10 ms
  supla_roller_shutter_cfg_t *rsCfg = nullptr;

  void SetUp() override {
    supla_esp_gpio_rs_set_time_margin(110);
    memset(&supla_esp_cfg, 0, sizeof(supla_esp_cfg));
    memset(&supla_esp_state, 0, sizeof(SuplaEspState));
    memset(&supla_relay_cfg, 0, sizeof(supla_relay_cfg));
    supla_esp_gpio_init_time = 0;
    gpioInitCb = *gpioCallbackFacadeBlind;

    EXPECT_CALL(time, system_get_time()).WillRepeatedly(ReturnPointee(&curTime));
    supla_esp_gpio_init();
    rsCfg = supla_esp_gpio_get_rs__cfg(1);
    ASSERT_NE(rsCfg, nullptr);
    // full opening and closing times are 10s, while tilt change takes 2s
    // facade blind is fully open
    *rsCfg->full_opening_time = 10000;
    *rsCfg->full_closing_time = 10000;
    *rsCfg->position = 100;
    *rsCfg->tilt_change_time = 2000;
    *rsCfg->tilt = 100;

    EXPECT_CALL(srpc, srpc_params_init(_));
    EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return((void *)1));
    EXPECT_CALL(srpc, srpc_set_proto_version(_, _));

    regResultFacadeBlinds.result_code = SUPLA_RESULTCODE_TRUE;

    EXPECT_CALL(srpc, srpc_getdata(_, _, _))
      .WillOnce(DoAll(Invoke(custom_srpc_getdata_facade_blinds), Return(1)));
    EXPECT_CALL(srpc, srpc_rd_free(_));
    EXPECT_CALL(srpc, srpc_free(_));
    EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
    EXPECT_CALL(srpc, srpc_dcs_async_set_activity_timeout(_, _));

    supla_esp_devconn_init();
    // this is called to disable devconn watchdog timer
    supla_esp_devconn_before_update_start();

    supla_esp_srpc_init();
    ASSERT_NE(srpc.on_remote_call_received, nullptr);

    srpc.on_remote_call_received((void *)1, 0, 0, nullptr, 0);
    EXPECT_EQ(supla_esp_devconn_is_registered(), 1);
  }

  void TearDown() override {
    supla_esp_devconn_release();
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

  void moveTime(const int timeMs) {
    for (int i = 0; i < timeMs / 10; i++) {
      curTime += 10000; // +10ms
      executeTimers();
    }
  }
};

TEST_F(FacadeBlindTestsF, FacadeBlindKeepPositionWhileTiltingMoveUpDown) {
  // ignore value changes in this test
  EXPECT_CALL(
      srpc, valueChanged(_, 0, _)).Times(AnyNumber());

  rsCfg->tilt_type = FB_TILT_TYPE_KEEP_POSITION_WHILE_TILTING;

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +2000 ms
  moveTime(2000);

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_DOWN, 1, 1);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(1000);
  EXPECT_EQ(*rsCfg->position, 100); // position shouldn't chnage
  EXPECT_EQ(*rsCfg->tilt, 100 + (50 * 100)); // tilt should be at 50%
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(1000);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100)); // tilt should be at 100%
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(10);
  EXPECT_NE(*rsCfg->position, 100);

  moveTime(790);
  EXPECT_EQ(*rsCfg->position, 100 + (10 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(7200);
  EXPECT_EQ(*rsCfg->position, 100 + (100 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(15000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_UP, 1, 1);
  moveTime(500);
  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (75 * 100));
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_DOWN, 1, 1);
  moveTime(1000);
  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (75 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(600);
  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(FacadeBlindTestsF, FacadeBlindChangePositionWhileTiltingMoveUpDown) {
  // ignore value changes in this test
  EXPECT_CALL(
      srpc, valueChanged(_, 0, _)).Times(AnyNumber());

  rsCfg->tilt_type = FB_TILT_TYPE_CHANGE_POSITION_WHILE_TILTING;

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(2000);

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // Move RS down.
  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_DOWN, 1, 1);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(1000);
  EXPECT_EQ(*rsCfg->position, 100 + (10 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (50 * 100)); // tilt should be at 50%
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(1000);
  EXPECT_EQ(*rsCfg->position, 100 + (20 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100)); // tilt should be at 100%
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(10);
  EXPECT_NE(*rsCfg->position, 100 + (20 * 100));

  moveTime(990);
  EXPECT_EQ(*rsCfg->position, 100 + (30 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(7000);
  EXPECT_EQ(*rsCfg->position, 100 + (100 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(15000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_UP, 1, 1);
  moveTime(500);
  EXPECT_EQ(*rsCfg->position, 100 + (95 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (75 * 100));
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_DOWN, 1, 1);
  moveTime(1000);
  EXPECT_EQ(*rsCfg->position, 100 + (95 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (75 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(600);
  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(FacadeBlindTestsF, FacadeBlindFixedTiltMoveUpDown) {
  // ignore value changes in this test
  EXPECT_CALL(
      srpc, valueChanged(_, 0, _)).Times(AnyNumber());

  // facade blinds has a fixed tilt at any position other than fully
  // closed. Tilting is possible only at fully closed
  // tilt change should be ignored if it is requested with
  // position == -1 and when actual position != 100

  rsCfg->tilt_type = FB_TILT_TYPE_TILTING_ONLY_AT_FULLY_CLOSED;

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(2000);

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // Move RS down.
  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_DOWN, 1, 1);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(800);
  EXPECT_EQ(*rsCfg->position, 100 + (10 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100); // tilt should not change
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(800);
  EXPECT_EQ(*rsCfg->position, 100 + (20 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(8*800);
  EXPECT_EQ(*rsCfg->position, 100 + (100 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100);

  moveTime(1000);
  EXPECT_EQ(*rsCfg->position, 100 + (100 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (50 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(1000);
  EXPECT_EQ(*rsCfg->position, 100 + (100 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(15000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_UP, 1, 1);
  moveTime(500);
  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (75 * 100));
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_set_relay(rsCfg, RS_RELAY_DOWN, 1, 1);
  moveTime(1000);
  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (75 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(600);
  EXPECT_EQ(*rsCfg->position, 10100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(FacadeBlindTestsF, FacadeBlindKeepPositionWhileTiltingTasks) {
  // ignore value changes in this test
  EXPECT_CALL(
      srpc, valueChanged(_, 0, _)).Times(AnyNumber());

  rsCfg->tilt_type = FB_TILT_TYPE_KEEP_POSITION_WHILE_TILTING;

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(2000);

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, 30, 0);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (30 * 100), 50);
  EXPECT_EQ(*rsCfg->tilt, 100 + (0 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, 20, 0);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (20 * 100), 50);
  EXPECT_EQ(*rsCfg->tilt, 100 + (0 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, 20, 60);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (20 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (60 * 100), 50);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));


  supla_esp_gpio_rs_add_task(0, 90, 25);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (90 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (25 * 100), 50);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, 100, 100);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (100 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (100 * 100), 50);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, 0, 0);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (0 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (0 * 100), 50);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));


  supla_esp_gpio_rs_add_task(0, 30, -1);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (30 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (0 * 100), 50);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, 30, 10);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (30 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (10 * 100), 50);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, -1, 90);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (30 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (90 * 100), 50);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, 80, 20);
  moveTime(1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, -1, 50);
  moveTime(1000);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_TRUE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, -1, 10);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (80 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (10 * 100), 50);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(FacadeBlindTestsF, FacadeBlindChangePositionWhileTiltingTasks) {
  // ignore value changes in this test
  EXPECT_CALL(
      srpc, valueChanged(_, 0, _)).Times(AnyNumber());

  rsCfg->tilt_type = FB_TILT_TYPE_CHANGE_POSITION_WHILE_TILTING;

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(2000);

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, 30, 0);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (30 * 100), 50);
  EXPECT_EQ(*rsCfg->tilt, 100 + (0 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, 20, 0);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (20 * 100), 50);
  EXPECT_EQ(*rsCfg->tilt, 100 + (0 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, 20, 50);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (20 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (50 * 100), 50);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, 90, 50);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (90 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (50 * 100), 50);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, 100, 100);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (100 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (100 * 100), 50);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, 0, 0);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (0 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (0 * 100), 50);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, 30, -1);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (30 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (0 * 100), 50);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, 30, 10);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (30 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (10 * 100), 50);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, -1, 90);
  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (46 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (90 * 100), 50);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(FacadeBlindTestsF, FacadeBlindFixedTiltAddTask) {
  // ignore value changes in this test
  EXPECT_CALL(
      srpc, valueChanged(_, 0, _)).Times(AnyNumber());

  // facade blinds has a fixed tilt at any position other than fully
  // closed. Tilting is possible only at fully closed
  // tilt change should be ignored if it is requested with
  // position == -1 and when actual position != 100

  rsCfg->tilt_type = FB_TILT_TYPE_TILTING_ONLY_AT_FULLY_CLOSED;

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(2000);

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // Request tilt change, while it it not possible
  supla_esp_gpio_rs_add_task(0, -1, 90);

  moveTime(800);
  EXPECT_EQ(*rsCfg->position, 100 + (0 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100); // tilt should not change
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  moveTime(1500);

  // Request postion=30 and tilt=50. Tilt setting is not possible at this
  // position, so it will be ignored
  supla_esp_gpio_rs_add_task(0, 30, 50);
  moveTime(5000);
  EXPECT_NEAR(*rsCfg->position, 100 + (30*100), 50);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // Request tilt change, while it it not possible
  supla_esp_gpio_rs_add_task(0, -1, 90);

  moveTime(800);
  EXPECT_NEAR(*rsCfg->position, 100 + (30*100), 50);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // Request tilt change, while it it not possible
  supla_esp_gpio_rs_add_task(0, 100, -1);

  moveTime(15000);
  EXPECT_NEAR(*rsCfg->position, 100 + (100*100), 50);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // Request tilt change
  supla_esp_gpio_rs_add_task(0, -1, 60);

  moveTime(8000);
  EXPECT_NEAR(*rsCfg->position, 100 + (100*100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (60 * 100), 50);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  supla_esp_gpio_rs_add_task(0, 95, -1);

  moveTime(8000);
  EXPECT_NEAR(*rsCfg->position, 100 + (95*100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (0 * 100), 50);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));
}

TEST_F(FacadeBlindTestsF, FacadeBlindKeepPositionWhileTiltingReqFromServer) {
  // ignore value changes in this test
  EXPECT_CALL(
      srpc, valueChanged(_, 0, _)).Times(AnyNumber());

  rsCfg->tilt_type = FB_TILT_TYPE_KEEP_POSITION_WHILE_TILTING;

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +2000 ms
  moveTime(2000);

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = (100) | (100 << 16);

  reqValue.value[0] = 1; // move down
  supla_esp_channel_set_value(&reqValue);
  moveTime(12000);
  EXPECT_EQ(*rsCfg->position, 100 + (100 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));

  reqValue.value[0] = 2; // move up
  supla_esp_channel_set_value(&reqValue);
  moveTime(12000);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (0 * 100));
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 0; // stop
  supla_esp_channel_set_value(&reqValue);
  moveTime(1000);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (0 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+50; // 50
  supla_esp_channel_set_value(&reqValue);
  moveTime(10000);
  EXPECT_EQ(*rsCfg->position, 100 + (50 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (0 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+50; // position
  reqValue.value[1] = 10+50; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(10000);
  EXPECT_EQ(*rsCfg->position, 100 + (50 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (50 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // commands stop/up/down on position field have priority over tilt setting
  // which is ignored in this case
  reqValue.value[0] = 0; // position stop
  reqValue.value[1] = 10+20; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(10000);
  EXPECT_EQ(*rsCfg->position, 100 + (50 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (50 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 1; // position move down
  reqValue.value[1] = 10+20; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(1000);
  EXPECT_EQ(*rsCfg->position, 100 + (50 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));

  reqValue.value[0] = 2; // position move up
  reqValue.value[1] = 10+20; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(1000 + 1000);
  EXPECT_EQ(*rsCfg->position, 100 + (50 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (50 * 100), 60);

  reqValue.value[0] = 0; // position stop
  reqValue.value[1] = 10+20; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(1000);
  EXPECT_EQ(*rsCfg->position, 100 + (50 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (50 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = -1; // position
  reqValue.value[1] = 10+20; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(2000);
  EXPECT_EQ(*rsCfg->position, 100 + (50 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (20 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = -1; // position
  reqValue.value[1] = 10+100; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(3000);
  EXPECT_EQ(*rsCfg->position, 100 + (50 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (100 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+20; // position
  reqValue.value[1] = 10+90; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(10000);
  EXPECT_EQ(*rsCfg->position, 100 + (20 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (90 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+70; // position
  reqValue.value[1] = -1; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(10000);
  EXPECT_EQ(*rsCfg->position, 100 + (70 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (90 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+71; // position
  reqValue.value[1] = 0; // tilt values 0-9 are ignored and converted to -1
  supla_esp_channel_set_value(&reqValue);
  moveTime(2000);
  EXPECT_EQ(*rsCfg->position, 100 + (71 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (90 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+69; // position
  reqValue.value[1] = 1; // tilt values 0-9 are ignored and converted to -1
  supla_esp_channel_set_value(&reqValue);
  moveTime(7000); // move up requires more time
  EXPECT_EQ(*rsCfg->position, 100 + (69 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (90 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+68; // position
  reqValue.value[1] = 2; // tilt values 0-9 are ignored and converted to -1
  supla_esp_channel_set_value(&reqValue);
  moveTime(7000);
  EXPECT_EQ(*rsCfg->position, 100 + (68 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (90 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+67; // position
  reqValue.value[1] = 9; // tilt values 0-9 are ignored and converted to -1
  supla_esp_channel_set_value(&reqValue);
  moveTime(7000);
  EXPECT_EQ(*rsCfg->position, 100 + (67 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (90 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

}

TEST_F(FacadeBlindTestsF, FacadeBlindChangePositionWhileTiltingReqFromServer) {
  // ignore value changes in this test
  EXPECT_CALL(
      srpc, valueChanged(_, 0, _)).Times(AnyNumber());

  rsCfg->tilt_type = FB_TILT_TYPE_CHANGE_POSITION_WHILE_TILTING;

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +2000 ms
  moveTime(2000);

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = (100) | (100 << 16);

  reqValue.value[0] = 1; // move down
  supla_esp_channel_set_value(&reqValue);
  moveTime(12000);
  EXPECT_EQ(*rsCfg->position, 100 + (100 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));

  reqValue.value[0] = 2; // move up
  supla_esp_channel_set_value(&reqValue);
  moveTime(12000);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (0 * 100));
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 0; // stop
  supla_esp_channel_set_value(&reqValue);
  moveTime(1000);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (0 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+50; // 50
  supla_esp_channel_set_value(&reqValue);
  moveTime(12000);
  EXPECT_NEAR(*rsCfg->position, 100 + (50 * 100), 50);
  EXPECT_EQ(*rsCfg->tilt, 100 + (0 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+50; // position
  reqValue.value[1] = 10+50; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(10000);
  EXPECT_EQ(*rsCfg->position, 100 + (50 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (50 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // commands stop/up/down on position field have priority over tilt setting
  // which is ignored in this case
  reqValue.value[0] = 0; // position stop
  reqValue.value[1] = 10+20; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(10000);
  EXPECT_EQ(*rsCfg->position, 100 + (50 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (50 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 1; // position move down
  reqValue.value[1] = 10+20; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(1000);
  EXPECT_EQ(*rsCfg->position, 100 + (60 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));

  reqValue.value[0] = 2; // position move up
  reqValue.value[1] = 10+20; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(1000 + 1000);
  EXPECT_NEAR(*rsCfg->position, 100 + (50 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (50 * 100), 60);

  reqValue.value[0] = 0; // position stop
  reqValue.value[1] = 10+20; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(1000);
  EXPECT_NEAR(*rsCfg->position, 100 + (50 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (50 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = -1; // position
  reqValue.value[1] = 10+0; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(2000);
  EXPECT_EQ(*rsCfg->position, 100 + (40 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (0 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = -1; // position
  reqValue.value[1] = 10+100; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(3000);
  EXPECT_EQ(*rsCfg->position, 100 + (60 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (100 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+20; // position
  reqValue.value[1] = 10+90; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(10000);
  EXPECT_EQ(*rsCfg->position, 100 + (20 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (90 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+70; // position
  reqValue.value[1] = -1; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(10000);
  EXPECT_EQ(*rsCfg->position, 100 + (70 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (90 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+71; // position
  reqValue.value[1] = 0; // tilt values 0-9 are ignored and converted to -1
  supla_esp_channel_set_value(&reqValue);
  moveTime(2000);
  EXPECT_EQ(*rsCfg->position, 100 + (71 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (90 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+69; // position
  reqValue.value[1] = 1; // tilt values 0-9 are ignored and converted to -1
  supla_esp_channel_set_value(&reqValue);
  moveTime(7000); // move up requires more time
  EXPECT_EQ(*rsCfg->position, 100 + (69 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (90 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+68; // position
  reqValue.value[1] = 2; // tilt values 0-9 are ignored and converted to -1
  supla_esp_channel_set_value(&reqValue);
  moveTime(7000);
  EXPECT_EQ(*rsCfg->position, 100 + (68 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (90 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+67; // position
  reqValue.value[1] = 9; // tilt values 0-9 are ignored and converted to -1
  supla_esp_channel_set_value(&reqValue);
  moveTime(7000);
  EXPECT_EQ(*rsCfg->position, 100 + (67 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (90 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

}

TEST_F(FacadeBlindTestsF, FacadeBlindFixedTiltReqFromServer) {
  // ignore value changes in this test
  EXPECT_CALL(
      srpc, valueChanged(_, 0, _)).Times(AnyNumber());

  rsCfg->tilt_type = FB_TILT_TYPE_TILTING_ONLY_AT_FULLY_CLOSED;

  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // +2000 ms
  moveTime(2000);

  // nothing should change
  EXPECT_EQ(rsCfg->up_time, 0);
  EXPECT_EQ(rsCfg->down_time, 0);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  TSD_SuplaChannelNewValue reqValue = {};
  reqValue.ChannelNumber = 0;
  reqValue.DurationMS = (100) | (100 << 16);

  reqValue.value[0] = 1; // move down
  supla_esp_channel_set_value(&reqValue);
  moveTime(12000);
  EXPECT_EQ(*rsCfg->position, 100 + (100 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (100 * 100));

  reqValue.value[0] = 2; // move up
  supla_esp_channel_set_value(&reqValue);
  moveTime(12000);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (0 * 100));
  EXPECT_TRUE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 0; // stop
  supla_esp_channel_set_value(&reqValue);
  moveTime(1000);
  EXPECT_EQ(*rsCfg->position, 100);
  EXPECT_EQ(*rsCfg->tilt, 100 + (0 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+50; // 50
  supla_esp_channel_set_value(&reqValue);
  moveTime(12000);
  EXPECT_NEAR(*rsCfg->position, 100 + (50 * 100), 50);
  EXPECT_EQ(*rsCfg->tilt, 100 + (0 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+50; // position
  reqValue.value[1] = 10+50; // tilt is ignored when position != 100
  supla_esp_channel_set_value(&reqValue);
  moveTime(10000);
  EXPECT_EQ(*rsCfg->position, 100 + (50 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (0 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  // commands stop/up/down on position field have priority over tilt setting
  // which is ignored in this case
  reqValue.value[0] = 0; // position stop
  reqValue.value[1] = 10+20; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(10000);
  EXPECT_EQ(*rsCfg->position, 100 + (50 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (0 * 100));
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 1; // position move down
  reqValue.value[1] = 10+20; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(800);
  EXPECT_EQ(*rsCfg->position, 100 + (60 * 100));
  EXPECT_EQ(*rsCfg->tilt, 100 + (0 * 100));

  reqValue.value[0] = 2; // position move up
  reqValue.value[1] = 10+20; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(1000 + 800);
  EXPECT_NEAR(*rsCfg->position, 100 + (50 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (0 * 100), 60);

  reqValue.value[0] = 0; // position stop
  reqValue.value[1] = 10+20; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(1000);
  EXPECT_NEAR(*rsCfg->position, 100 + (50 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (0 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = -1; // position
  reqValue.value[1] = 10+0; // tilt is ignored - current position != 100
  supla_esp_channel_set_value(&reqValue);
  moveTime(2000);
  EXPECT_NEAR(*rsCfg->position, 100 + (50 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (0 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = -1; // position
  reqValue.value[1] = 10+100; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(3000);
  EXPECT_NEAR(*rsCfg->position, 100 + (50 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (0 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+20; // position
  reqValue.value[1] = 10+90; // tilt - ignored
  supla_esp_channel_set_value(&reqValue);
  moveTime(10000);
  EXPECT_NEAR(*rsCfg->position, 100 + (20 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (0 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+70; // position
  reqValue.value[1] = -1; // tilt
  supla_esp_channel_set_value(&reqValue);
  moveTime(10000);
  EXPECT_NEAR(*rsCfg->position, 100 + (70 * 100), 50);
  EXPECT_NEAR(*rsCfg->tilt, 100 + (0 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+100; // position
  reqValue.value[1] = 10; // tilt values 0-9 are ignored and converted to -1
  supla_esp_channel_set_value(&reqValue);
  moveTime(4000);
  EXPECT_EQ(*rsCfg->position, 100 + (100 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (0 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+100; // position
  reqValue.value[1] = 1; // tilt values 0-9 are ignored and converted to -1
  supla_esp_channel_set_value(&reqValue);
  moveTime(7000);
  EXPECT_EQ(*rsCfg->position, 100 + (100 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (0 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+100; // position
  reqValue.value[1] = 2; // tilt values 0-9 are ignored and converted to -1
  supla_esp_channel_set_value(&reqValue);
  moveTime(7000);
  EXPECT_EQ(*rsCfg->position, 100 + (100 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (0 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

  reqValue.value[0] = 10+100; // position
  reqValue.value[1] = 9; // tilt values 0-9 are ignored and converted to -1
  supla_esp_channel_set_value(&reqValue);
  moveTime(7000);
  EXPECT_EQ(*rsCfg->position, 100 + (100 * 100));
  EXPECT_NEAR(*rsCfg->tilt, 100 + (0 * 100), 60);
  EXPECT_FALSE(eagleStub.getGpioValue(UP_GPIO));
  EXPECT_FALSE(eagleStub.getGpioValue(DOWN_GPIO));

}
