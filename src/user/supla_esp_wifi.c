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

#include "supla_esp_wifi.h"

#include <osapi.h>
#include <string.h>

#include "supla-dev/log.h"
#include "supla_esp_cfg.h"
#include "supla_esp_gpio.h"
#include "supla_esp_state.h"

#define WIFI_CHECK_STATUS_INTERVAL_MS 200

typedef struct {
  ETSTimer timer;
  _wifi_void_status status_cb;
  uint8 last_status;
} _supla_esp_wifi_vars_t;

_supla_esp_wifi_vars_t supla_esp_wifi_vars = {};

void ICACHE_FLASH_ATTR supla_esp_wifi_init(void) {}

void ICACHE_FLASH_ATTR supla_esp_wifi_check_status(void *ptr) {
  uint8 status = wifi_station_get_connect_status();

  if (supla_esp_wifi_vars.last_status == status) {
    return;
  }

  supla_log(LOG_DEBUG, "WiFi Status: %i", status);

  supla_esp_wifi_vars.last_status = status;

  if (STATION_GOT_IP == status) {
    supla_esp_gpio_state_ipreceived();
  } else {
    switch (status) {
      case STATION_NO_AP_FOUND: {
        char buffer[WIFI_SSID_MAXSIZE + 40];
        ets_snprintf(buffer, sizeof(buffer),
                     "WiFi Network &quot;%s&quot; Not found",
                     supla_esp_cfg.WIFI_SSID);
        supla_esp_set_state(LOG_NOTICE, buffer);
      } break;
      case STATION_WRONG_PASSWORD:
        supla_esp_set_state(LOG_NOTICE, "WiFi - Wrong password");
        break;
    }

    supla_esp_gpio_state_disconnected();
  }

  if (supla_esp_wifi_vars.status_cb) {
    supla_esp_wifi_vars.status_cb(status);
  }
}

void ICACHE_FLASH_ATTR
supla_esp_wifi_station_connect(_wifi_void_status status_cb) {
  supla_esp_wifi_vars.status_cb = status_cb;

  supla_esp_wifi_vars.last_status = STATION_GOT_IP + 1;
  supla_esp_wifi_check_status(NULL);

  supla_esp_set_state(LOG_NOTICE, "WiFi - Connecting...");

  wifi_station_disconnect();

  supla_esp_gpio_state_disconnected();

  struct station_config stationConf;
  memset(&stationConf, 0, sizeof(struct station_config));

  wifi_set_opmode(STATION_MODE);

#ifdef WIFI_SLEEP_DISABLE
  wifi_set_sleep_type(NONE_SLEEP_T);
#endif

  os_memcpy(stationConf.ssid, supla_esp_cfg.WIFI_SSID, WIFI_SSID_MAXSIZE);
  os_memcpy(stationConf.password, supla_esp_cfg.WIFI_PWD, WIFI_PWD_MAXSIZE);

  stationConf.ssid[31] = 0;
  stationConf.password[63] = 0;
  stationConf.threshold.rssi = -127;

  wifi_station_set_config(&stationConf);
  wifi_station_set_auto_connect(1);

  wifi_station_connect();

  os_timer_disarm(&supla_esp_wifi_vars.timer);
  os_timer_setfn(&supla_esp_wifi_vars.timer,
                 (os_timer_func_t *)supla_esp_wifi_check_status, NULL);
  os_timer_arm(&supla_esp_wifi_vars.timer, WIFI_CHECK_STATUS_INTERVAL_MS, 1);
}

void ICACHE_FLASH_ATTR supla_esp_wifi_station_disconnect(void) {
  supla_esp_gpio_state_disconnected();
  os_timer_disarm(&supla_esp_wifi_vars.timer);
  supla_esp_wifi_check_status(NULL);
}
