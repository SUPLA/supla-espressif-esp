/*
 Copyright (C) AC SOFTWARE SP. Z O.O.

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

#include "supla_esp_impulse_counter.h"

#include <eagle_soc.h>
#include <ets_sys.h>
#include <os_type.h>
#include <osapi.h>

#include "supla-dev/log.h"
#include "supla_esp_devconn.h"
#include "supla_esp_mqtt.h"

#ifdef IMPULSE_COUNTER_COUNT

ETSTimer supla_ic_timer1;
TDS_ImpulseCounter_Value last_icv[IMPULSE_COUNTER_COUNT];

void ICACHE_FLASH_ATTR supla_esp_ic_on_timer(void *ptr) {
#ifdef MQTT_SUPPORT_ENABLED
  if (!supla_esp_devconn_is_registered() &&
      !supla_esp_mqtt_server_connected()) {
    return;
  }
#else
  if (!supla_esp_devconn_is_registered()) {
    return;
  }
}
#endif /*MQTT_SUPPORT_ENABLED*/

  unsigned char channel_number = 0;
  char value[SUPLA_CHANNELVALUE_SIZE];

  TDS_ImpulseCounter_Value icv;
  memset(&icv, 0, sizeof(TDS_ImpulseCounter_Value));

  while (channel_number < IMPULSE_COUNTER_COUNT) {
    memcpy(&icv, &last_icv[channel_number], sizeof(TDS_ImpulseCounter_Value));
    if (supla_esp_board_get_impulse_counter(channel_number, &icv) == 1 &&
        memcmp(&last_icv[channel_number], &icv,
               sizeof(TDS_ImpulseCounter_Value)) != 0) {
      memset(value, 0, SUPLA_CHANNELVALUE_SIZE);
      memcpy(value, &icv, sizeof(TDS_ImpulseCounter_Value));
      memcpy(&last_icv[channel_number], &icv, sizeof(TDS_ImpulseCounter_Value));
      supla_esp_channel_value__changed(channel_number, value);

      // supla_log(LOG_DEBUG, "Value changed %i",
      // last_icv[channel_number].counter);
    }

    channel_number++;
  }
}

void ICACHE_FLASH_ATTR supla_esp_ic_init(void) {
  memset(last_icv, 0, sizeof(TDS_ImpulseCounter_Value) * IMPULSE_COUNTER_COUNT);
}

void ICACHE_FLASH_ATTR supla_esp_ic_start(void) {
  supla_esp_ic_set_measurement_frequency(5000);
}

void ICACHE_FLASH_ATTR supla_esp_ic_stop(void) {
  os_timer_disarm(&supla_ic_timer1);
}

void ICACHE_FLASH_ATTR supla_esp_ic_device_registered(void) {
  supla_esp_ic_on_timer(NULL);
}

void ICACHE_FLASH_ATTR supla_esp_ic_get_value(
    unsigned char channel_number, char value[SUPLA_CHANNELVALUE_SIZE]) {
  TDS_ImpulseCounter_Value icv;
  memset(&icv, 0, sizeof(TDS_ImpulseCounter_Value));

  if (supla_esp_board_get_impulse_counter(channel_number, &icv) == 1) {
    memset(value, 0, SUPLA_CHANNELVALUE_SIZE);
    memcpy(value, &icv, sizeof(TDS_ImpulseCounter_Value));
    // supla_log(LOG_DEBUG, "GET VALUE: %i", icv.counter);
  }
}

void ICACHE_FLASH_ATTR supla_esp_ic_set_measurement_frequency(int freq) {
  os_timer_disarm(&supla_ic_timer1);
  if (freq >= 1000) {
    os_timer_setfn(&supla_ic_timer1, (os_timer_func_t *)supla_esp_ic_on_timer,
                   NULL);
    os_timer_arm(&supla_ic_timer1, freq, 1);
  }
}

#endif /*IMPULSE_COUNTER_COUNT*/
