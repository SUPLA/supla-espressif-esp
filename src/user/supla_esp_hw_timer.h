/*
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

#ifndef SUPLA_ESP_HW_TIMER_H_
#define SUPLA_ESP_HW_TIMER_H_

#include "supla_esp.h"

typedef void (*_supla_esp_hw_timer_callback)(void);

#define FRC1_ENABLE_TIMER  BIT7
#define FRC1_AUTO_LOAD  BIT6

typedef enum {
    FRC1_SOURCE = 0,
    NMI_SOURCE = 1,
} FRC1_TIMER_SOURCE_TYPE;

typedef enum {
    DIVDED_BY_1 = 0,            //timer clock
    DIVDED_BY_16 = 4,   //divided by 16
    DIVDED_BY_256 = 8,  //divided by 256
} TIMER_PREDIVED_MODE;

typedef enum {                  //timer interrupt mode
    TM_LEVEL_INT = 1,   // level interrupt
    TM_EDGE_INT   = 0,  //edge interrupt
} TIMER_INT_MODE;

#define US_TO_RTC_TIMER_TICKS(t)          \
    ((t) ?                                   \
     (((t) > 0x35A) ?                   \
      (((t)>>2) * ((APB_CLK_FREQ>>4)/250000) + ((t)&0x3) * ((APB_CLK_FREQ>>4)/1000000))  :    \
      (((t) *(APB_CLK_FREQ>>4)) / 1000000)) :    \
     0)


#define REG_READ(_r) (*(volatile uint32 *)(_r))
#define WDEV_NOW() REG_READ(0x3ff20c00)

void supla_esp_hw_timer_init(FRC1_TIMER_SOURCE_TYPE source_type, u8 req, _supla_esp_hw_timer_callback callback);
void supla_esp_hw_timer_arm(u32 val);

#endif