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

#ifndef SUPLA_ESP_H_
#define SUPLA_ESP_H_

#define MEMLEAK_DEBUG

#include "supla-dev/proto.h"
#include "board/supla_esp_board.h"
#include "espmissingincludes.h"

#define SUPLA_ESP_SOFTVER "2.5"

#define LO_VALUE  0
#define HI_VALUE  1

#define RELAY_INIT_VALUE LO_VALUE
#define SAVE_STATE_DELAY  1000

#ifndef INPUT_MAX_COUNT
#define INPUT_MAX_COUNT         7
#endif /*INPUT_MAX_COUNT*/

#ifndef RELAY_MAX_COUNT
#define RELAY_MAX_COUNT         4
#endif /*RELAY_MAX_COUNT*/

#ifndef RS_MAX_COUNT
#define RS_MAX_COUNT            2
#endif /*RS_MAX_COUNT*/

#ifndef SMOOTH_MAX_COUNT
#define SMOOTH_MAX_COUNT        1
#endif /*SMOOTH_MAX_COUNT*/

#define INPUT_FLAG_PULLUP             0x01
#define INPUT_FLAG_CFG_BTN            0x02
#define INPUT_FLAG_FACTORY_RESET      0x04

#define INPUT_TYPE_SENSOR        1
#define INPUT_TYPE_BUTTON        2
#define INPUT_TYPE_BUTTON_HILO   3
#define INPUT_TYPE_SWITCH        4
#define INPUT_TYPE_CUSTOM        200

// milliseconds
#ifndef RS_SWITCH_DELAY
#define RS_SWITCH_DELAY 1000
#endif

// microseconds
#ifndef RELAY_MIN_DELAY
#define RELAY_MIN_DELAY 100000
#endif

#ifndef MAIN_ICACHE_FLASH
#define MAIN_ICACHE_FLASH ICACHE_FLASH_ATTR
#endif

#ifndef GPIO_ICACHE_FLASH
#define GPIO_ICACHE_FLASH ICACHE_FLASH_ATTR
#endif

#ifndef DEVCONN_ICACHE_FLASH
#define DEVCONN_ICACHE_FLASH ICACHE_FLASH_ATTR
#endif

#ifndef CFG_ICACHE_FLASH_ATTR
#define CFG_ICACHE_FLASH_ATTR  ICACHE_FLASH_ATTR
#endif

#ifndef DHT_ICACHE_FLASH
#define DHT_ICACHE_FLASH ICACHE_FLASH_ATTR
#endif

void supla_esp_board_set_device_name(char *buffer, uint8 buffer_size);
void supla_esp_board_set_channels(TDS_SuplaDeviceChannel_B *channels, unsigned char *channel_count);
void supla_esp_board_relay_before_change_state(void);
void supla_esp_board_relay_after_change_state(void);
void supla_esp_board_gpio_init(void);



#ifdef __FOTA

#define RSA_NUM_BYTES 512
#define RSA_PUBLIC_EXPONENT 65537

extern const uint8_t rsa_public_key_bytes[RSA_NUM_BYTES];
#endif


#ifndef GPIO_PORT_INIT
#define GPIO_PORT_INIT \
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0); \
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4); \
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5); \
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12); \
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13); \
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14)
#endif

// PWM ----------------------------------

#ifndef PWM_0_OUT_IO_MUX
	#define PWM_0_OUT_IO_MUX PERIPHS_IO_MUX_GPIO4_U
	#define PWM_0_OUT_IO_NUM 4
	#define PWM_0_OUT_IO_FUNC  FUNC_GPIO4
#endif

#ifndef PWM_1_OUT_IO_MUX
	#define PWM_1_OUT_IO_MUX PERIPHS_IO_MUX_GPIO5_U
	#define PWM_1_OUT_IO_NUM 5
	#define PWM_1_OUT_IO_FUNC  FUNC_GPIO5
#endif

#ifndef PWM_2_OUT_IO_MUX
	#define PWM_2_OUT_IO_MUX PERIPHS_IO_MUX_MTDI_U
	#define PWM_2_OUT_IO_NUM 12
	#define PWM_2_OUT_IO_FUNC  FUNC_GPIO12
#endif

#ifndef PWM_3_OUT_IO_MUX
	#define PWM_3_OUT_IO_MUX PERIPHS_IO_MUX_MTCK_U
	#define PWM_3_OUT_IO_NUM 13
	#define PWM_3_OUT_IO_FUNC  FUNC_GPIO13
#endif

#ifndef PWM_4_OUT_IO_MUX
	#define PWM_4_OUT_IO_MUX PERIPHS_IO_MUX_MTMS_U
	#define PWM_4_OUT_IO_NUM 14
	#define PWM_4_OUT_IO_FUNC  FUNC_GPIO14
#endif

#ifndef PWM_PERIOD
  #define PWM_PERIOD 1000
#endif

// --------------------------------------

#ifndef AP_SSID
	#ifdef ESP8285
		#define AP_SSID "SUPLA-ESP8285"
	#else
		#define AP_SSID "SUPLA-ESP8266"
	#endif
#endif

#define SPI_FLASH_SEC_SIZE  4096
#define SERVER_MAXSIZE      100
#define WIFI_SSID_MAXSIZE   32
#define WIFI_PWD_MAXSIZE    64

#define STATE_MAXSIZE       200

#define RECVBUFF_MAXSIZE  1024

#define ACTIVITY_TIMEOUT 10

#ifndef WATCHDOG_TIMEOUT
#define WATCHDOG_TIMEOUT 60000000
#endif /*WATCHDOG_TIMEOUT*/


#endif /* SUPLA_ESP_H_ */
