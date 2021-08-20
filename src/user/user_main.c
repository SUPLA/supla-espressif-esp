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

#include <ip_addr.h>
#include <mem.h>
#include <osapi.h>
#include <stdio.h>
#include <string.h>
#include <user_interface.h>

#include "supla-dev/log.h"
#include "supla_dht.h"
#include "supla_ds18b20.h"
#include "supla_esp.h"
#include "supla_esp_cfg.h"
#include "supla_esp_cfgmode.h"
#include "supla_esp_countdown_timer.h"
#include "supla_esp_devconn.h"
#include "supla_esp_dns_client.h"
#include "supla_esp_electricity_meter.h"
#include "supla_esp_gpio.h"
#include "supla_esp_impulse_counter.h"
#include "supla_esp_pwm.h"
#include "supla_esp_wifi.h"
#include "uptime.h"

#ifdef MQTT_SUPPORT_ENABLED
#include "supla_esp_mqtt.h"
#endif

#include "board/supla_esp_board.c"

#if ((SPI_FLASH_SIZE_MAP == 0) || (SPI_FLASH_SIZE_MAP == 1))
#error "The flash map is not supported"
#elif (SPI_FLASH_SIZE_MAP == 2)
#define SYSTEM_PARTITION_OTA_SIZE 0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR 0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR 0xfb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR 0xfc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR 0xfd000
#define SYSTEM_PARTITION_AT_PARAMETER_ADDR 0x7d000
#define SYSTEM_PARTITION_SSL_CLIENT_CERT_PRIVKEY_ADDR 0x7c000
#define SYSTEM_PARTITION_SSL_CLIENT_CA_ADDR 0x7b000
#define SYSTEM_PARTITION_WPA2_ENTERPRISE_CERT_PRIVKEY_ADDR 0x7a000
#define SYSTEM_PARTITION_WPA2_ENTERPRISE_CA_ADDR 0x79000
#elif (SPI_FLASH_SIZE_MAP == 3)
#error "The flash map is not supported"
#elif (SPI_FLASH_SIZE_MAP == 4)
#error "The flash map is not supported"
#elif (SPI_FLASH_SIZE_MAP == 5)
#define SYSTEM_PARTITION_OTA_SIZE 0xE0000
#define SYSTEM_PARTITION_OTA_2_ADDR 0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR 0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR 0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR 0x1fd000
#define SYSTEM_PARTITION_AT_PARAMETER_ADDR 0xfd000
#define SYSTEM_PARTITION_SSL_CLIENT_CERT_PRIVKEY_ADDR 0xfc000
#define SYSTEM_PARTITION_SSL_CLIENT_CA_ADDR 0xfb000
#define SYSTEM_PARTITION_WPA2_ENTERPRISE_CERT_PRIVKEY_ADDR 0xfa000
#define SYSTEM_PARTITION_WPA2_ENTERPRISE_CA_ADDR 0xf9000
#elif (SPI_FLASH_SIZE_MAP == 6)
#define SYSTEM_PARTITION_OTA_SIZE 0xE0000
#define SYSTEM_PARTITION_OTA_2_ADDR 0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR 0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR 0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR 0x3fd000
#define SYSTEM_PARTITION_AT_PARAMETER_ADDR 0xfd000
#define SYSTEM_PARTITION_SSL_CLIENT_CERT_PRIVKEY_ADDR 0xfc000
#define SYSTEM_PARTITION_SSL_CLIENT_CA_ADDR 0xfb000
#define SYSTEM_PARTITION_WPA2_ENTERPRISE_CERT_PRIVKEY_ADDR 0xfa000
#define SYSTEM_PARTITION_WPA2_ENTERPRISE_CA_ADDR 0xf9000
#else
#error "The flash map is not supported"
#endif

static const partition_item_t at_partition_table[] = {
    {SYSTEM_PARTITION_BOOTLOADER, 0x0, 0x1000},
    {SYSTEM_PARTITION_OTA_1, 0x1000, SYSTEM_PARTITION_OTA_SIZE},
    {SYSTEM_PARTITION_OTA_2, SYSTEM_PARTITION_OTA_2_ADDR,
     SYSTEM_PARTITION_OTA_SIZE},
    {SYSTEM_PARTITION_RF_CAL, SYSTEM_PARTITION_RF_CAL_ADDR, 0x1000},
    {SYSTEM_PARTITION_PHY_DATA, SYSTEM_PARTITION_PHY_DATA_ADDR, 0x1000},
    {SYSTEM_PARTITION_SYSTEM_PARAMETER, SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR,
     0x3000},
    {SYSTEM_PARTITION_AT_PARAMETER, SYSTEM_PARTITION_AT_PARAMETER_ADDR, 0x3000},
    {SYSTEM_PARTITION_SSL_CLIENT_CERT_PRIVKEY,
     SYSTEM_PARTITION_SSL_CLIENT_CERT_PRIVKEY_ADDR, 0x1000},
    {SYSTEM_PARTITION_SSL_CLIENT_CA, SYSTEM_PARTITION_SSL_CLIENT_CA_ADDR,
     0x1000},
#ifdef CONFIG_AT_WPA2_ENTERPRISE_COMMAND_ENABLE
    {SYSTEM_PARTITION_WPA2_ENTERPRISE_CERT_PRIVKEY,
     SYSTEM_PARTITION_WPA2_ENTERPRISE_CERT_PRIVKEY_ADDR, 0x1000},
    {SYSTEM_PARTITION_WPA2_ENTERPRISE_CA,
     SYSTEM_PARTITION_WPA2_ENTERPRISE_CA_ADDR, 0x1000},
#endif
};

#ifdef __FOTA
#include "supla_update.h"
#endif

ETSTimer system_restart_delay_timer;

void ICACHE_FLASH_ATTR supla_system_restart(void) {
#ifdef BOARD_IS_RESTART_ALLOWED
  if (supla_esp_board_is_restart_allowed() == 0) {
    return;
  }
#endif

#ifndef COUNTDOWN_TIMER_DISABLED
  supla_esp_save_state(0);
#endif /*COUNTDOWN_TIMER_DISABLED*/

#ifdef MQTT_SUPPORT_ENABLED
  if (supla_esp_cfg.Flags & CFG_FLAG_MQTT_ENABLED) {
    supla_esp_mqtt_before_system_restart();
  } else {
    supla_esp_devconn_before_system_restart();
  }
#else
  supla_esp_devconn_before_system_restart();
#endif /*MQTT_SUPPORT_ENABLED*/

#ifdef BOARD_BEFORE_REBOOT
  supla_esp_board_before_reboot();
#endif

  supla_log(LOG_DEBUG, "RESTART");
  supla_log(LOG_DEBUG, "Free heap size: %i", system_get_free_heap_size());

  system_restart();
}

void ICACHE_FLASH_ATTR supla_system_restart_with_delay_cb(void *ptr) {
  supla_system_restart();
}

void ICACHE_FLASH_ATTR supla_system_restart_with_delay(uint32 delay_ms) {
  os_timer_disarm(&system_restart_delay_timer);
  os_timer_setfn(&system_restart_delay_timer,
                 (os_timer_func_t *)supla_system_restart_with_delay_cb, NULL);
  os_timer_arm(&system_restart_delay_timer, delay_ms, 0);
}

uint32 MAIN_ICACHE_FLASH user_rf_cal_sector_set(void) {
  enum flash_size_map size_map = system_get_flash_size_map();
  uint32 rf_cal_sec = 0;

  switch (size_map) {
    case FLASH_SIZE_4M_MAP_256_256:
      rf_cal_sec = 128 - 5;
      break;

    case FLASH_SIZE_8M_MAP_512_512:
      rf_cal_sec = 256 - 5;
      break;

    case FLASH_SIZE_16M_MAP_512_512:
    case FLASH_SIZE_16M_MAP_1024_1024:
      rf_cal_sec = 512 - 5;
      break;

    case FLASH_SIZE_32M_MAP_512_512:
    case FLASH_SIZE_32M_MAP_1024_1024:
      rf_cal_sec = 1024 - 5;
      break;

    default:
      rf_cal_sec = 0;
      break;
  }

  return rf_cal_sec;
}

void MAIN_ICACHE_FLASH user_rf_pre_init(){};

void MAIN_ICACHE_FLASH user_pre_init(void) {
  if (!system_partition_table_regist(
          at_partition_table,
          sizeof(at_partition_table) / sizeof(at_partition_table[0]),
          SPI_FLASH_SIZE_MAP)) {
    os_printf("system_partition_table_regist fail\r\n");
  }
}

void MAIN_ICACHE_FLASH user_init(void) {
  supla_esp_uptime_init();

#ifdef BOARD_USER_INIT
  BOARD_USER_INIT;
#else

  struct rst_info *rtc_info = system_get_rst_info();
  supla_log(LOG_DEBUG, "RST reason: %i", rtc_info->reason);

  // setting DHCP hostname
  char hostname[32] = {};
  // hostname max length is 32, but it has to end with \0
  supla_esp_cfgmode_generate_ssid_name(hostname, sizeof(hostname) - 1);
  wifi_station_set_hostname(hostname);

  system_soft_wdt_restart();

  wifi_status_led_uninstall();
  supla_esp_cfg_init();

#ifndef COUNTDOWN_TIMER_DISABLED
  supla_esp_countdown_timer_init();
#endif /*COUNTDOWN_TIMER_DISABLED*/

  supla_esp_gpio_init();
  supla_esp_wifi_init();

  supla_log(LOG_DEBUG, "Starting %i", system_get_time());

#ifdef BOARD_ESP_STARTING
  BOARD_ESP_STARTING;
#endif

#if NOSSL == 1
  supla_log(LOG_DEBUG, "NO SSL!");
#endif

#ifdef __FOTA
  supla_esp_update_init();
#endif

#ifndef ADDITIONAL_DNS_CLIENT_DISABLED
  supla_esp_dns_client_init();
#endif /*ADDITIONAL_DNS_CLIENT_DISABLED*/

#ifdef MQTT_SUPPORT_ENABLED
  if (supla_esp_cfg.Flags & CFG_FLAG_MQTT_ENABLED) {
    supla_esp_mqtt_init();
  } else {
    supla_esp_devconn_init();
  }
#else
  supla_esp_devconn_init();
#endif /*MQTT_SUPPORT_ENABLED*/

#ifdef DS18B20
  supla_ds18b20_init();
#endif

#ifdef DHTSENSOR
  supla_dht_init();
#endif

#ifdef SUPLA_PWM_COUNT
  supla_esp_pwm_init();
#endif

#ifdef ELECTRICITY_METER_COUNT
  supla_esp_em_init();
#endif

#ifdef IMPULSE_COUNTER_COUNT
  supla_esp_ic_init();
#endif

#ifdef MQTT_SUPPORT_ENABLED
  if (supla_esp_cfg.WIFI_SSID[0] == 0 || supla_esp_cfg.WIFI_PWD[0] == 0 ||
      (supla_esp_cfg.Flags & CFG_FLAG_MQTT_ENABLED &&
       (supla_esp_cfg.Server[0] == 0 ||
        (!(supla_esp_cfg.Flags & CFG_FLAG_MQTT_NO_AUTH) &&
         (supla_esp_cfg.Username[0] == 0 ||
          supla_esp_cfg.Password[0] == 0)))) ||
      (!(supla_esp_cfg.Flags & CFG_FLAG_MQTT_ENABLED) &&
       (supla_esp_cfg.Server[0] == 0 || supla_esp_cfg.Email[0] == 0))) {
    supla_esp_cfgmode_start();
    return;
  }
#else
  if (((supla_esp_cfg.LocationID == 0 || supla_esp_cfg.LocationPwd[0] == 0) &&
       supla_esp_cfg.Email[0] == 0) ||
      supla_esp_cfg.Server[0] == 0 || supla_esp_cfg.WIFI_PWD[0] == 0 ||
      supla_esp_cfg.WIFI_SSID[0] == 0) {
    supla_esp_cfgmode_start();
    return;
  }
#endif /*MQTT_SUPPORT_ENABLED*/

#ifdef DS18B20
  supla_ds18b20_start();
#endif

#ifdef DHTSENSOR
  supla_dht_start();
#endif

#ifdef ELECTRICITY_METER_COUNT
  supla_esp_em_start();
#endif

#ifdef IMPULSE_COUNTER_COUNT
  supla_esp_ic_start();
#endif

#ifdef MQTT_SUPPORT_ENABLED
  if (supla_esp_cfg.Flags & CFG_FLAG_MQTT_ENABLED) {
    supla_esp_mqtt_client_start();
  } else {
    supla_esp_devconn_start();
  }
#else
  supla_esp_devconn_start();
#endif /*MQTT_SUPPORT_ENABLED*/

  system_print_meminfo();

#ifdef BOARD_ESP_STARTED
  BOARD_ESP_STARTED;
#endif

#endif /*BOARD_USER_INIT*/
}
