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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include <ip_addr.h>
#include <user_interface.h>
#include <espconn.h>
#include <spi_flash.h>
#include <osapi.h>
#include <mem.h>

#include "supla_esp.h"
#include "supla_esp_cfg.h"
#include "supla_esp_cfgmode.h"
#include "supla_esp_devconn.h"
#include "supla_esp_gpio.h"
#include "supla_esp_state.h"

#include "supla-dev/log.h"

#define TYPE_UNKNOWN  0
#define TYPE_GET      1
#define TYPE_POST     2

#define STEP_TYPE        0
#define STEP_GET         2
#define STEP_POST        3
#define STEP_PARSE_VARS  4
#define STEP_DONE        10

#define VAR_NONE         0
#define VAR_SID          1
#define VAR_WPW          2
#define VAR_SVR          3
#define VAR_LID          4
#define VAR_PWD          5
#define VAR_CFGBTN       6
#define VAR_BTN1         7
#define VAR_BTN2         8
#define VAR_ICF          9
#define VAR_LED          10
#define VAR_UPD          11
#define VAR_RBT          12
#define VAR_EML          20
#define VAR_USD          21
#define VAR_TRG          22
#define VAR_CMD          23

#define VAR_PRO 24 // Protocol
#define VAR_MVR 25 // MQTT Server
#define VAR_PRT 26 // Port
#define VAR_TLS 27 // TLS
#define VAR_USR 28 // Username
#define VAR_MWD 29 // MQTT Password
#define VAR_PFX 30 // Topic prefix
#define VAR_QOS 31 // MQTT QoS
#define VAR_RET 32 // MQTT No Retain

#ifdef CFG_TIME_VARIABLES
#define VAR_T10          33
#define VAR_T11          34
#define VAR_T20          35
#define VAR_T21          36
#endif /*CFG_TIME_VARIABLES*/



typedef struct {
	
	char step;
	char type;
	char current_var;
	
	short matched;
	
	char *pbuff;
	int buff_size;
	int offset;
	char intval[12];
	
}TrivialHttpParserVars;


unsigned int supla_esp_cfgmode_entertime = 0;
ETSTimer http_response_timer;
uint32 http_response_buff_len = NULL;
uint32 http_response_pos = 0;
char *http_response_buff = 0;

void ICACHE_FLASH_ATTR 
supla_esp_http_send_response_cb(struct espconn *pespconn) {

	uint32 len = http_response_buff_len - http_response_pos;
	if ( len > 200 ) {
		len = 200;
	}
	
	if ( len <= 0 ) {
		os_timer_disarm(&http_response_timer);
		return;
	}
	
	if ( 0 == espconn_sent(pespconn, (unsigned char*)&http_response_buff[http_response_pos], len) ) {
		http_response_pos+=len;
	}
	
	
}

void ICACHE_FLASH_ATTR 
supla_esp_http_send_response(struct espconn *pespconn, const char *code, const char *html) {
	
	os_timer_disarm(&http_response_timer);
	
	if ( http_response_buff ) {
		free(http_response_buff);
		http_response_buff = NULL;
	}
	
	int html_len = html != NULL ? strlen(html) : 0;
	char response[] = "HTTP/1.1 %s\r\nAccept-Ranges: bytes\r\nContent-Length: %i\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: close\r\n\r\n";
	

	http_response_buff_len = strlen(code)+strlen(response)+html_len+1;
	http_response_buff = os_malloc(http_response_buff_len);
	http_response_pos = 0;
	
	ets_snprintf(http_response_buff, http_response_buff_len, response, code, html_len);
	
	if ( html_len > 0 ) {
		memcpy(&http_response_buff[http_response_buff_len-html_len-1], html, html_len);
	}
	
	http_response_buff[http_response_buff_len-1] = 0;
	
	http_response_buff_len = strlen(http_response_buff);
	
	os_timer_setfn(&http_response_timer, (os_timer_func_t *)supla_esp_http_send_response_cb, pespconn);
	os_timer_arm (&http_response_timer, 10, true);
	
}

void ICACHE_FLASH_ATTR 
supla_esp_http_ok(struct espconn *pespconn, const char *html) {
	supla_esp_http_send_response(pespconn, "200 OK", html);
}

void ICACHE_FLASH_ATTR 
supla_esp_http_404(struct espconn *pespconn) {
	supla_esp_http_send_response(pespconn, "404 Not Found", "Not Found");
}

void ICACHE_FLASH_ATTR 
supla_esp_http_error(struct espconn *pespconn) {
	supla_esp_http_send_response(pespconn, "500 Internal Server Error", "Error");
}

int ICACHE_FLASH_ATTR
Power(int x, int y) {
	
    int result = 1;
    while (y)
    {
        if (y & 1)
            result *= x;
        
        y >>= 1;
        x *= x;
    }

    return result;
}

int ICACHE_FLASH_ATTR
HexToInt(char *str, int len) {

	int a, n, p;
	int result = 0;

	if ( len%2 != 0 )
		return 0;

	p = len - 1;

	for(a=0;a<len;a++) {
		n = 0;

		if ( str[a] >= 'A' && str[a] <= 'F' )
			n = str[a]-55;
		else if ( str[a] >= 'a' && str[a] <= 'f' )
			n = str[a]-87;
		else if ( str[a] >= '0' && str[a] <= '9' )
			n = str[a]-48;


		result+=Power(16, p)*n;
		p--;
	}

	return result;


};

int ICACHE_FLASH_ATTR cfg_str2int(TrivialHttpParserVars *pVars) {
	int result = 0;

	short s=0;
	while(pVars->intval[s]!=0) {

		if ( pVars->intval[s] >= '0' && pVars->intval[s] <= '9' ) {
			result = result*10 + pVars->intval[s] - '0';
		}

		s++;
	}

	return result;
}


void ICACHE_FLASH_ATTR
supla_esp_parse_request(TrivialHttpParserVars *pVars, char *pdata, unsigned short len, SuplaEspCfg *cfg, char *reboot) {
	
	if ( len == 0 ) 
		return;
	
	int a, p;
	
	//for(a=0;a<len;a++)
	//	printf("%c", pdata[a]);

	if ( pVars->step == STEP_TYPE ) {
		
		char get[] = "GET";
		char post[] = "POST";
		char url[] = " / HTTP";
		
		if ( len >= 3
			 && memcmp(pdata, get, 3) == 0
			 && len >= 10
			 && memcmp(&pdata[3], url, 7) == 0 ) {
			
			pVars->step = STEP_GET;
			pVars->type = TYPE_GET;
			
		} else if ( len >= 4
				 && memcmp(pdata, post, 4) == 0
				 && len >= 11
				 && memcmp(&pdata[4], url, 7) == 0 )  {
			
			pVars->step = STEP_POST;
			pVars->type = TYPE_POST;
		}
		
	}
	
	p = 0;
	
	if ( pVars->step == STEP_POST ) {
		
		char header_end[4] = { '\r', '\n', '\r', '\n' };
		
		for(a=p;a<len;a++) {
		
			if ( len-a >= 4
				 && memcmp(header_end, &pdata[a], 4) == 0 ) {

				pVars->step = STEP_PARSE_VARS;
				p+=3;
			}

		}
		
	}
	
	if ( pVars->step == STEP_PARSE_VARS ) {
		
		for(a=p;a<len;a++) {
		
			if ( pVars->current_var == VAR_NONE ) {
				
				char sid[3] = { 's', 'i', 'd' };
				char wpw[3] = { 'w', 'p', 'w' };
				char svr[3] = { 's', 'v', 'r' };
				char lid[3] = { 'l', 'i', 'd' };
				char pwd[3] = { 'p', 'w', 'd' };
				char btncfg[3] = { 'c', 'f', 'g' };
				char btn1[3] = { 'b', 't', '1' };
				char btn2[3] = { 'b', 't', '2' };
				char icf[3] = { 'i', 'c', 'f' };
				char led[3] = { 'l', 'e', 'd' };
				char upd[3] = { 'u', 'p', 'd' };
				char rbt[3] = { 'r', 'b', 't' };
				char eml[3] = { 'e', 'm', 'l' };
				char usd[3] = { 'u', 's', 'd' };
				char trg[3] = { 't', 'r', 'g' };
				char cmd[3] = { 'c', 'm', 'd' };

				char pro[3] = { 'p', 'r', 'o' };
				char mvr[3] = { 'm', 'v', 'r' };
				char prt[3] = { 'p', 'r', 't' };
				char tls[3] = { 't', 'l', 's' };
				char usr[3] = { 'u', 's', 'r' };
				char mwd[3] = { 'm', 'w', 'd' };
				char pfx[3] = { 'p', 'f', 'x' };
				char qos[3] = { 'q', 'o', 's' };
				char ret[3] = { 'r', 'e', 't' };


#ifdef CFG_TIME_VARIABLES
				char t10[3] = { 't', '1', '0' };
				char t11[3] = { 't', '1', '1' };
				char t20[3] = { 't', '2', '0' };
				char t21[3] = { 't', '2', '1' };
#endif /*CFG_TIME_VARIABLES*/
				
				if ( len-a >= 4
					 && pdata[a+3] == '=' ) {

					if ( memcmp(sid, &pdata[a], 3) == 0 ) {
						
						pVars->current_var = VAR_SID;
						pVars->buff_size = WIFI_SSID_MAXSIZE;
						pVars->pbuff = cfg->WIFI_SSID;
						
					} else if ( memcmp(wpw, &pdata[a], 3) == 0 ) {
						
						pVars->current_var = VAR_WPW;
						pVars->buff_size = WIFI_PWD_MAXSIZE;
						pVars->pbuff = cfg->WIFI_PWD;
						
					} else if ( memcmp(svr, &pdata[a], 3) == 0 ) {
						
						if (!(cfg->Flags & CFG_FLAG_MQTT_ENABLED)) {
							pVars->current_var = VAR_SVR;
							pVars->buff_size = SERVER_MAXSIZE;
							pVars->pbuff = cfg->Server;
						}
						
					} else if ( memcmp(lid, &pdata[a], 3) == 0 ) {
						
						pVars->current_var = VAR_LID;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;
						
					} else if ( memcmp(pwd, &pdata[a], 3) == 0 ) {
						
						pVars->current_var = VAR_PWD;
						pVars->buff_size = SUPLA_LOCATION_PWD_MAXSIZE;
						pVars->pbuff = cfg->LocationPwd;
						
					} else if ( memcmp(btncfg, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_CFGBTN;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

					} else if ( memcmp(btn1, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_BTN1;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

					} else if ( memcmp(btn2, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_BTN2;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

					} else if ( memcmp(icf, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_ICF;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

					} else if ( memcmp(led, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_LED;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

					} else if ( memcmp(upd, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_UPD;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

					} else if ( memcmp(rbt, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_RBT;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

					} else if ( memcmp(eml, &pdata[a], 3) == 0 ) {
						if (!(cfg->Flags & CFG_FLAG_MQTT_ENABLED)) {
							pVars->current_var = VAR_EML;
							pVars->buff_size = SUPLA_EMAIL_MAXSIZE;
							pVars->pbuff = cfg->Email;
						}
					} else if ( memcmp(usd, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_USD;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

				    } else if ( memcmp(trg, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_TRG;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

				    } else if ( memcmp(cmd, &pdata[a], 3) == 0 ) {

				    	if (user_cmd == NULL) {
				    		user_cmd = malloc(CMD_MAXSIZE);
				    	}

						pVars->current_var = VAR_CMD;
						pVars->buff_size = CMD_MAXSIZE;
						pVars->pbuff = user_cmd;

					} else if ( memcmp(pro, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_PRO;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

				    } else if ( memcmp(mvr, &pdata[a], 3) == 0 ) {

				    	if (cfg->Flags & CFG_FLAG_MQTT_ENABLED) {
							pVars->current_var = VAR_MVR;
							pVars->buff_size = SERVER_MAXSIZE;
							pVars->pbuff = cfg->Server;
				    	}

					} else if ( memcmp(prt, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_PRT;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

					} else if ( memcmp(tls, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_TLS;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

				    } else if ( memcmp(usr, &pdata[a], 3) == 0 ) {

				    	if (cfg->Flags & CFG_FLAG_MQTT_ENABLED) {
							pVars->current_var = VAR_USR;
							pVars->buff_size = SUPLA_EMAIL_MAXSIZE;
							pVars->pbuff = cfg->Username;
				    	}

				    } else if ( memcmp(mwd, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_MWD;
						pVars->buff_size = SUPLA_LOCATION_PWD_MAXSIZE;
						pVars->pbuff = cfg->Password;

				    } else if ( memcmp(pfx, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_PFX;
						pVars->buff_size = MQTT_PREFIX_SIZE;
						pVars->pbuff = cfg->MqttTopicPrefix;

					} else if ( memcmp(qos, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_QOS;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

					} else if ( memcmp(ret, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_RET;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

#ifndef CFG_TIME_VARIABLES
				    }
#else
				    } else if ( memcmp(t10, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_T10;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

				    } else if ( memcmp(t11, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_T11;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

				    } else if ( memcmp(t20, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_T20;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

				    } else if ( memcmp(t21, &pdata[a], 3) == 0 ) {

						pVars->current_var = VAR_T21;
						pVars->buff_size = 12;
						pVars->pbuff = pVars->intval;

				    }
#endif /*CFG_TIME_VARIABLES*/
					a+=4;
					pVars->offset = 0;
				}

			}
			
			if ( pVars->current_var != VAR_NONE ) {
				
				if ( pVars->offset < pVars->buff_size
					 && a < len
					 && pdata[a] != '&' ) {
					
					if ( pdata[a] == '%' && a+2 < len ) {
						
						pVars->pbuff[pVars->offset] = HexToInt(&pdata[a+1], 2);
						pVars->offset++;
						a+=2;
						
					} else if ( pdata[a] == '+' ) {

						pVars->pbuff[pVars->offset] = ' ';
						pVars->offset++;

					} else {

						pVars->pbuff[pVars->offset] = pdata[a];
						pVars->offset++;

					}
					
				}

				
				if ( pVars->offset >= pVars->buff_size
					  || a >= len-1
					  || pdata[a] == '&'  ) {
					
					if ( pVars->offset < pVars->buff_size )
						pVars->pbuff[pVars->offset] = 0;
					else
						pVars->pbuff[pVars->buff_size-1] = 0;
					
					
					if ( pVars->current_var == VAR_LID ) {
						
						cfg->LocationID = cfg_str2int(pVars);

					} else if ( pVars->current_var == VAR_CFGBTN ) {

						cfg->CfgButtonType = pVars->intval[0] - '0';

					} else if ( pVars->current_var == VAR_BTN1 ) {

						cfg->Button1Type = pVars->intval[0] - '0';

					} else if ( pVars->current_var == VAR_BTN2 ) {

						cfg->Button2Type = pVars->intval[0] - '0';

					} else if ( pVars->current_var == VAR_ICF ) {

						cfg->InputCfgTriggerOff = (pVars->intval[0] - '0') == 1 ? 1 : 0;

					} else if ( pVars->current_var == VAR_LED ) {

						cfg->StatusLedOff = (pVars->intval[0] - '0');

					} else if ( pVars->current_var == VAR_UPD ) {

						cfg->FirmwareUpdate = pVars->intval[0] - '0';

					} else if ( pVars->current_var == VAR_RBT ) {

						if ( reboot != NULL )
							*reboot = pVars->intval[0] - '0';

					} else if ( pVars->current_var == VAR_USD ) {

						cfg->UpsideDown = (pVars->intval[0] - '0') == 1 ? 1 : 0;

					} else if ( pVars->current_var == VAR_TRG ) {

						cfg->Trigger = pVars->intval[0] - '0';

					} else if ( pVars->current_var == VAR_PRO ) {
						if (pVars->intval[0] - '0' == 1) {
							cfg->Flags |= CFG_FLAG_MQTT_ENABLED;
						} else {
							cfg->Flags &= ~CFG_FLAG_MQTT_ENABLED;
						}

					} else if ( pVars->current_var == VAR_PRT ) {

						int port = cfg_str2int(pVars);
						if (port > 0 && port <= 65535) {
							 cfg->Port = port;
					    }

					} else if ( pVars->current_var == VAR_TLS ) {
						if (pVars->intval[0] - '0' == 1) {
							cfg->Flags |= CFG_FLAG_MQTT_TLS;
						} else {
							cfg->Flags &= ~CFG_FLAG_MQTT_TLS;
						}

					} else if ( pVars->current_var == VAR_QOS ) {

						int qos = pVars->intval[0] - '0';
						if (qos >= 0 && qos <= 2) {
							 cfg->MqttQoS = qos;
					    }

					} else if ( pVars->current_var == VAR_RET ) {
						if (pVars->intval[0] - '0' == 1) {
							cfg->Flags |= CFG_FLAG_MQTT_NO_RETAIN;
						} else {
							cfg->Flags &= ~CFG_FLAG_MQTT_NO_RETAIN;
						}

#ifndef CFG_TIME_VARIABLES
				    }
#else
					} else if ( pVars->current_var == VAR_T10 ) {

						cfg->Time1[0] = cfg_str2int(pVars);

					} else if ( pVars->current_var == VAR_T11 ) {

						cfg->Time1[1] = cfg_str2int(pVars);

					} else if ( pVars->current_var == VAR_T20 ) {

						cfg->Time2[0] = cfg_str2int(pVars);

					} else if ( pVars->current_var == VAR_T21 ) {

						cfg->Time2[1] = cfg_str2int(pVars);

					}
#endif /*CFG_TIME_VARIABLES*/
					pVars->matched++;
					pVars->current_var = VAR_NONE;

				}
				
			}
			
		}
				
	}
	
}

void ICACHE_FLASH_ATTR
supla_esp_recv_callback (void *arg, char *pdata, unsigned short len)
{	
	struct espconn *conn = (struct espconn *)arg;
	char mac[6];
	char data_saved = 0;
	char reboot = 0;
	
//	char pd[] = { 0x47, 0x45, 0x54, 0x20, 0x2F, 0x20, 0x48, 0x54, 0x54, 0x50, 0x2F, 0x31, 0x2E, 0x31, 0x0D, 0x0A, 0x48, 0x6F, 0x73, 0x74, 0x3A, 0x20, 0x31, 0x39, 0x32, 0x2E, 0x31, 0x36, 0x38, 0x2E, 0x34, 0x2E, 0x31, 0x0D, 0x0A, 0x43, 0x6F, 0x6E, 0x6E, 0x65, 0x63, 0x74, 0x69, 0x6F, 0x6E, 0x3A, 0x20, 0x6B, 0x65, 0x65, 0x70, 0x2D, 0x61, 0x6C, 0x69, 0x76, 0x65, 0x0D, 0x0A, 0x43, 0x61, 0x63, 0x68, 0x65, 0x2D, 0x43, 0x6F, 0x6E, 0x74, 0x72, 0x6F, 0x6C, 0x3A, 0x20, 0x6D, 0x61, 0x78, 0x2D, 0x61, 0x67, 0x65, 0x3D, 0x30, 0x0D, 0x0A, 0x55, 0x70, 0x67, 0x72, 0x61, 0x64, 0x65, 0x2D, 0x49, 0x6E, 0x73, 0x65, 0x63, 0x75, 0x72, 0x65, 0x2D, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x73, 0x3A, 0x20, 0x31, 0x0D, 0x0A, 0x55, 0x73, 0x65, 0x72, 0x2D, 0x41, 0x67, 0x65, 0x6E, 0x74, 0x3A, 0x20, 0x4D, 0x6F, 0x7A, 0x69, 0x6C, 0x6C, 0x61, 0x2F, 0x35, 0x2E, 0x30, 0x20, 0x28, 0x4C, 0x69, 0x6E, 0x75, 0x78, 0x3B, 0x20, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x20, 0x36, 0x2E, 0x30, 0x3B, 0x20, 0x48, 0x55, 0x41, 0x57, 0x45, 0x49, 0x20, 0x56, 0x4E, 0x53, 0x2D, 0x4C, 0x32, 0x31, 0x20, 0x42, 0x75, 0x69, 0x6C, 0x64, 0x2F, 0x48, 0x55, 0x41, 0x57, 0x45, 0x49, 0x56, 0x4E, 0x53, 0x2D, 0x4C, 0x32, 0x31, 0x29, 0x20, 0x41, 0x70, 0x70, 0x6C, 0x65, 0x57, 0x65, 0x62, 0x4B, 0x69, 0x74, 0x2F, 0x35, 0x33, 0x37, 0x2E, 0x33, 0x36, 0x20, 0x28, 0x4B, 0x48, 0x54, 0x4D, 0x4C, 0x2C, 0x20, 0x6C, 0x69, 0x6B, 0x65, 0x20, 0x47, 0x65, 0x63, 0x6B, 0x6F, 0x29, 0x20, 0x43, 0x68, 0x72, 0x6F, 0x6D, 0x65, 0x2F, 0x35, 0x38, 0x2E, 0x30, 0x2E, 0x33, 0x30, 0x32, 0x39, 0x2E, 0x38, 0x33, 0x20, 0x4D, 0x6F, 0x62, 0x69, 0x6C, 0x65, 0x20, 0x53, 0x61, 0x66, 0x61, 0x72, 0x69, 0x2F, 0x35, 0x33, 0x37, 0x2E, 0x33, 0x36, 0x0D, 0x0A, 0x41, 0x63, 0x63, 0x65, 0x70, 0x74, 0x3A, 0x20, 0x74, 0x65, 0x78, 0x74, 0x2F, 0x68, 0x74, 0x6D, 0x6C, 0x2C, 0x61, 0x70, 0x70, 0x6C, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6F, 0x6E, 0x2F, 0x78, 0x68, 0x74, 0x6D, 0x6C, 0x2B, 0x78, 0x6D, 0x6C, 0x2C, 0x61, 0x70, 0x70, 0x6C, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6F, 0x6E, 0x2F, 0x78, 0x6D, 0x6C, 0x3B, 0x71, 0x3D, 0x30, 0x2E, 0x39, 0x2C, 0x69, 0x6D, 0x61, 0x67, 0x65, 0x2F, 0x77, 0x65, 0x62, 0x70, 0x2C, 0x2A, 0x2F, 0x2A, 0x3B, 0x71, 0x3D, 0x30, 0x2E, 0x38, 0x0D, 0x0A, 0x41, 0x63, 0x63, 0x65, 0x70, 0x74, 0x2D, 0x45, 0x6E, 0x63, 0x6F, 0x64, 0x69, 0x6E, 0x67, 0x3A, 0x20, 0x67, 0x7A, 0x69, 0x70, 0x2C, 0x20, 0x64, 0x65, 0x66, 0x6C, 0x61, 0x74, 0x65, 0x2C, 0x20, 0x73, 0x64, 0x63, 0x68, 0x0D, 0x0A, 0x41, 0x63, 0x63, 0x65, 0x70, 0x74, 0x2D, 0x4C, 0x61, 0x6E, 0x67, 0x75, 0x61, 0x67, 0x65, 0x3A, 0x20, 0x70, 0x6C, 0x2D, 0x50, 0x4C, 0x2C, 0x70, 0x6C, 0x3B, 0x71, 0x3D, 0x30, 0x2E, 0x38, 0x2C, 0x65, 0x6E, 0x2D, 0x55, 0x53, 0x3B, 0x71, 0x3D, 0x30, 0x2E, 0x36, 0x2C, 0x65, 0x6E, 0x3B, 0x71, 0x3D, 0x30, 0x2E, 0x34, 0x0D, 0x0A, 0x0D, 0x0A, 0x00 }; 

//	len = strlen(pd);
//	pdata = pd;
	
	supla_log(LOG_DEBUG, "Free heap size: %i", system_get_free_heap_size());
	supla_log(LOG_DEBUG, "REQUEST LEN: %i", len);
	
	
	//unsigned short x;
	//for(x=0;x<len;x++)
	//	os_printf("[%d] 0x%02X ", x, (unsigned char)pdata[x]);

	
	TrivialHttpParserVars *pVars = (TrivialHttpParserVars *)conn->reverse;

	if ( pdata == NULL || pVars == NULL )
		return;
	
	SuplaEspCfg new_cfg;
	memcpy(&new_cfg, &supla_esp_cfg, sizeof(SuplaEspCfg));
	
	supla_esp_parse_request(pVars, pdata, len, &new_cfg, &reboot);
	
	if ( pVars->type == TYPE_UNKNOWN ) {
		
		supla_esp_http_404(conn);
		return;
	};
	
	if ( pVars->type == TYPE_POST ) {
		
		supla_log(LOG_DEBUG, "Matched: %i, step: %i", pVars->matched, pVars->step);

		if ( pVars->matched < 4 ) {
			return;
		}

		if ( reboot > 0 ) {
			if (reboot == 2) {
				supla_system_restart_with_delay(1500);
			} else {
				supla_system_restart();
				return;
			}
		}
				
		// This also works for cfg->Password (union)
		if ( new_cfg.LocationPwd[0] == 0 )
			memcpy(new_cfg.LocationPwd, supla_esp_cfg.LocationPwd, SUPLA_LOCATION_PWD_MAXSIZE);
		
		if ( new_cfg.WIFI_PWD[0] == 0 )
			memcpy(new_cfg.WIFI_PWD, supla_esp_cfg.WIFI_PWD, WIFI_PWD_MAXSIZE);
	    
		if ( 1 == supla_esp_cfg_save(&new_cfg) ) {
					
			memcpy(&supla_esp_cfg, &new_cfg, sizeof(SuplaEspCfg));
			data_saved = 1;
			
			#ifdef BOARD_ON_CFG_SAVED
			supla_esp_board_on_cfg_saved();
			#endif

		} else {
			supla_esp_http_error(conn);
		}
		
	}

	
	if ( false == wifi_get_macaddr(STATION_IF, (unsigned char*)mac) ) {
		supla_esp_http_error(conn);
		return;
	}
	
	
	char dev_name[25];
	supla_esp_board_set_device_name(dev_name, 25);
	dev_name[24] = 0;
	
	char *buffer = 0;

	#ifdef BOARD_CFG_HTML_TEMPLATE
	
	
	buffer = supla_esp_board_cfg_html_template(dev_name, mac, data_saved);
	    
	#else

	#ifdef CFGBTN_TYPE_SELECTION

		char html_template_header[] = "<!DOCTYPE html><meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no\"><style>body{font-size:14px;font-family:HelveticaNeue,\"Helvetica Neue\",HelveticaNeueRoman,HelveticaNeue-Roman,\"Helvetica Neue Roman\",TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:400;font-stretch:normal;background:#00d151;color:#fff;line-height:20px;padding:0}.s{width:460px;margin:0 auto;margin-top:calc(50vh - 340px);border:solid 3px #fff;padding:0 10px 10px;border-radius:3px}#l{display:block;max-width:150px;height:155px;margin:-80px auto 20px;background:#00d151;padding-right:5px}#l path{fill:#000}.w{margin:3px 0 16px;padding:5px 0;border-radius:3px;background:#fff;box-shadow:0 1px 3px rgba(0,0,0,.3)}h1,h3{margin:10px 8px;font-family:HelveticaNeueLight,HelveticaNeue-Light,\"Helvetica Neue Light\",HelveticaNeue,\"Helvetica Neue\",TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:300;font-stretch:normal;color:#000;font-size:23px}h1{margin-bottom:14px;color:#fff}span{display:block;margin:10px 7px 14px}i{display:block;font-style:normal;position:relative;border-bottom:solid 1px #00d151;height:42px}i:last-child{border:none}label{position:absolute;display:inline-block;top:0;left:8px;color:#00d151;line-height:41px;pointer-events:none}input,select{width:calc(100% - 145px);border:none;font-size:16px;line-height:40px;border-radius:0;letter-spacing:-.5px;background:#fff;color:#000;padding-left:144px;-webkit-appearance:none;-moz-appearance:none;appearance:none;outline:0!important;height:40px}select{padding:0;float:right;margin:1px 3px 1px 2px}button{width:100%;border:0;background:#000;padding:5px 10px;font-size:16px;line-height:40px;color:#fff;border-radius:3px;box-shadow:0 1px 3px rgba(0,0,0,.3);cursor:pointer}.c{background:#ffe836;position:fixed;width:100%;line-height:80px;color:#000;top:0;left:0;box-shadow:0 1px 3px rgba(0,0,0,.3);text-align:center;font-size:26px;z-index:100}@media all and (max-height:920px){.s{margin-top:80px}}@media all and (max-width:900px){.s{width:calc(100% - 20px);margin-top:40px;border:none;padding:0 8px;border-radius:0}#l{max-width:80px;height:auto;margin:10px auto 20px}h1,h3{font-size:19px}i{border:none;height:auto}label{display:block;margin:4px 0 12px;color:#00d151;font-size:13px;position:relative;line-height:18px}input,select{width:calc(100% - 10px);font-size:16px;line-height:28px;padding:0 5px;border-bottom:solid 1px #00d151}select{width:100%;float:none;margin:0}}</style><script type=\"text/javascript\">setTimeout(function(){var element =  document.getElementById('msg');if ( element != null ) element.style.visibility = \"hidden\";},3200);</script>";

		#ifdef __FOTA
		char html_template[] = "%s%s<div class=\"s\"><svg version=\"1.1\" id=\"l\" x=\"0\" y=\"0\" viewBox=\"0 0 200 200\" xml:space=\"preserve\"><path d=\"M59.3,2.5c18.1,0.6,31.8,8,40.2,23.5c3.1,5.7,4.3,11.9,4.1,18.3c-0.1,3.6-0.7,7.1-1.9,10.6c-0.2,0.7-0.1,1.1,0.6,1.5c12.8,7.7,25.5,15.4,38.3,23c2.9,1.7,5.8,3.4,8.7,5.3c1,0.6,1.6,0.6,2.5-0.1c4.5-3.6,9.8-5.3,15.7-5.4c12.5-0.1,22.9,7.9,25.2,19c1.9,9.2-2.9,19.2-11.8,23.9c-8.4,4.5-16.9,4.5-25.5,0.2c-0.7-0.3-1-0.2-1.5,0.3c-4.8,4.9-9.7,9.8-14.5,14.6c-5.3,5.3-10.6,10.7-15.9,16c-1.8,1.8-3.6,3.7-5.4,5.4c-0.7,0.6-0.6,1,0,1.6c3.6,3.4,5.8,7.5,6.2,12.2c0.7,7.7-2.2,14-8.8,18.5c-12.3,8.6-30.3,3.5-35-10.4c-2.8-8.4,0.6-17.7,8.6-22.8c0.9-0.6,1.1-1,0.8-2c-2-6.2-4.4-12.4-6.6-18.6c-6.3-17.6-12.7-35.1-19-52.7c-0.2-0.7-0.5-1-1.4-0.9c-12.5,0.7-23.6-2.6-33-10.4c-8-6.6-12.9-15-14.2-25c-1.5-11.5,1.7-21.9,9.6-30.7C32.5,8.9,42.2,4.2,53.7,2.7c0.7-0.1,1.5-0.2,2.2-0.2C57,2.4,58.2,2.5,59.3,2.5z M76.5,81c0,0.1,0.1,0.3,0.1,0.6c1.6,6.3,3.2,12.6,4.7,18.9c4.5,17.7,8.9,35.5,13.3,53.2c0.2,0.9,0.6,1.1,1.6,0.9c5.4-1.2,10.7-0.8,15.7,1.6c0.8,0.4,1.2,0.3,1.7-0.4c11.2-12.9,22.5-25.7,33.4-38.7c0.5-0.6,0.4-1,0-1.6c-5.6-7.9-6.1-16.1-1.3-24.5c0.5-0.8,0.3-1.1-0.5-1.6c-9.1-4.7-18.1-9.3-27.2-14c-6.8-3.5-13.5-7-20.3-10.5c-0.7-0.4-1.1-0.3-1.6,0.4c-1.3,1.8-2.7,3.5-4.3,5.1c-4.2,4.2-9.1,7.4-14.7,9.7C76.9,80.3,76.4,80.3,76.5,81z M89,42.6c0.1-2.5-0.4-5.4-1.5-8.1C83,23.1,74.2,16.9,61.7,15.8c-10-0.9-18.6,2.4-25.3,9.7c-8.4,9-9.3,22.4-2.2,32.4c6.8,9.6,19.1,14.2,31.4,11.9C79.2,67.1,89,55.9,89,42.6z M102.1,188.6c0.6,0.1,1.5-0.1,2.4-0.2c9.5-1.4,15.3-10.9,11.6-19.2c-2.6-5.9-9.4-9.6-16.8-8.6c-8.3,1.2-14.1,8.9-12.4,16.6C88.2,183.9,94.4,188.6,102.1,188.6z M167.7,88.5c-1,0-2.1,0.1-3.1,0.3c-9,1.7-14.2,10.6-10.8,18.6c2.9,6.8,11.4,10.3,19,7.8c7.1-2.3,11.1-9.1,9.6-15.9C180.9,93,174.8,88.5,167.7,88.5z\"/></svg><h1>%s</h1><span>LAST STATE: %s<br>Firmware: %s<br>GUID: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X<br>MAC: %02X:%02X:%02X:%02X:%02X:%02X</span><form method=\"post\"><div class=\"w\"><h3>Wi-Fi Settings</h3><i><input name=\"sid\" value=\"%s\"><label>Network name</label></i><i><input name=\"wpw\"><label>Password</label></i></div><div class=\"w\"><h3>Supla Settings</h3><i><input name=\"svr\" value=\"%s\"><label>Server</label></i><i><input name=\"eml\" value=\"%s\"><label>E-mail</label></i></div><div class=\"w\"><h3>Additional Settings</h3><i><select name=\"cfg\"><option value=\"0\" %s>button</option><option value=\"1\" %s>switch</option></select><label>Button type</label></i><i><select name=\"upd\"><option value=\"0\" %s>NO</option><option value=\"1\" %s>YES</option></select><label>Firmware update</label></i></div><button type=\"submit\">SAVE</button></form></div><br><br>";
		#else
		char html_template[] = "%s%s<div class=\"s\"><svg version=\"1.1\" id=\"l\" x=\"0\" y=\"0\" viewBox=\"0 0 200 200\" xml:space=\"preserve\"><path d=\"M59.3,2.5c18.1,0.6,31.8,8,40.2,23.5c3.1,5.7,4.3,11.9,4.1,18.3c-0.1,3.6-0.7,7.1-1.9,10.6c-0.2,0.7-0.1,1.1,0.6,1.5c12.8,7.7,25.5,15.4,38.3,23c2.9,1.7,5.8,3.4,8.7,5.3c1,0.6,1.6,0.6,2.5-0.1c4.5-3.6,9.8-5.3,15.7-5.4c12.5-0.1,22.9,7.9,25.2,19c1.9,9.2-2.9,19.2-11.8,23.9c-8.4,4.5-16.9,4.5-25.5,0.2c-0.7-0.3-1-0.2-1.5,0.3c-4.8,4.9-9.7,9.8-14.5,14.6c-5.3,5.3-10.6,10.7-15.9,16c-1.8,1.8-3.6,3.7-5.4,5.4c-0.7,0.6-0.6,1,0,1.6c3.6,3.4,5.8,7.5,6.2,12.2c0.7,7.7-2.2,14-8.8,18.5c-12.3,8.6-30.3,3.5-35-10.4c-2.8-8.4,0.6-17.7,8.6-22.8c0.9-0.6,1.1-1,0.8-2c-2-6.2-4.4-12.4-6.6-18.6c-6.3-17.6-12.7-35.1-19-52.7c-0.2-0.7-0.5-1-1.4-0.9c-12.5,0.7-23.6-2.6-33-10.4c-8-6.6-12.9-15-14.2-25c-1.5-11.5,1.7-21.9,9.6-30.7C32.5,8.9,42.2,4.2,53.7,2.7c0.7-0.1,1.5-0.2,2.2-0.2C57,2.4,58.2,2.5,59.3,2.5z M76.5,81c0,0.1,0.1,0.3,0.1,0.6c1.6,6.3,3.2,12.6,4.7,18.9c4.5,17.7,8.9,35.5,13.3,53.2c0.2,0.9,0.6,1.1,1.6,0.9c5.4-1.2,10.7-0.8,15.7,1.6c0.8,0.4,1.2,0.3,1.7-0.4c11.2-12.9,22.5-25.7,33.4-38.7c0.5-0.6,0.4-1,0-1.6c-5.6-7.9-6.1-16.1-1.3-24.5c0.5-0.8,0.3-1.1-0.5-1.6c-9.1-4.7-18.1-9.3-27.2-14c-6.8-3.5-13.5-7-20.3-10.5c-0.7-0.4-1.1-0.3-1.6,0.4c-1.3,1.8-2.7,3.5-4.3,5.1c-4.2,4.2-9.1,7.4-14.7,9.7C76.9,80.3,76.4,80.3,76.5,81z M89,42.6c0.1-2.5-0.4-5.4-1.5-8.1C83,23.1,74.2,16.9,61.7,15.8c-10-0.9-18.6,2.4-25.3,9.7c-8.4,9-9.3,22.4-2.2,32.4c6.8,9.6,19.1,14.2,31.4,11.9C79.2,67.1,89,55.9,89,42.6z M102.1,188.6c0.6,0.1,1.5-0.1,2.4-0.2c9.5-1.4,15.3-10.9,11.6-19.2c-2.6-5.9-9.4-9.6-16.8-8.6c-8.3,1.2-14.1,8.9-12.4,16.6C88.2,183.9,94.4,188.6,102.1,188.6z M167.7,88.5c-1,0-2.1,0.1-3.1,0.3c-9,1.7-14.2,10.6-10.8,18.6c2.9,6.8,11.4,10.3,19,7.8c7.1-2.3,11.1-9.1,9.6-15.9C180.9,93,174.8,88.5,167.7,88.5z\"/></svg><h1>%s</h1><span>LAST STATE: %s<br>Firmware: %s<br>GUID: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X<br>MAC: %02X:%02X:%02X:%02X:%02X:%02X</span><form method=\"post\"><div class=\"w\"><h3>Wi-Fi Settings</h3><i><input name=\"sid\" value=\"%s\"><label>Network name</label></i><i><input name=\"wpw\"><label>Password</label></i></div><div class=\"w\"><h3>Supla Settings</h3><i><input name=\"svr\" value=\"%s\"><label>Server</label></i><i><input name=\"eml\" value=\"%s\"><label>E-mail</label></i></div><div class=\"w\"><h3>Additional Settings</h3><i><select name=\"cfg\"><option value=\"0\" %s>button</option><option value=\"1\" %s>switch</option></select><label>Button type</label></i></div><button type=\"submit\">SAVE</button></form></div><br><br>";
		#endif


		int bufflen = strlen(supla_esp_get_laststate())
					  +strlen(dev_name)
					  +strlen(SUPLA_ESP_SOFTVER)
					  +strlen(supla_esp_cfg.WIFI_SSID)
					  +strlen(supla_esp_cfg.Server)
					  +strlen(supla_esp_cfg.Email)
					  +strlen(html_template_header)
					  +strlen(html_template)
					  +200;

		buffer = (char*)malloc(bufflen);
		
		ets_snprintf(buffer,
				bufflen,
				html_template,
				html_template_header,
				data_saved == 1 ? "<div id=\"msg\" class=\"c\">Data saved</div>" : "",
				dev_name,
				supla_esp_get_laststate(),
				SUPLA_ESP_SOFTVER,
				(unsigned char)supla_esp_cfg.GUID[0],
				(unsigned char)supla_esp_cfg.GUID[1],
				(unsigned char)supla_esp_cfg.GUID[2],
				(unsigned char)supla_esp_cfg.GUID[3],
				(unsigned char)supla_esp_cfg.GUID[4],
				(unsigned char)supla_esp_cfg.GUID[5],
				(unsigned char)supla_esp_cfg.GUID[6],
				(unsigned char)supla_esp_cfg.GUID[7],
				(unsigned char)supla_esp_cfg.GUID[8],
				(unsigned char)supla_esp_cfg.GUID[9],
				(unsigned char)supla_esp_cfg.GUID[10],
				(unsigned char)supla_esp_cfg.GUID[11],
				(unsigned char)supla_esp_cfg.GUID[12],
				(unsigned char)supla_esp_cfg.GUID[13],
				(unsigned char)supla_esp_cfg.GUID[14],
				(unsigned char)supla_esp_cfg.GUID[15],
				(unsigned char)mac[0],
				(unsigned char)mac[1],
				(unsigned char)mac[2],
				(unsigned char)mac[3],
				(unsigned char)mac[4],
				(unsigned char)mac[5],
				supla_esp_cfg.WIFI_SSID,
				supla_esp_cfg.Server,
				supla_esp_cfg.Email,
				supla_esp_cfg.CfgButtonType == BTN_TYPE_MONOSTABLE ? "selected" : "",
				supla_esp_cfg.CfgButtonType == BTN_TYPE_BISTABLE ? "selected" : ""
				#ifdef __FOTA
				,
				supla_esp_cfg.FirmwareUpdate == 0 ? "selected" : "",
				supla_esp_cfg.FirmwareUpdate == 1 ? "selected" : ""
				#endif
				);


	#elif defined(BTN1_2_TYPE_SELECTION)
	

		char html_template_header[] = "<!DOCTYPE html><meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no\"><style>body{font-size:14px;font-family:HelveticaNeue,\"Helvetica Neue\",HelveticaNeueRoman,HelveticaNeue-Roman,\"Helvetica Neue Roman\",TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:400;font-stretch:normal;background:#00d151;color:#fff;line-height:20px;padding:0}.s{width:460px;margin:0 auto;margin-top:calc(50vh - 340px);border:solid 3px #fff;padding:0 10px 10px;border-radius:3px}#l{display:block;max-width:150px;height:155px;margin:-80px auto 20px;background:#00d151;padding-right:5px}#l path{fill:#000}.w{margin:3px 0 16px;padding:5px 0;border-radius:3px;background:#fff;box-shadow:0 1px 3px rgba(0,0,0,.3)}h1,h3{margin:10px 8px;font-family:HelveticaNeueLight,HelveticaNeue-Light,\"Helvetica Neue Light\",HelveticaNeue,\"Helvetica Neue\",TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:300;font-stretch:normal;color:#000;font-size:23px}h1{margin-bottom:14px;color:#fff}span{display:block;margin:10px 7px 14px}i{display:block;font-style:normal;position:relative;border-bottom:solid 1px #00d151;height:42px}i:last-child{border:none}label{position:absolute;display:inline-block;top:0;left:8px;color:#00d151;line-height:41px;pointer-events:none}input,select{width:calc(100% - 145px);border:none;font-size:16px;line-height:40px;border-radius:0;letter-spacing:-.5px;background:#fff;color:#000;padding-left:144px;-webkit-appearance:none;-moz-appearance:none;appearance:none;outline:0!important;height:40px}select{padding:0;float:right;margin:1px 3px 1px 2px}button{width:100%;border:0;background:#000;padding:5px 10px;font-size:16px;line-height:40px;color:#fff;border-radius:3px;box-shadow:0 1px 3px rgba(0,0,0,.3);cursor:pointer}.c{background:#ffe836;position:fixed;width:100%;line-height:80px;color:#000;top:0;left:0;box-shadow:0 1px 3px rgba(0,0,0,.3);text-align:center;font-size:26px;z-index:100}@media all and (max-height:920px){.s{margin-top:80px}}@media all and (max-width:900px){.s{width:calc(100% - 20px);margin-top:40px;border:none;padding:0 8px;border-radius:0}#l{max-width:80px;height:auto;margin:10px auto 20px}h1,h3{font-size:19px}i{border:none;height:auto}label{display:block;margin:4px 0 12px;color:#00d151;font-size:13px;position:relative;line-height:18px}input,select{width:calc(100% - 10px);font-size:16px;line-height:28px;padding:0 5px;border-bottom:solid 1px #00d151}select{width:100%;float:none;margin:0}}</style><script type=\"text/javascript\">setTimeout(function(){var element =  document.getElementById('msg');if ( element != null ) element.style.visibility = \"hidden\";},3200);</script>";

		#ifdef __FOTA
		char html_template[] = "%s%s<div class=\"s\"><svg version=\"1.1\" id=\"l\" x=\"0\" y=\"0\" viewBox=\"0 0 200 200\" xml:space=\"preserve\"><path d=\"M59.3,2.5c18.1,0.6,31.8,8,40.2,23.5c3.1,5.7,4.3,11.9,4.1,18.3c-0.1,3.6-0.7,7.1-1.9,10.6c-0.2,0.7-0.1,1.1,0.6,1.5c12.8,7.7,25.5,15.4,38.3,23c2.9,1.7,5.8,3.4,8.7,5.3c1,0.6,1.6,0.6,2.5-0.1c4.5-3.6,9.8-5.3,15.7-5.4c12.5-0.1,22.9,7.9,25.2,19c1.9,9.2-2.9,19.2-11.8,23.9c-8.4,4.5-16.9,4.5-25.5,0.2c-0.7-0.3-1-0.2-1.5,0.3c-4.8,4.9-9.7,9.8-14.5,14.6c-5.3,5.3-10.6,10.7-15.9,16c-1.8,1.8-3.6,3.7-5.4,5.4c-0.7,0.6-0.6,1,0,1.6c3.6,3.4,5.8,7.5,6.2,12.2c0.7,7.7-2.2,14-8.8,18.5c-12.3,8.6-30.3,3.5-35-10.4c-2.8-8.4,0.6-17.7,8.6-22.8c0.9-0.6,1.1-1,0.8-2c-2-6.2-4.4-12.4-6.6-18.6c-6.3-17.6-12.7-35.1-19-52.7c-0.2-0.7-0.5-1-1.4-0.9c-12.5,0.7-23.6-2.6-33-10.4c-8-6.6-12.9-15-14.2-25c-1.5-11.5,1.7-21.9,9.6-30.7C32.5,8.9,42.2,4.2,53.7,2.7c0.7-0.1,1.5-0.2,2.2-0.2C57,2.4,58.2,2.5,59.3,2.5z M76.5,81c0,0.1,0.1,0.3,0.1,0.6c1.6,6.3,3.2,12.6,4.7,18.9c4.5,17.7,8.9,35.5,13.3,53.2c0.2,0.9,0.6,1.1,1.6,0.9c5.4-1.2,10.7-0.8,15.7,1.6c0.8,0.4,1.2,0.3,1.7-0.4c11.2-12.9,22.5-25.7,33.4-38.7c0.5-0.6,0.4-1,0-1.6c-5.6-7.9-6.1-16.1-1.3-24.5c0.5-0.8,0.3-1.1-0.5-1.6c-9.1-4.7-18.1-9.3-27.2-14c-6.8-3.5-13.5-7-20.3-10.5c-0.7-0.4-1.1-0.3-1.6,0.4c-1.3,1.8-2.7,3.5-4.3,5.1c-4.2,4.2-9.1,7.4-14.7,9.7C76.9,80.3,76.4,80.3,76.5,81z M89,42.6c0.1-2.5-0.4-5.4-1.5-8.1C83,23.1,74.2,16.9,61.7,15.8c-10-0.9-18.6,2.4-25.3,9.7c-8.4,9-9.3,22.4-2.2,32.4c6.8,9.6,19.1,14.2,31.4,11.9C79.2,67.1,89,55.9,89,42.6z M102.1,188.6c0.6,0.1,1.5-0.1,2.4-0.2c9.5-1.4,15.3-10.9,11.6-19.2c-2.6-5.9-9.4-9.6-16.8-8.6c-8.3,1.2-14.1,8.9-12.4,16.6C88.2,183.9,94.4,188.6,102.1,188.6z M167.7,88.5c-1,0-2.1,0.1-3.1,0.3c-9,1.7-14.2,10.6-10.8,18.6c2.9,6.8,11.4,10.3,19,7.8c7.1-2.3,11.1-9.1,9.6-15.9C180.9,93,174.8,88.5,167.7,88.5z\"/></svg><h1>%s</h1><span>LAST STATE: %s<br>Firmware: %s<br>GUID: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X<br>MAC: %02X:%02X:%02X:%02X:%02X:%02X</span><form method=\"post\"><div class=\"w\"><h3>Wi-Fi Settings</h3><i><input name=\"sid\" value=\"%s\"><label>Network name</label></i><i><input name=\"wpw\"><label>Password</label></i></div><div class=\"w\"><h3>Supla Settings</h3><i><input name=\"svr\" value=\"%s\"><label>Server</label></i><i><input name=\"eml\" value=\"%s\"><label>E-mail</label></i></div><div class=\"w\"><h3>Additional Settings</h3><i><select name=\"bt1\"><option value=\"0\" %s>monostble</option><option value=\"1\" %s>bistable</option></select><label>Input1 type:</label></i><i><select name=\"bt2\"><option value=\"0\" %s>button</option><option value=\"1\" %s>switch</option></select><label>Input2 type:</label></i><i><select name=\"upd\"><option value=\"0\" %s>NO</option><option value=\"1\" %s>YES</option></select><label>Firmware update</label></i></div><button type=\"submit\">SAVE</button></form></div><br><br>";
		#else
		char html_template[] = "%s%s<div class=\"s\"><svg version=\"1.1\" id=\"l\" x=\"0\" y=\"0\" viewBox=\"0 0 200 200\" xml:space=\"preserve\"><path d=\"M59.3,2.5c18.1,0.6,31.8,8,40.2,23.5c3.1,5.7,4.3,11.9,4.1,18.3c-0.1,3.6-0.7,7.1-1.9,10.6c-0.2,0.7-0.1,1.1,0.6,1.5c12.8,7.7,25.5,15.4,38.3,23c2.9,1.7,5.8,3.4,8.7,5.3c1,0.6,1.6,0.6,2.5-0.1c4.5-3.6,9.8-5.3,15.7-5.4c12.5-0.1,22.9,7.9,25.2,19c1.9,9.2-2.9,19.2-11.8,23.9c-8.4,4.5-16.9,4.5-25.5,0.2c-0.7-0.3-1-0.2-1.5,0.3c-4.8,4.9-9.7,9.8-14.5,14.6c-5.3,5.3-10.6,10.7-15.9,16c-1.8,1.8-3.6,3.7-5.4,5.4c-0.7,0.6-0.6,1,0,1.6c3.6,3.4,5.8,7.5,6.2,12.2c0.7,7.7-2.2,14-8.8,18.5c-12.3,8.6-30.3,3.5-35-10.4c-2.8-8.4,0.6-17.7,8.6-22.8c0.9-0.6,1.1-1,0.8-2c-2-6.2-4.4-12.4-6.6-18.6c-6.3-17.6-12.7-35.1-19-52.7c-0.2-0.7-0.5-1-1.4-0.9c-12.5,0.7-23.6-2.6-33-10.4c-8-6.6-12.9-15-14.2-25c-1.5-11.5,1.7-21.9,9.6-30.7C32.5,8.9,42.2,4.2,53.7,2.7c0.7-0.1,1.5-0.2,2.2-0.2C57,2.4,58.2,2.5,59.3,2.5z M76.5,81c0,0.1,0.1,0.3,0.1,0.6c1.6,6.3,3.2,12.6,4.7,18.9c4.5,17.7,8.9,35.5,13.3,53.2c0.2,0.9,0.6,1.1,1.6,0.9c5.4-1.2,10.7-0.8,15.7,1.6c0.8,0.4,1.2,0.3,1.7-0.4c11.2-12.9,22.5-25.7,33.4-38.7c0.5-0.6,0.4-1,0-1.6c-5.6-7.9-6.1-16.1-1.3-24.5c0.5-0.8,0.3-1.1-0.5-1.6c-9.1-4.7-18.1-9.3-27.2-14c-6.8-3.5-13.5-7-20.3-10.5c-0.7-0.4-1.1-0.3-1.6,0.4c-1.3,1.8-2.7,3.5-4.3,5.1c-4.2,4.2-9.1,7.4-14.7,9.7C76.9,80.3,76.4,80.3,76.5,81z M89,42.6c0.1-2.5-0.4-5.4-1.5-8.1C83,23.1,74.2,16.9,61.7,15.8c-10-0.9-18.6,2.4-25.3,9.7c-8.4,9-9.3,22.4-2.2,32.4c6.8,9.6,19.1,14.2,31.4,11.9C79.2,67.1,89,55.9,89,42.6z M102.1,188.6c0.6,0.1,1.5-0.1,2.4-0.2c9.5-1.4,15.3-10.9,11.6-19.2c-2.6-5.9-9.4-9.6-16.8-8.6c-8.3,1.2-14.1,8.9-12.4,16.6C88.2,183.9,94.4,188.6,102.1,188.6z M167.7,88.5c-1,0-2.1,0.1-3.1,0.3c-9,1.7-14.2,10.6-10.8,18.6c2.9,6.8,11.4,10.3,19,7.8c7.1-2.3,11.1-9.1,9.6-15.9C180.9,93,174.8,88.5,167.7,88.5z\"/></svg><h1>%s</h1><span>LAST STATE: %s<br>Firmware: %s<br>GUID: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X<br>MAC: %02X:%02X:%02X:%02X:%02X:%02X</span><form method=\"post\"><div class=\"w\"><h3>Wi-Fi Settings</h3><i><input name=\"sid\" value=\"%s\"><label>Network name</label></i><i><input name=\"wpw\"><label>Password</label></i></div><div class=\"w\"><h3>Supla Settings</h3><i><input name=\"svr\" value=\"%s\"><label>Server</label></i><i><input name=\"eml\" value=\"%s\"><label>E-mail</label></i></div><div class=\"w\"><h3>Additional Settings</h3><i><select name=\"bt1\"><option value=\"0\" %s>monostable</option><option value=\"1\" %s>bistable</option></select><label>Input1 type:</label></i><i><select name=\"bt2\"><option value=\"0\" %s>button</option><option value=\"1\" %s>switch</option></select><label>Input2 type:</label></i></div><button type=\"submit\">SAVE</button></form></div><br><br>";
		#endif

		int bufflen = strlen(supla_esp_get_laststate())
					  +strlen(dev_name)
					  +strlen(SUPLA_ESP_SOFTVER)
					  +strlen(supla_esp_cfg.WIFI_SSID)
					  +strlen(supla_esp_cfg.Server)
					  +strlen(supla_esp_cfg.Email)
					  +strlen(html_template_header)
					  +strlen(html_template)
					  +200;

		buffer = (char*)malloc(bufflen);

		ets_snprintf(buffer,
				bufflen,
				html_template,
				html_template_header,
				data_saved == 1 ? "<div id=\"msg\" class=\"c\">Data saved</div>" : "",
				dev_name,
				supla_esp_get_laststate(),
				SUPLA_ESP_SOFTVER,
				(unsigned char)supla_esp_cfg.GUID[0],
				(unsigned char)supla_esp_cfg.GUID[1],
				(unsigned char)supla_esp_cfg.GUID[2],
				(unsigned char)supla_esp_cfg.GUID[3],
				(unsigned char)supla_esp_cfg.GUID[4],
				(unsigned char)supla_esp_cfg.GUID[5],
				(unsigned char)supla_esp_cfg.GUID[6],
				(unsigned char)supla_esp_cfg.GUID[7],
				(unsigned char)supla_esp_cfg.GUID[8],
				(unsigned char)supla_esp_cfg.GUID[9],
				(unsigned char)supla_esp_cfg.GUID[10],
				(unsigned char)supla_esp_cfg.GUID[11],
				(unsigned char)supla_esp_cfg.GUID[12],
				(unsigned char)supla_esp_cfg.GUID[13],
				(unsigned char)supla_esp_cfg.GUID[14],
				(unsigned char)supla_esp_cfg.GUID[15],
				(unsigned char)mac[0],
				(unsigned char)mac[1],
				(unsigned char)mac[2],
				(unsigned char)mac[3],
				(unsigned char)mac[4],
				(unsigned char)mac[5],
				supla_esp_cfg.WIFI_SSID,
				supla_esp_cfg.Server,
				supla_esp_cfg.Email,
				supla_esp_cfg.Button1Type == BTN_TYPE_MONOSTABLE ? "selected" : "",
				supla_esp_cfg.Button1Type == BTN_TYPE_BISTABLE ? "selected" : "",
				supla_esp_cfg.Button2Type == BTN_TYPE_MONOSTABLE ? "selected" : "",
				supla_esp_cfg.Button2Type == BTN_TYPE_BISTABLE ? "selected" : ""

				#ifdef __FOTA
				,
				supla_esp_cfg.FirmwareUpdate == 0 ? "selected" : "",
				supla_esp_cfg.FirmwareUpdate == 1 ? "selected" : ""
				#endif
				);

	
	#else
	
		char html_template_header[] = "<!DOCTYPE html><meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no\"><style>body{font-size:14px;font-family:HelveticaNeue,\"Helvetica Neue\",HelveticaNeueRoman,HelveticaNeue-Roman,\"Helvetica Neue Roman\",TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:400;font-stretch:normal;background:#00d151;color:#fff;line-height:20px;padding:0}.s{width:460px;margin:0 auto;margin-top:calc(50vh - 340px);border:solid 3px #fff;padding:0 10px 10px;border-radius:3px}#l{display:block;max-width:150px;height:155px;margin:-80px auto 20px;background:#00d151;padding-right:5px}#l path{fill:#000}.w{margin:3px 0 16px;padding:5px 0;border-radius:3px;background:#fff;box-shadow:0 1px 3px rgba(0,0,0,.3)}h1,h3{margin:10px 8px;font-family:HelveticaNeueLight,HelveticaNeue-Light,\"Helvetica Neue Light\",HelveticaNeue,\"Helvetica Neue\",TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:300;font-stretch:normal;color:#000;font-size:23px}h1{margin-bottom:14px;color:#fff}span{display:block;margin:10px 7px 14px}i{display:block;font-style:normal;position:relative;border-bottom:solid 1px #00d151;height:42px}i:last-child{border:none}label{position:absolute;display:inline-block;top:0;left:8px;color:#00d151;line-height:41px;pointer-events:none}input,select{width:calc(100% - 145px);border:none;font-size:16px;line-height:40px;border-radius:0;letter-spacing:-.5px;background:#fff;color:#000;padding-left:144px;-webkit-appearance:none;-moz-appearance:none;appearance:none;outline:0!important;height:40px}select{padding:0;float:right;margin:1px 3px 1px 2px}button{width:100%;border:0;background:#000;padding:5px 10px;font-size:16px;line-height:40px;color:#fff;border-radius:3px;box-shadow:0 1px 3px rgba(0,0,0,.3);cursor:pointer}.c{background:#ffe836;position:fixed;width:100%;line-height:80px;color:#000;top:0;left:0;box-shadow:0 1px 3px rgba(0,0,0,.3);text-align:center;font-size:26px;z-index:100}@media all and (max-height:920px){.s{margin-top:80px}}@media all and (max-width:900px){.s{width:calc(100% - 20px);margin-top:40px;border:none;padding:0 8px;border-radius:0}#l{max-width:80px;height:auto;margin:10px auto 20px}h1,h3{font-size:19px}i{border:none;height:auto}label{display:block;margin:4px 0 12px;color:#00d151;font-size:13px;position:relative;line-height:18px}input,select{width:calc(100% - 10px);font-size:16px;line-height:28px;padding:0 5px;border-bottom:solid 1px #00d151}select{width:100%;float:none;margin:0}}</style><script type=\"text/javascript\">setTimeout(function(){var element =  document.getElementById('msg');if ( element != null ) element.style.visibility = \"hidden\";},3200);</script>";

		#ifdef __FOTA
		char html_template[] = "%s%s<div class=\"s\"><svg version=\"1.1\" id=\"l\" x=\"0\" y=\"0\" viewBox=\"0 0 200 200\" xml:space=\"preserve\"><path d=\"M59.3,2.5c18.1,0.6,31.8,8,40.2,23.5c3.1,5.7,4.3,11.9,4.1,18.3c-0.1,3.6-0.7,7.1-1.9,10.6c-0.2,0.7-0.1,1.1,0.6,1.5c12.8,7.7,25.5,15.4,38.3,23c2.9,1.7,5.8,3.4,8.7,5.3c1,0.6,1.6,0.6,2.5-0.1c4.5-3.6,9.8-5.3,15.7-5.4c12.5-0.1,22.9,7.9,25.2,19c1.9,9.2-2.9,19.2-11.8,23.9c-8.4,4.5-16.9,4.5-25.5,0.2c-0.7-0.3-1-0.2-1.5,0.3c-4.8,4.9-9.7,9.8-14.5,14.6c-5.3,5.3-10.6,10.7-15.9,16c-1.8,1.8-3.6,3.7-5.4,5.4c-0.7,0.6-0.6,1,0,1.6c3.6,3.4,5.8,7.5,6.2,12.2c0.7,7.7-2.2,14-8.8,18.5c-12.3,8.6-30.3,3.5-35-10.4c-2.8-8.4,0.6-17.7,8.6-22.8c0.9-0.6,1.1-1,0.8-2c-2-6.2-4.4-12.4-6.6-18.6c-6.3-17.6-12.7-35.1-19-52.7c-0.2-0.7-0.5-1-1.4-0.9c-12.5,0.7-23.6-2.6-33-10.4c-8-6.6-12.9-15-14.2-25c-1.5-11.5,1.7-21.9,9.6-30.7C32.5,8.9,42.2,4.2,53.7,2.7c0.7-0.1,1.5-0.2,2.2-0.2C57,2.4,58.2,2.5,59.3,2.5z M76.5,81c0,0.1,0.1,0.3,0.1,0.6c1.6,6.3,3.2,12.6,4.7,18.9c4.5,17.7,8.9,35.5,13.3,53.2c0.2,0.9,0.6,1.1,1.6,0.9c5.4-1.2,10.7-0.8,15.7,1.6c0.8,0.4,1.2,0.3,1.7-0.4c11.2-12.9,22.5-25.7,33.4-38.7c0.5-0.6,0.4-1,0-1.6c-5.6-7.9-6.1-16.1-1.3-24.5c0.5-0.8,0.3-1.1-0.5-1.6c-9.1-4.7-18.1-9.3-27.2-14c-6.8-3.5-13.5-7-20.3-10.5c-0.7-0.4-1.1-0.3-1.6,0.4c-1.3,1.8-2.7,3.5-4.3,5.1c-4.2,4.2-9.1,7.4-14.7,9.7C76.9,80.3,76.4,80.3,76.5,81z M89,42.6c0.1-2.5-0.4-5.4-1.5-8.1C83,23.1,74.2,16.9,61.7,15.8c-10-0.9-18.6,2.4-25.3,9.7c-8.4,9-9.3,22.4-2.2,32.4c6.8,9.6,19.1,14.2,31.4,11.9C79.2,67.1,89,55.9,89,42.6z M102.1,188.6c0.6,0.1,1.5-0.1,2.4-0.2c9.5-1.4,15.3-10.9,11.6-19.2c-2.6-5.9-9.4-9.6-16.8-8.6c-8.3,1.2-14.1,8.9-12.4,16.6C88.2,183.9,94.4,188.6,102.1,188.6z M167.7,88.5c-1,0-2.1,0.1-3.1,0.3c-9,1.7-14.2,10.6-10.8,18.6c2.9,6.8,11.4,10.3,19,7.8c7.1-2.3,11.1-9.1,9.6-15.9C180.9,93,174.8,88.5,167.7,88.5z\"/></svg><h1>%s</h1><span>LAST STATE: %s<br>Firmware: %s<br>GUID: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X<br>MAC: %02X:%02X:%02X:%02X:%02X:%02X</span><form method=\"post\"><div class=\"w\"><h3>Wi-Fi Settings</h3><i><input name=\"sid\" value=\"%s\"><label>Network name</label></i><i><input name=\"wpw\"><label>Password</label></i></div><div class=\"w\"><h3>Supla Settings</h3><i><input name=\"svr\" value=\"%s\"><label>Server</label></i><i><input name=\"eml\" value=\"%s\"><label>E-mail</label></i></div><div class=\"w\"><h3>Additional Settings</h3><i><select name=\"upd\"><option value=\"0\" %s>NO</option><option value=\"1\" %s>YES</option></select><label>Firmware update</label></i></div><button type=\"submit\">SAVE</button></form></div><br><br>";
		#else
		char html_template[] = "%s%s<div class=\"s\"><svg version=\"1.1\" id=\"l\" x=\"0\" y=\"0\" viewBox=\"0 0 200 200\" xml:space=\"preserve\"><path d=\"M59.3,2.5c18.1,0.6,31.8,8,40.2,23.5c3.1,5.7,4.3,11.9,4.1,18.3c-0.1,3.6-0.7,7.1-1.9,10.6c-0.2,0.7-0.1,1.1,0.6,1.5c12.8,7.7,25.5,15.4,38.3,23c2.9,1.7,5.8,3.4,8.7,5.3c1,0.6,1.6,0.6,2.5-0.1c4.5-3.6,9.8-5.3,15.7-5.4c12.5-0.1,22.9,7.9,25.2,19c1.9,9.2-2.9,19.2-11.8,23.9c-8.4,4.5-16.9,4.5-25.5,0.2c-0.7-0.3-1-0.2-1.5,0.3c-4.8,4.9-9.7,9.8-14.5,14.6c-5.3,5.3-10.6,10.7-15.9,16c-1.8,1.8-3.6,3.7-5.4,5.4c-0.7,0.6-0.6,1,0,1.6c3.6,3.4,5.8,7.5,6.2,12.2c0.7,7.7-2.2,14-8.8,18.5c-12.3,8.6-30.3,3.5-35-10.4c-2.8-8.4,0.6-17.7,8.6-22.8c0.9-0.6,1.1-1,0.8-2c-2-6.2-4.4-12.4-6.6-18.6c-6.3-17.6-12.7-35.1-19-52.7c-0.2-0.7-0.5-1-1.4-0.9c-12.5,0.7-23.6-2.6-33-10.4c-8-6.6-12.9-15-14.2-25c-1.5-11.5,1.7-21.9,9.6-30.7C32.5,8.9,42.2,4.2,53.7,2.7c0.7-0.1,1.5-0.2,2.2-0.2C57,2.4,58.2,2.5,59.3,2.5z M76.5,81c0,0.1,0.1,0.3,0.1,0.6c1.6,6.3,3.2,12.6,4.7,18.9c4.5,17.7,8.9,35.5,13.3,53.2c0.2,0.9,0.6,1.1,1.6,0.9c5.4-1.2,10.7-0.8,15.7,1.6c0.8,0.4,1.2,0.3,1.7-0.4c11.2-12.9,22.5-25.7,33.4-38.7c0.5-0.6,0.4-1,0-1.6c-5.6-7.9-6.1-16.1-1.3-24.5c0.5-0.8,0.3-1.1-0.5-1.6c-9.1-4.7-18.1-9.3-27.2-14c-6.8-3.5-13.5-7-20.3-10.5c-0.7-0.4-1.1-0.3-1.6,0.4c-1.3,1.8-2.7,3.5-4.3,5.1c-4.2,4.2-9.1,7.4-14.7,9.7C76.9,80.3,76.4,80.3,76.5,81z M89,42.6c0.1-2.5-0.4-5.4-1.5-8.1C83,23.1,74.2,16.9,61.7,15.8c-10-0.9-18.6,2.4-25.3,9.7c-8.4,9-9.3,22.4-2.2,32.4c6.8,9.6,19.1,14.2,31.4,11.9C79.2,67.1,89,55.9,89,42.6z M102.1,188.6c0.6,0.1,1.5-0.1,2.4-0.2c9.5-1.4,15.3-10.9,11.6-19.2c-2.6-5.9-9.4-9.6-16.8-8.6c-8.3,1.2-14.1,8.9-12.4,16.6C88.2,183.9,94.4,188.6,102.1,188.6z M167.7,88.5c-1,0-2.1,0.1-3.1,0.3c-9,1.7-14.2,10.6-10.8,18.6c2.9,6.8,11.4,10.3,19,7.8c7.1-2.3,11.1-9.1,9.6-15.9C180.9,93,174.8,88.5,167.7,88.5z\"/></svg><h1>%s</h1><span>LAST STATE: %s<br>Firmware: %s<br>GUID: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X<br>MAC: %02X:%02X:%02X:%02X:%02X:%02X</span><form method=\"post\"><div class=\"w\"><h3>Wi-Fi Settings</h3><i><input name=\"sid\" value=\"%s\"><label>Network name</label></i><i><input name=\"wpw\"><label>Password</label></i></div><div class=\"w\"><h3>Supla Settings</h3><i><input name=\"svr\" value=\"%s\"><label>Server</label></i><i><input name=\"eml\" value=\"%s\"><label>E-mail</label></i></div><button type=\"submit\">SAVE</button></form></div><br><br>";
		#endif

		int bufflen = strlen(supla_esp_get_laststate())
					  +strlen(dev_name)
					  +strlen(SUPLA_ESP_SOFTVER)
					  +strlen(supla_esp_cfg.WIFI_SSID)
					  +strlen(supla_esp_cfg.Server)
					  +strlen(supla_esp_cfg.Email)
					  +strlen(html_template_header)
					  +strlen(html_template)
					  +200;

		buffer = (char*)malloc(bufflen);

		ets_snprintf(buffer,
				bufflen,
				html_template,
				html_template_header,
				data_saved == 1 ? "<div id=\"msg\" class=\"c\">Data saved</div>" : "",
				dev_name,
				supla_esp_get_laststate(),
				SUPLA_ESP_SOFTVER,
				(unsigned char)supla_esp_cfg.GUID[0],
				(unsigned char)supla_esp_cfg.GUID[1],
				(unsigned char)supla_esp_cfg.GUID[2],
				(unsigned char)supla_esp_cfg.GUID[3],
				(unsigned char)supla_esp_cfg.GUID[4],
				(unsigned char)supla_esp_cfg.GUID[5],
				(unsigned char)supla_esp_cfg.GUID[6],
				(unsigned char)supla_esp_cfg.GUID[7],
				(unsigned char)supla_esp_cfg.GUID[8],
				(unsigned char)supla_esp_cfg.GUID[9],
				(unsigned char)supla_esp_cfg.GUID[10],
				(unsigned char)supla_esp_cfg.GUID[11],
				(unsigned char)supla_esp_cfg.GUID[12],
				(unsigned char)supla_esp_cfg.GUID[13],
				(unsigned char)supla_esp_cfg.GUID[14],
				(unsigned char)supla_esp_cfg.GUID[15],
				(unsigned char)mac[0],
				(unsigned char)mac[1],
				(unsigned char)mac[2],
				(unsigned char)mac[3],
				(unsigned char)mac[4],
				(unsigned char)mac[5],
				supla_esp_cfg.WIFI_SSID,
				supla_esp_cfg.Server,
				supla_esp_cfg.Email
				#ifdef __FOTA
				,
				supla_esp_cfg.FirmwareUpdate == 0 ? "selected" : "",
				supla_esp_cfg.FirmwareUpdate == 1 ? "selected" : ""
				#endif
				);

	#endif

	#endif /*BOARD_CFG_HTML_TEMPLATE*/
		
	
	supla_log(LOG_DEBUG, "HTTP OK (%i) Free heap size: %i", buffer!=NULL, system_get_free_heap_size());
		
	if ( buffer ) {
		supla_esp_http_ok((struct espconn *)arg, buffer);
		free(buffer);
	}
	
	
}


void ICACHE_FLASH_ATTR
supla_esp_discon_callback(void *arg) {
	
	supla_log(LOG_DEBUG, "Disconnect");

    struct espconn *conn = (struct espconn *)arg;
	
    if ( conn->reverse != NULL ) {
    	free(conn->reverse);
    	conn->reverse = NULL;
    }
}

void ICACHE_FLASH_ATTR supla_esp_connectcb(void *arg) {
  struct espconn *conn = (struct espconn *)arg;

  // espconn_set_opt(conn, ESPCONN_NODELAY);
  espconn_set_opt(conn, ESPCONN_COPY);
  // espconn_set_opt(conn, ESPCONN_REUSEADDR);

  TrivialHttpParserVars *pVars = malloc(sizeof(TrivialHttpParserVars));
  memset(pVars, 0, sizeof(TrivialHttpParserVars));
  conn->reverse = pVars;

  espconn_regist_recvcb(conn, supla_esp_recv_callback);
  espconn_regist_disconcb(conn, supla_esp_discon_callback);
}

void ICACHE_FLASH_ATTR supla_esp_cfgmode_start(void) {
  char APSSID[] = AP_SSID;
  char mac[6];

#ifdef BOARD_BEFORE_CFGMODE_START
  supla_esp_board_before_cfgmode_start();
#endif

  supla_esp_devconn_before_cfgmode_start();

  wifi_get_macaddr(SOFTAP_IF, (unsigned char *)mac);

  struct softap_config apconfig;
  wifi_softap_get_config(&apconfig);

  memset(apconfig.ssid, 0, sizeof(apconfig.ssid));
  memset(apconfig.password, 0, sizeof(apconfig.password));

  struct espconn *conn;

  if (supla_esp_cfgmode_entertime != 0) return;

  supla_esp_devconn_stop();

  supla_esp_cfgmode_entertime = system_get_time();

  supla_log(LOG_DEBUG, "ENTER CFG MODE");
  supla_esp_gpio_state_cfgmode();

#ifdef WIFI_SLEEP_DISABLE
  wifi_set_sleep_type(NONE_SLEEP_T);
#endif

  int apssid_len = strnlen(APSSID, sizeof(apconfig.ssid));
  memcpy(apconfig.ssid, APSSID, apssid_len);

  char mac_str[14] = {};

#ifdef CFGMODE_SSID_LIMIT_MACLEN
  ets_snprintf(mac_str, sizeof(mac_str), "-%02X%02X", (unsigned char)mac[4],
               (unsigned char)mac[5]);
#else
  ets_snprintf(mac_str, sizeof(mac_str), "-%02X%02X%02X%02X%02X%02X",
               (unsigned char)mac[0], (unsigned char)mac[1],
               (unsigned char)mac[2], (unsigned char)mac[3],
               (unsigned char)mac[4], (unsigned char)mac[5]);
#endif /*CFGMODE_SSID_LIMIT_MACLEN*/

  int mac_str_len = strnlen(mac_str, sizeof(mac_str));

  if (apssid_len + mac_str_len > sizeof(apconfig.ssid)) {
    apssid_len -= apssid_len + mac_str_len - sizeof(apconfig.ssid);
  }

  memcpy(&apconfig.ssid[apssid_len], mac_str, mac_str_len);
  apconfig.ssid_len = apssid_len + mac_str_len;
  apconfig.channel = 1;
  apconfig.authmode = AUTH_OPEN;
  apconfig.ssid_hidden = 0;
  apconfig.max_connection = 4;
  apconfig.beacon_interval = 100;

  wifi_set_opmode(SOFTAP_MODE);

  wifi_softap_set_config(&apconfig);

  conn = (struct espconn *)malloc(sizeof(struct espconn));
  memset(conn, 0, sizeof(struct espconn));

  espconn_create(conn);
  espconn_regist_time(conn, 5, 0);

  conn->type = ESPCONN_TCP;
  conn->state = ESPCONN_NONE;

  conn->proto.tcp = (esp_tcp *)zalloc(sizeof(esp_tcp));
  conn->proto.tcp->local_port = 80;

  espconn_regist_connectcb(conn, supla_esp_connectcb);
  espconn_accept(conn);
}

char ICACHE_FLASH_ATTR
supla_esp_cfgmode_started(void) {

	return supla_esp_cfgmode_entertime == 0 ? 0 : 1;
}

