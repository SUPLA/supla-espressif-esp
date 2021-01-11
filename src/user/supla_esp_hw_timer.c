/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: hw_timer.c
 *
 * Description: hw_timer driver
 *
 * Modification history:
 *     2014/5/1, v1.0 create this file.
 *******************************************************************************/

#include "supla_esp_hw_timer.h"

void supla_esp_hw_timer_init(FRC1_TIMER_SOURCE_TYPE source_type, u8 req,
                             _supla_esp_hw_timer_callback callback,
                             _supla_esp_hw_timer_nmi_callback nmi_callback) {
  if (req == 1) {
    RTC_REG_WRITE(FRC1_CTRL_ADDRESS, FRC1_AUTO_LOAD | DIVDED_BY_16 |
                                         FRC1_ENABLE_TIMER | TM_EDGE_INT);
  } else {
    RTC_REG_WRITE(FRC1_CTRL_ADDRESS,
                  DIVDED_BY_16 | FRC1_ENABLE_TIMER | TM_EDGE_INT);
  }

  if (source_type == NMI_SOURCE) {
    ETS_FRC_TIMER1_NMI_INTR_ATTACH(nmi_callback);
  } else {
    ETS_FRC_TIMER1_INTR_ATTACH(callback, NULL);
  }

  TM1_EDGE_INT_ENABLE();
  ETS_FRC1_INTR_ENABLE();
}

void supla_esp_hw_timer_arm(u32 val) {
  RTC_REG_WRITE(FRC1_LOAD_ADDRESS, US_TO_RTC_TIMER_TICKS(val));
}

void supla_esp_hw_timer_disarm(void) { RTC_REG_WRITE(FRC1_CTRL_ADDRESS, 0); }
