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

#ifdef __FOTA

#include <string.h>
#include <stdio.h>

#include <user_interface.h>
#include <espconn.h>
#include <spi_flash.h>
#include <osapi.h>
#include <mem.h>
#include <upgrade.h>

#include "supla_update.h"

#include "supla_esp_devconn.h"
#include "supla_esp_gpio.h"
#include "supla_esp_cfg.h"
#include "supla-dev/srpc.h"
#include "supla-dev/log.h"

#include "nettle/sha2.h"
#include "nettle/rsa.h"
#include "nettle/bignum.h"

#define FUPDT_STEP_NONE            0
#define FUPDT_STEP_CHECK           1
#define FUPDT_STEP_CHECKING        2
#define FUPDT_STEP_DOWNLOADING     3

#define MAX_HTTP_HEADER_SIZE 700
#define MAX_FLASH_ATTEMPTS     5


typedef struct {


	TSD_FirmwareUpdate_UrlResult *url;

	char *http_header_data;
	int http_header_data_len;
	char http_header_matched;
	int expected_file_size;
	int downloaded_data_size;

	struct espconn conn;
	esp_tcp tcp;
	ip_addr_t ipaddr;

	uint32 flash_addr;
	uint32 flash_awo; // FLASH ADDRESS WITH OFFSET

	char *buff;
	int buff_pos;

	ETSTimer delay_timer;

}update_params;

static update_params *update = NULL;
static char update_step;
static unsigned int update_checking_start_time = 0;

void supla_esp_update_init(void) {
	
	update = NULL;

	if ( supla_esp_cfg.FirmwareUpdate == 1 ) {
		update_step = FUPDT_STEP_CHECK;

	} else {
		update_step = FUPDT_STEP_NONE;
	}

}

void ICACHE_FLASH_ATTR
supla_esp_update_reboot(char uf_finish) {

	if ( update->http_header_data != NULL ) {
		free(update->http_header_data);
		update->http_header_data = NULL;

	}

	if ( update->buff != NULL ) {
		free(update->buff);
		update->buff = NULL;
	}

	if ( uf_finish ) {

		system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
		system_upgrade_reboot();

	} else {

		system_upgrade_flag_set(UPGRADE_FLAG_IDLE);
		supla_system_restart();
	}



}

void ICACHE_FLASH_ATTR
supla_esp_check_updates(void *srpc) {
	
	supla_log(LOG_DEBUG, "supla_esp_check_updates");
	if ( update_step == FUPDT_STEP_CHECK ) {
		update_step = FUPDT_STEP_CHECKING;
		update_checking_start_time = system_get_time();
		supla_log(LOG_DEBUG, "if - supla_esp_check_updates");

		supla_esp_cfg.FirmwareUpdate = 0;
		supla_esp_cfg_save(&supla_esp_cfg);		
	
		TDS_FirmwareUpdateParams params;
		memset(&params, 0, sizeof(TDS_FirmwareUpdateParams));


		params.Platform = SUPLA_PLATFORM_ESP8266;
		params.Param1 = system_get_flash_size_map();
		params.Param2 = system_upgrade_userbin_check();

		//supla_log(LOG_DEBUG, "get_firmware_update_url");
		srpc_sd_async_get_firmware_update_url(srpc, &params);
	}
}

char ICACHE_FLASH_ATTR supla_esp_update_flash_write(void) {

	int a;

	uint32 sector = update->flash_awo/SPI_FLASH_SEC_SIZE;

	for(a=0;a<MAX_FLASH_ATTEMPTS;a++)
		if ( SPI_FLASH_RESULT_OK == spi_flash_erase_sector(sector)
			 && SPI_FLASH_RESULT_OK == spi_flash_write(update->flash_awo, (uint32_t *)update->buff, update->buff_pos) )
			break;



	if ( a>=MAX_FLASH_ATTEMPTS ) {

		supla_log(LOG_DEBUG, "WRITE AT %X (sector:%i) FAILED!", update->flash_awo, sector);

		system_upgrade_flag_set(UPGRADE_FLAG_IDLE);
		supla_esp_update_reboot(0);

		return 0;
	}

	update->flash_awo+=update->buff_pos;
	update->buff_pos = 0;

	return 1;
}

void ICACHE_FLASH_ATTR
supal_esp_update_download(char *content, unsigned short content_len) {

	if ( update->buff == NULL ) {
		update->buff = (char *)malloc(SPI_FLASH_SEC_SIZE);
	}

	unsigned short content_offset = 0;

	while(content_len > 0) {

		unsigned short len =  content_len;

		if ( len+update->buff_pos > SPI_FLASH_SEC_SIZE )
			len = SPI_FLASH_SEC_SIZE-update->buff_pos;

		content_len-=len;

		memcpy(&update->buff[update->buff_pos], &content[content_offset], len);
		update->buff_pos+=len;
		content_offset+=len;

		if ( update->buff_pos == SPI_FLASH_SEC_SIZE ) {

			if ( supla_esp_update_flash_write() == 0 )
				return;

		}

		update->downloaded_data_size+=len;

	}

	if ( update->buff_pos > 0 && update->downloaded_data_size == update->expected_file_size ) {

		if ( supla_esp_update_flash_write() == 0 )
			return;

	}

}

void ICACHE_FLASH_ATTR
supla_esp_update_verify_and_reboot (void) {

	//https://github.com/jkent/opensonoff-firmware/blob/f5d0bc5f8147e93f80b59dbe2b88617fb31de8ec/src/ota.c

	char verified = 0;
	struct sha256_ctx hash;
	struct rsa_public_key public_key;
	mpz_t signature;
	uint32_t key_bytes = 0;

	uint8_t footer[16];
	memset(footer, 0, 16);

	supla_log(LOG_DEBUG, "update->downloaded_data_size=%i", update->downloaded_data_size);

	if ( update->downloaded_data_size > 16 + RSA_NUM_BYTES )
		spi_flash_read(update->flash_awo - 16, (uint32_t *)footer, 16);

	if ( footer[0] != 0xBA || footer[1] != 0xBE || footer[2] != 0x2B ||
		 footer[3] != 0xED || footer[4] != 0 || footer[5] != 1) {

		key_bytes = RSA_NUM_BYTES-1;

	} else {
		key_bytes = (footer[6] << 8) - footer[7];
	}

	//supla_log(LOG_DEBUG, "CONTENT");

	if ( key_bytes == RSA_NUM_BYTES ) {

		sha256_init(&hash);

		int bytes_left = update->flash_awo-update->flash_addr-16-key_bytes;
		update->flash_awo = update->flash_addr;

		while(bytes_left > 0) {

			update->buff_pos = SPI_FLASH_SEC_SIZE;

			if ( update->buff_pos > bytes_left )
				update->buff_pos = bytes_left;

			if ( SPI_FLASH_RESULT_OK != spi_flash_read(update->flash_awo, (uint32*)update->buff, update->buff_pos) ) {

				supla_log(LOG_DEBUG, "READ ERROR at %X", update->flash_awo);
				break;
			}

			supla_log(LOG_DEBUG, "%i, %i, %i", bytes_left, (int)update->buff[0], (int)update->buff[update->buff_pos-1]);

			sha256_update(&hash, update->buff_pos, (unsigned char *)update->buff);

			bytes_left-=update->buff_pos;
			update->flash_awo+=update->buff_pos;
		}

		rsa_public_key_init(&public_key);
		nettle_mpz_set_str_256_u(public_key.n, RSA_NUM_BYTES, rsa_public_key_bytes);
		mpz_set_ui(public_key.e, RSA_PUBLIC_EXPONENT);
		rsa_public_key_prepare(&public_key);

		mpz_init(signature);

		if ( SPI_FLASH_RESULT_OK != spi_flash_read(update->flash_awo, (uint32_t *)update->buff, RSA_NUM_BYTES) ) {

			supla_log(LOG_DEBUG, "READ ERROR at %X", update->flash_awo);

		} else {

			nettle_mpz_init_set_str_256_u(signature, RSA_NUM_BYTES, (uint8_t *)update->buff);
			verified = !!rsa_sha256_verify(&public_key, &hash, signature);

		}


		mpz_clear(signature);
		rsa_public_key_clear(&public_key);
	}



	supla_log(LOG_DEBUG, "FINISH... %i", verified);
	supla_esp_update_reboot(verified);

}

void ICACHE_FLASH_ATTR
supla_esp_update_recv_cb (void *arg, char *pdata, unsigned short len) {

	int a;

	if ( len == 0 )
		return;

	if ( update->http_header_matched == 0 ) {

		if ( update->http_header_data == NULL ) {

			supla_log(LOG_DEBUG, "Free heap size: %i", system_get_free_heap_size());

			update->http_header_data = (char*)malloc(MAX_HTTP_HEADER_SIZE);
			update->http_header_data_len = 0;
		}

		for(a=0;a<len;a++) {

			if ( update->http_header_data_len >= MAX_HTTP_HEADER_SIZE-1  ) {
				update->http_header_matched = -1;
				break;
			}


			update->http_header_data[update->http_header_data_len] = pdata[a];
			update->http_header_data_len++;

			if ( update->http_header_data_len > 3
				 && update->http_header_data[update->http_header_data_len-1] == '\n'
				 && update->http_header_data[update->http_header_data_len-2] == '\r'
				 && update->http_header_data[update->http_header_data_len-3] == '\n'
				 && update->http_header_data[update->http_header_data_len-4] == '\r' ) {

				update->http_header_data[MAX_HTTP_HEADER_SIZE-1] = 0;
				update->http_header_matched=1;

				char *str;

				if ( NULL != strstr(update->http_header_data, "HTTP/1.1 200 OK")
					 && NULL != strstr(update->http_header_data, "Content-Type: application/octet-stream")
					 && NULL != (str = strstr(update->http_header_data, "Content-Length: ")) ) {

					int pos = (int)str - (int)update->http_header_data;
					pos+=16;

					supla_log(LOG_DEBUG, "%s", update->http_header_data);

					for(a=pos;a<update->http_header_data_len;a++) {

							if ( update->http_header_data[a] != '\r' && update->http_header_data[a] != '\n' ) {

								if ( update->http_header_data[a] < '0'
								     || update->http_header_data[a] > '9' )
								break;


								update->expected_file_size = (update->expected_file_size<<3)
										             +(update->expected_file_size<<1)+update->http_header_data[a]
										             -'0';
							}

							if ( update->http_header_data[a] == '\r'
								 || update->http_header_data[a] == '\n' ) {

								if ( update->expected_file_size > 0 ) {

									update->downloaded_data_size = 0;

									switch(system_get_flash_size_map()) {
										case FLASH_SIZE_4M_MAP_256_256:
										case FLASH_SIZE_2M:
											break;
										case FLASH_SIZE_8M_MAP_512_512:
										case FLASH_SIZE_16M_MAP_512_512:
										case FLASH_SIZE_32M_MAP_512_512:

											if ( update->expected_file_size <= 1024*492 )
												update_step = FUPDT_STEP_DOWNLOADING;

											break;

										case FLASH_SIZE_16M_MAP_1024_1024:
										case FLASH_SIZE_32M_MAP_1024_1024:

											if ( update->expected_file_size <= 1024*1004 )
												update_step = FUPDT_STEP_DOWNLOADING;

											break;
									}
								}
								break;
							}
					}

				}

				break;
			}

		}

		if ( update->http_header_matched != 0
			 && update->http_header_data != NULL ) {

				free(update->http_header_data);
				update->http_header_data = NULL;

		}

		if ( update_step == FUPDT_STEP_DOWNLOADING ) {
			system_upgrade_flag_set(UPGRADE_FLAG_START);
			supla_log(LOG_DEBUG, "WRITE ADDR: %X", update->flash_awo);
		}

	};

	if ( update_step == FUPDT_STEP_DOWNLOADING ) {

		supla_log(LOG_DEBUG, "FUPDT_STEP_DOWNLOADING, %i, %i", update->downloaded_data_size, update->expected_file_size);

		supal_esp_update_download(&pdata[update->http_header_data_len], len-update->http_header_data_len);
		update->http_header_data_len=0;

		if ( update->downloaded_data_size == update->expected_file_size ) {
			supla_esp_update_verify_and_reboot();
		}
	}

}

void ICACHE_FLASH_ATTR
supla_esp_update_disconnect_cb(void *arg){

	if (  update_step != FUPDT_STEP_DOWNLOADING ) {
		supla_log(LOG_DEBUG, "UPDATE STEP: %i", update_step);
		supla_esp_update_reboot(0);
	}


}

static void ICACHE_FLASH_ATTR
supla_esp_update_reconnect_cb(void *arg, int8_t err)
{
	supla_esp_update_disconnect_cb(arg);
}

void ICACHE_FLASH_ATTR
supla_esp_update_connect_cb(void *arg) {

	char request[SUPLA_URL_PATH_MAXSIZE+SUPLA_URL_HOST_MAXSIZE+110];

	ets_snprintf(request,
			     SUPLA_URL_PATH_MAXSIZE+SUPLA_URL_HOST_MAXSIZE+110,
                 "GET /%s HTTP/1.1\r\nUser-Agent: ESP8266\r\nHost: %s\r\nAccept: */*\r\nConnection: Keep-Alive\r\n\r\n",
			     update->url->url.path,
			     update->url->url.host);

	/*int s =*/ espconn_sent(&update->conn, (unsigned char*)request, strlen(request));
	//supla_log(LOG_DEBUG, "espconn_sent %i, %i", s, strlen(request));
}

void ICACHE_FLASH_ATTR
supla_esp_update_dns_found_cb(const char *name, ip_addr_t *ip, void *arg) {

	if ( ip == NULL ) {
		supla_log(LOG_NOTICE, "Domain not found. (update)");
		return;
	}


	update->conn.proto.tcp = &update->tcp;
	update->conn.type = ESPCONN_TCP;
	update->conn.state = ESPCONN_NONE;

	memcpy(update->conn.proto.tcp->remote_ip, ip, 4);

	update->conn.proto.tcp->local_port = espconn_port();
	update->conn.proto.tcp->remote_port = update->url->url.port;

	espconn_regist_recvcb(&update->conn, supla_esp_update_recv_cb);
	espconn_regist_connectcb(&update->conn, supla_esp_update_connect_cb);
	espconn_regist_disconcb(&update->conn, supla_esp_update_disconnect_cb);
	espconn_regist_reconcb(&update->conn, supla_esp_update_reconnect_cb);

	espconn_connect(&update->conn);


}

void ICACHE_FLASH_ATTR
supla_esp_update_resolvandconnect(void) {

	uint32_t _ip = ipaddr_addr(update->url->url.host);

	if ( _ip == -1 ) {
		 supla_log(LOG_DEBUG, "Resolv %s", update->url->url.host);
		 espconn_gethostbyname(&update->conn, update->url->url.host, &update->ipaddr, supla_esp_update_dns_found_cb);
	} else {
		 supla_esp_update_dns_found_cb(update->url->url.host, (ip_addr_t *)&_ip, NULL);
	}


}

void ICACHE_FLASH_ATTR
supla_esp_update_delay_timer_func(void *timer_arg) {
	supla_esp_update_resolvandconnect();
}

void ICACHE_FLASH_ATTR
supla_esp_update_url_result(TSD_FirmwareUpdate_UrlResult *url_result) {
	
	if ( update_step != FUPDT_STEP_CHECKING ) return;
	
	supla_log(LOG_DEBUG, "Firmware -- exists = %i, host = %s, port = %i, path = %s", url_result->exists, url_result->url.host, url_result->url.port, url_result->url.path);

	if ( url_result->exists
         && url_result->url.available_protocols & SUPLA_URL_PROTO_HTTP ) {

		if ( update == NULL ) {
			update = (update_params*)malloc(sizeof(update_params));
			memset(update, 0, sizeof(update_params));
		}


		int ubin = system_upgrade_userbin_check();

		switch(system_get_flash_size_map()) {
		    case FLASH_SIZE_4M_MAP_256_256:
		    case FLASH_SIZE_2M:

		        // UPDATE NOT SUPPORTED

		        free(update);
		        update = NULL;
		        return;

			case FLASH_SIZE_8M_MAP_512_512:
			case FLASH_SIZE_16M_MAP_512_512:
			case FLASH_SIZE_32M_MAP_512_512:
				update->flash_addr = ubin == UPGRADE_FW_BIN1 ? 0x81000 : 0x01000;
				break;
			case FLASH_SIZE_16M_MAP_1024_1024:
			case FLASH_SIZE_32M_MAP_1024_1024:
				update->flash_addr = ubin == UPGRADE_FW_BIN1 ? 0x101000 : 0x01000;
				break;
		}

		update->flash_awo = update->flash_addr;

		update->url = (TSD_FirmwareUpdate_UrlResult*)malloc(sizeof(TSD_FirmwareUpdate_UrlResult));
		memcpy(update->url, url_result, sizeof(TSD_FirmwareUpdate_UrlResult));

		supla_esp_gpio_state_update();
		supla_esp_devconn_before_update_start();
		supla_esp_devconn_stop();

		os_timer_disarm(&update->delay_timer);
		os_timer_setfn(&update->delay_timer, supla_esp_update_delay_timer_func, NULL);
		os_timer_arm (&update->delay_timer, 2000, false);

	}
	
}

char ICACHE_FLASH_ATTR
supla_esp_update_started(void) {
	if ( update_checking_start_time > 0 &&
			system_get_time() - update_checking_start_time < UPDATE_TIMEOUT &&
			update_step >= FUPDT_STEP_CHECKING ) {
		return 1;
	}

	return 0;
}

#endif
