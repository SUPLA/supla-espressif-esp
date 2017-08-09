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

#include "supla_esp_hw_timer.h"


void supla_esp_hw_timer_init(FRC1_TIMER_SOURCE_TYPE source_type, u8 req, _supla_esp_hw_timer_callback callback)
{
    if (req == 1) {
        RTC_REG_WRITE(FRC1_CTRL_ADDRESS,
                      FRC1_AUTO_LOAD | DIVDED_BY_16 | FRC1_ENABLE_TIMER | TM_EDGE_INT);
    } else {
        RTC_REG_WRITE(FRC1_CTRL_ADDRESS,
                      DIVDED_BY_16 | FRC1_ENABLE_TIMER | TM_EDGE_INT);
    }

    if (source_type == NMI_SOURCE) {
        ETS_FRC_TIMER1_NMI_INTR_ATTACH(callback);
    } else {
        ETS_FRC_TIMER1_INTR_ATTACH(callback, NULL);
    }

    TM1_EDGE_INT_ENABLE();
    ETS_FRC1_INTR_ENABLE();
}

void  supla_esp_hw_timer_arm(u32 val)
{
    RTC_REG_WRITE(FRC1_LOAD_ADDRESS, US_TO_RTC_TIMER_TICKS(val));
}
