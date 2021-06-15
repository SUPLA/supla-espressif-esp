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

#include "supla_esp_cfgmode.h"

#include <espconn.h>
#include <ip_addr.h>
#include <mem.h>
#include <osapi.h>
#include <spi_flash.h>
#include <user_interface.h>

#include "supla-dev/log.h"
#include "supla_esp.h"
#include "supla_esp_cfg.h"
#include "supla_esp_devconn.h"
#include "supla_esp_gpio.h"
#include "supla_esp_mqtt.h"

#define TYPE_UNKNOWN 0
#define TYPE_GET 1
#define TYPE_POST 2

#define STEP_TYPE 0
#define STEP_GET 2
#define STEP_POST 3
#define STEP_PARSE_VARS 4
#define STEP_DONE 10

#define VAR_NONE 0
#define VAR_SID 1
#define VAR_WPW 2
#define VAR_SVR 3
#define VAR_LID 4
#define VAR_PWD 5
#define VAR_CFGBTN 6
#define VAR_BTN1 7
#define VAR_BTN2 8
#define VAR_ICF 9
#define VAR_LED 10
#define VAR_UPD 11
#define VAR_RBT 12
#define VAR_EML 20
#define VAR_USD 21
#define VAR_TRG 22
#define VAR_CMD 23

#define VAR_PRO 24  // Protocol
#define VAR_MVR 25  // MQTT Server
#define VAR_PRT 26  // Port
#define VAR_TLS 27  // TLS
#define VAR_USR 28  // Username
#define VAR_MWD 29  // MQTT Password
#define VAR_PFX 30  // Topic prefix
#define VAR_QOS 31  // MQTT QoS
#define VAR_RET 32  // MQTT No Retain
#define VAR_MAU 33  // MQTT Auth

#define VAR_TH1 34  // Overcurrent threshold 1
#define VAR_TH2 35  // Overcurrent threshold 2

#ifdef CFG_TIME_VARIABLES
#define VAR_T10 36
#define VAR_T11 37
#define VAR_T20 38
#define VAR_T21 39
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

} TrivialHttpParserVars;

typedef struct {
  unsigned int entertime;
  ETSTimer response_timer;

  char *header;
  uint32 header_size;

  char *content;
  uint32 content_size;
  uint32 pos;

} _cfgmode_vars_t;

_cfgmode_vars_t cfgmode_vars = {};

char *ICACHE_FLASH_ATTR supla_esp_cfgmode_get_html_template(
    char dev_name[25], const char mac[6], const char data_saved);

void ICACHE_FLASH_ATTR
supla_esp_http_send_response_cb(struct espconn *pespconn) {
  char *ptr = NULL;
  uint32 size = 0;

  if (cfgmode_vars.header) {
    if (cfgmode_vars.pos >= cfgmode_vars.header_size) {
      free(cfgmode_vars.header);
      cfgmode_vars.header = NULL;
      cfgmode_vars.pos = 0;
      return;
    } else {
      ptr = &cfgmode_vars.header[cfgmode_vars.pos];
      size = cfgmode_vars.header_size;
    }
  } else if (cfgmode_vars.content) {
    if (cfgmode_vars.pos >= cfgmode_vars.content_size) {
      free(cfgmode_vars.content);
      cfgmode_vars.content = NULL;
      cfgmode_vars.pos = 0;
    } else {
      ptr = &cfgmode_vars.content[cfgmode_vars.pos];
      size = cfgmode_vars.content_size;
    }
  }

  if (ptr == NULL) {
    os_timer_disarm(&cfgmode_vars.response_timer);
    return;
  }

  size -= cfgmode_vars.pos;

  if (size > 200) {
    size = 200;
  }

  if (0 == espconn_sent(pespconn, (unsigned char *)ptr, size)) {
    cfgmode_vars.pos += size;
  }
}

void ICACHE_FLASH_ATTR supla_esp_http_send_response(struct espconn *pespconn,
                                                    const char *code,
                                                    char *html) {
  os_timer_disarm(&cfgmode_vars.response_timer);

  cfgmode_vars.pos = 0;

  if (cfgmode_vars.header) {
    free(cfgmode_vars.header);
    cfgmode_vars.header = NULL;
  }

  if (cfgmode_vars.content) {
    free(cfgmode_vars.content);
    cfgmode_vars.content = NULL;
  }

  cfgmode_vars.content_size = html != NULL ? strlen(html) : 0;
  cfgmode_vars.content = cfgmode_vars.content_size ? html : NULL;

  char header[] =
      "HTTP/1.1 %s\r\nAccept-Ranges: bytes\r\nContent-Length: "
      "%i\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: "
      "close\r\n\r\n";

  char c = 0;
  cfgmode_vars.header_size =
      ets_snprintf(&c, 1, header, code, cfgmode_vars.content_size) + 1;

  cfgmode_vars.header = os_malloc(cfgmode_vars.header_size);
  if (!cfgmode_vars.header) {
    return;
  }

  ets_snprintf(cfgmode_vars.header, cfgmode_vars.header_size, header, code,
               cfgmode_vars.content_size);
  cfgmode_vars.header_size--;

  os_timer_setfn(&cfgmode_vars.response_timer,
                 (os_timer_func_t *)supla_esp_http_send_response_cb, pespconn);
  os_timer_arm(&cfgmode_vars.response_timer, 10, 1);
}

void ICACHE_FLASH_ATTR supla_esp_http_ok(struct espconn *pespconn, char *html) {
  supla_esp_http_send_response(pespconn, "200 OK", html);
}

void ICACHE_FLASH_ATTR supla_esp_http_404(struct espconn *pespconn) {
  supla_esp_http_send_response(pespconn, "404 Not Found", "Not Found");
}

void ICACHE_FLASH_ATTR supla_esp_http_error(struct espconn *pespconn) {
  supla_esp_http_send_response(pespconn, "500 Internal Server Error", "Error");
}

int ICACHE_FLASH_ATTR Power(int x, int y) {
  int result = 1;
  while (y) {
    if (y & 1) result *= x;

    y >>= 1;
    x *= x;
  }

  return result;
}

int ICACHE_FLASH_ATTR HexToInt(char *str, int len) {
  int a, n, p;
  int result = 0;

  if (len % 2 != 0) return 0;

  p = len - 1;

  for (a = 0; a < len; a++) {
    n = 0;

    if (str[a] >= 'A' && str[a] <= 'F')
      n = str[a] - 55;
    else if (str[a] >= 'a' && str[a] <= 'f')
      n = str[a] - 87;
    else if (str[a] >= '0' && str[a] <= '9')
      n = str[a] - 48;

    result += Power(16, p) * n;
    p--;
  }

  return result;
};
int ICACHE_FLASH_ATTR cfg_str2int(TrivialHttpParserVars *pVars) {
  int result = 0;

  short s = 0;
  while (pVars->intval[s] != 0) {
    if (pVars->intval[s] >= '0' && pVars->intval[s] <= '9') {
      result = result * 10 + pVars->intval[s] - '0';
    }

    s++;
  }

  return result;
}

// Converts float with 0.01 precision to int multiplied by 100, i.e. "3.1415" -> 314 
int ICACHE_FLASH_ATTR cfg_str2centInt(TrivialHttpParserVars *pVars) {
  int result = 0;

  int decimalPlaces = -1;
  for (int i = 0; pVars->intval[i] != 0; i++) {
    if (pVars->intval[i] >= '0' && pVars->intval[i] <= '9') {
      result = result * 10 + pVars->intval[i] - '0';
      if (decimalPlaces >= 0) {
        decimalPlaces++;
      }
    } else if (pVars->intval[i] == '.' || pVars->intval[i] == ',') {
      decimalPlaces++;
    }
    if (decimalPlaces >= 2) {
      break;
    }
  }

  if (decimalPlaces < 0) {
    decimalPlaces = 0;
  }

  while (decimalPlaces < 2) {
    result *= 10;
    decimalPlaces++;
  }

  return result;
}

void ICACHE_FLASH_ATTR supla_esp_parse_proto_var(TrivialHttpParserVars *pVars,
                                                 char *pdata,
                                                 unsigned short len,
                                                 SuplaEspCfg *cfg) {
  for (int a = 0; a < len; a++) {
    if (pVars->current_var == VAR_NONE) {
      char pro[3] = {'p', 'r', 'o'};

      if (len - a >= 4 && pdata[a + 3] == '=') {
        if (memcmp(pro, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_PRO;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;
        }

        a += 4;
        pVars->offset = 0;
      }
    }

    if (pVars->current_var == VAR_PRO) {
      if (pVars->offset >= pVars->buff_size || a >= len - 1 ||
          pdata[a] == '&') {
        if (pVars->offset < pVars->buff_size)
          pVars->pbuff[pVars->offset] = 0;
        else
          pVars->pbuff[pVars->buff_size - 1] = 0;

        pVars->matched++;
        pVars->current_var = VAR_NONE;

        if (pVars->intval[0] - '0' == 1) {
          cfg->Flags |= CFG_FLAG_MQTT_ENABLED;
        } else {
          cfg->Flags &= ~CFG_FLAG_MQTT_ENABLED;
        }
        return;
      }
    }
  }
}

void ICACHE_FLASH_ATTR supla_esp_parse_vars(TrivialHttpParserVars *pVars,
                                            char *pdata, unsigned short len,
                                            SuplaEspCfg *cfg, char *reboot) {
  for (int a = 0; a < len; a++) {
    if (pVars->current_var == VAR_NONE) {
      char sid[3] = {'s', 'i', 'd'};
      char wpw[3] = {'w', 'p', 'w'};
      char svr[3] = {'s', 'v', 'r'};
      char lid[3] = {'l', 'i', 'd'};
      char pwd[3] = {'p', 'w', 'd'};
      char btncfg[3] = {'c', 'f', 'g'};
      char btn1[3] = {'b', 't', '1'};
      char btn2[3] = {'b', 't', '2'};
      char icf[3] = {'i', 'c', 'f'};
      char led[3] = {'l', 'e', 'd'};
      char upd[3] = {'u', 'p', 'd'};
      char rbt[3] = {'r', 'b', 't'};
      char eml[3] = {'e', 'm', 'l'};
      char usd[3] = {'u', 's', 'd'};
      char trg[3] = {'t', 'r', 'g'};
      char cmd[3] = {'c', 'm', 'd'};

      char mvr[3] = {'m', 'v', 'r'};
      char prt[3] = {'p', 'r', 't'};
      char tls[3] = {'t', 'l', 's'};
      char usr[3] = {'u', 's', 'r'};
      char mwd[3] = {'m', 'w', 'd'};
      char pfx[3] = {'p', 'f', 'x'};
      char qos[3] = {'q', 'o', 's'};
      char ret[3] = {'r', 'e', 't'};
      char mau[3] = {'m', 'a', 'u'};

      char th1[3] = {'t', 'h', '1'};
      char th2[3] = {'t', 'h', '2'};

#ifdef CFG_TIME_VARIABLES
      char t10[3] = {'t', '1', '0'};
      char t11[3] = {'t', '1', '1'};
      char t20[3] = {'t', '2', '0'};
      char t21[3] = {'t', '2', '1'};
#endif /*CFG_TIME_VARIABLES*/

      if (len - a >= 4 && pdata[a + 3] == '=') {
        if (memcmp(sid, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_SID;
          pVars->buff_size = WIFI_SSID_MAXSIZE;
          pVars->pbuff = cfg->WIFI_SSID;

        } else if (memcmp(wpw, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_WPW;
          pVars->buff_size = WIFI_PWD_MAXSIZE;
          pVars->pbuff = cfg->WIFI_PWD;

        } else if (memcmp(svr, &pdata[a], 3) == 0) {
          if (!(cfg->Flags & CFG_FLAG_MQTT_ENABLED)) {
            pVars->current_var = VAR_SVR;
            pVars->buff_size = SERVER_MAXSIZE;
            pVars->pbuff = cfg->Server;
          }

        } else if (memcmp(lid, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_LID;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(pwd, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_PWD;
          pVars->buff_size = SUPLA_LOCATION_PWD_MAXSIZE;
          pVars->pbuff = cfg->LocationPwd;

        } else if (memcmp(btncfg, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_CFGBTN;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(btn1, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_BTN1;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(btn2, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_BTN2;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(icf, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_ICF;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(led, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_LED;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(upd, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_UPD;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(rbt, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_RBT;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(eml, &pdata[a], 3) == 0) {
          if (!(cfg->Flags & CFG_FLAG_MQTT_ENABLED)) {
            pVars->current_var = VAR_EML;
            pVars->buff_size = SUPLA_EMAIL_MAXSIZE;
            pVars->pbuff = cfg->Email;
          }
        } else if (memcmp(usd, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_USD;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(trg, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_TRG;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(cmd, &pdata[a], 3) == 0) {
          if (user_cmd == NULL) {
            user_cmd = malloc(CMD_MAXSIZE);
          }

          pVars->current_var = VAR_CMD;
          pVars->buff_size = CMD_MAXSIZE;
          pVars->pbuff = user_cmd;

        } else if (memcmp(mvr, &pdata[a], 3) == 0) {
          if (cfg->Flags & CFG_FLAG_MQTT_ENABLED) {
            pVars->current_var = VAR_MVR;
            pVars->buff_size = SERVER_MAXSIZE;
            pVars->pbuff = cfg->Server;
          }

        } else if (memcmp(prt, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_PRT;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(tls, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_TLS;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(usr, &pdata[a], 3) == 0) {
          if (cfg->Flags & CFG_FLAG_MQTT_ENABLED) {
            pVars->current_var = VAR_USR;
            pVars->buff_size = SUPLA_EMAIL_MAXSIZE;
            pVars->pbuff = cfg->Username;
          }

        } else if (memcmp(mwd, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_MWD;
          pVars->buff_size = SUPLA_LOCATION_PWD_MAXSIZE;
          pVars->pbuff = cfg->Password;

        } else if (memcmp(pfx, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_PFX;
          pVars->buff_size = MQTT_PREFIX_SIZE;
          pVars->pbuff = cfg->MqttTopicPrefix;

        } else if (memcmp(qos, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_QOS;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(ret, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_RET;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(mau, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_MAU;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(th1, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_TH1;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(th2, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_TH2;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;
        }
#ifdef CFG_TIME_VARIABLES
          else if (memcmp(t10, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_T10;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(t11, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_T11;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(t20, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_T20;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(t21, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_T21;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;
        }
#endif /*CFG_TIME_VARIABLES*/
        a += 4;
        pVars->offset = 0;
      }
    }

    if (pVars->current_var != VAR_NONE) {
      if (pVars->offset < pVars->buff_size && a < len && pdata[a] != '&') {
        if (pdata[a] == '%' && a + 2 < len) {
          pVars->pbuff[pVars->offset] = HexToInt(&pdata[a + 1], 2);
          pVars->offset++;
          a += 2;

        } else if (pdata[a] == '+') {
          pVars->pbuff[pVars->offset] = ' ';
          pVars->offset++;

        } else {
          pVars->pbuff[pVars->offset] = pdata[a];
          pVars->offset++;
        }
      }

      if (pVars->offset >= pVars->buff_size || a >= len - 1 ||
          pdata[a] == '&') {
        if (pVars->offset < pVars->buff_size)
          pVars->pbuff[pVars->offset] = 0;
        else
          pVars->pbuff[pVars->buff_size - 1] = 0;

        if (pVars->current_var == VAR_LID) {
          cfg->LocationID = cfg_str2int(pVars);

        } else if (pVars->current_var == VAR_CFGBTN) {
          cfg->CfgButtonType = pVars->intval[0] - '0';

        } else if (pVars->current_var == VAR_BTN1) {
          cfg->Button1Type = pVars->intval[0] - '0';

        } else if (pVars->current_var == VAR_BTN2) {
          cfg->Button2Type = pVars->intval[0] - '0';

        } else if (pVars->current_var == VAR_ICF) {
          cfg->InputCfgTriggerOff = (pVars->intval[0] - '0') == 1 ? 1 : 0;

        } else if (pVars->current_var == VAR_LED) {
          cfg->StatusLedOff = (pVars->intval[0] - '0');

        } else if (pVars->current_var == VAR_UPD) {
          cfg->FirmwareUpdate = pVars->intval[0] - '0';

        } else if (pVars->current_var == VAR_RBT) {
          if (reboot != NULL) *reboot = pVars->intval[0] - '0';

        } else if (pVars->current_var == VAR_USD) {
          cfg->UpsideDown = (pVars->intval[0] - '0') == 1 ? 1 : 0;

        } else if (pVars->current_var == VAR_TRG) {
          cfg->Trigger = pVars->intval[0] - '0';

        } else if (pVars->current_var == VAR_PRT) {
          int port = cfg_str2int(pVars);
          if (port > 0 && port <= 65535) {
            cfg->Port = port;
          }

        } else if (pVars->current_var == VAR_TLS) {
          if (pVars->intval[0] - '0' == 1) {
            cfg->Flags |= CFG_FLAG_MQTT_TLS;
          } else {
            cfg->Flags &= ~CFG_FLAG_MQTT_TLS;
          }

        } else if (pVars->current_var == VAR_QOS) {
          int qos = pVars->intval[0] - '0';
          if (qos >= 0 && qos <= 2) {
            cfg->MqttQoS = qos;
          }

        } else if (pVars->current_var == VAR_RET) {
          if (pVars->intval[0] - '0' == 1) {
            cfg->Flags |= CFG_FLAG_MQTT_NO_RETAIN;
          } else {
            cfg->Flags &= ~CFG_FLAG_MQTT_NO_RETAIN;
          }

        } else if (pVars->current_var == VAR_MAU) {
          if (pVars->intval[0] - '0' == 1) {
            cfg->Flags &= ~CFG_FLAG_MQTT_NO_AUTH;
          } else {
            cfg->Flags |= CFG_FLAG_MQTT_NO_AUTH;
          }

        } else if (pVars->current_var == VAR_TH1) {
          cfg->OvercurrentThreshold1 = cfg_str2centInt(pVars);
          supla_log(LOG_DEBUG, "Found TH1 = %d", cfg->OvercurrentThreshold1);

        } else if (pVars->current_var == VAR_TH2) {
          cfg->OvercurrentThreshold2 = cfg_str2centInt(pVars);
          supla_log(LOG_DEBUG, "Found TH2 = %d", cfg->OvercurrentThreshold2);
        }

#ifdef CFG_TIME_VARIABLES
          else if (pVars->current_var == VAR_T10) {
          cfg->Time1[0] = cfg_str2int(pVars);

        } else if (pVars->current_var == VAR_T11) {
          cfg->Time1[1] = cfg_str2int(pVars);

        } else if (pVars->current_var == VAR_T20) {
          cfg->Time2[0] = cfg_str2int(pVars);

        } else if (pVars->current_var == VAR_T21) {
          cfg->Time2[1] = cfg_str2int(pVars);
        }
#endif /*CFG_TIME_VARIABLES*/
        pVars->matched++;
        pVars->current_var = VAR_NONE;
      }
    }
  }
}

void ICACHE_FLASH_ATTR supla_esp_parse_request(TrivialHttpParserVars *pVars,
                                               char *pdata, unsigned short len,
                                               SuplaEspCfg *cfg, char *reboot) {
  if (len == 0) return;

  int a, p;

  if (pVars->step == STEP_TYPE) {
    char get[] = "GET";
    char post[] = "POST";
    char url[] = " / HTTP";

    if (len >= 3 && memcmp(pdata, get, 3) == 0 && len >= 10 &&
        memcmp(&pdata[3], url, 7) == 0) {
      pVars->step = STEP_GET;
      pVars->type = TYPE_GET;

    } else if (len >= 4 && memcmp(pdata, post, 4) == 0 && len >= 11 &&
               memcmp(&pdata[4], url, 7) == 0) {
      pVars->step = STEP_POST;
      pVars->type = TYPE_POST;
    }
  }

  p = 0;

  if (pVars->step == STEP_POST) {
    char header_end[4] = {'\r', '\n', '\r', '\n'};

    for (a = p; a < len; a++) {
      if (len - a >= 4 && memcmp(header_end, &pdata[a], 4) == 0) {
        pVars->step = STEP_PARSE_VARS;
        p += 3;
      }
    }
  }

  if (pVars->step == STEP_PARSE_VARS) {
    len -= p;
    pdata += p;
    // First, check what protocol is selected
    supla_esp_parse_proto_var(pVars, pdata, len, cfg);
    // Only after checking the protocol, parse the remaining fields of the form
    supla_esp_parse_vars(pVars, pdata, len, cfg, reboot);
  }
}

void ICACHE_FLASH_ATTR supla_esp_recv_callback(void *arg, char *pdata,
                                               unsigned short len) {
  struct espconn *conn = (struct espconn *)arg;
  char mac[6];
  char data_saved = 0;
  char reboot = 0;

  supla_log(LOG_DEBUG, "Free heap size: %i", system_get_free_heap_size());
  supla_log(LOG_DEBUG, "REQUEST LEN: %i", len);

  TrivialHttpParserVars *pVars = (TrivialHttpParserVars *)conn->reverse;

  if (pdata == NULL || pVars == NULL) return;

  SuplaEspCfg new_cfg;
  memcpy(&new_cfg, &supla_esp_cfg, sizeof(SuplaEspCfg));

  supla_esp_parse_request(pVars, pdata, len, &new_cfg, &reboot);

  if (pVars->type == TYPE_UNKNOWN) {
    supla_esp_http_404(conn);
    return;
  };

  if (pVars->type == TYPE_POST) {
    supla_log(LOG_DEBUG, "Matched: %i, step: %i", pVars->matched, pVars->step);

    if (pVars->matched < 4) {
      return;
    }

    if (reboot > 0) {
      if (reboot == 2) {
        supla_system_restart_with_delay(1500);
      } else {
        supla_system_restart();
        return;
      }
    }

    // This also works for cfg->Password (union)
    if (new_cfg.LocationPwd[0] == 0)
      memcpy(new_cfg.LocationPwd, supla_esp_cfg.LocationPwd,
             SUPLA_LOCATION_PWD_MAXSIZE);

    if (new_cfg.WIFI_PWD[0] == 0)
      memcpy(new_cfg.WIFI_PWD, supla_esp_cfg.WIFI_PWD, WIFI_PWD_MAXSIZE);

    if (1 == supla_esp_cfg_save(&new_cfg)) {
      memcpy(&supla_esp_cfg, &new_cfg, sizeof(SuplaEspCfg));
      data_saved = 1;

#ifdef BOARD_ON_CFG_SAVED
      supla_esp_board_on_cfg_saved();
#endif

    } else {
      supla_esp_http_error(conn);
    }
  }

  if (false == wifi_get_macaddr(STATION_IF, (unsigned char *)mac)) {
    supla_esp_http_error(conn);
    return;
  }

  char dev_name[25];
  supla_esp_board_set_device_name(dev_name, 25);
  dev_name[24] = 0;

#ifdef BOARD_CFG_HTML_TEMPLATE
  char *buffer = supla_esp_board_cfg_html_template(dev_name, mac, data_saved);
#else
  char *buffer = supla_esp_cfgmode_get_html_template(dev_name, mac, data_saved);
#endif /*BOARD_CFG_HTML_TEMPLATE*/

  supla_log(LOG_DEBUG, "HTTP OK (%i) Free heap size: %i", buffer != NULL,
            system_get_free_heap_size());

  if (buffer) {
    supla_esp_http_ok((struct espconn *)arg, buffer);
  }
}

void ICACHE_FLASH_ATTR supla_esp_discon_callback(void *arg) {
  supla_log(LOG_DEBUG, "Disconnect");

  struct espconn *conn = (struct espconn *)arg;

  if (conn->reverse != NULL) {
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

  if (!(supla_esp_cfg.Flags & CFG_FLAG_MQTT_ENABLED)) {
    supla_esp_devconn_before_cfgmode_start();
  }

  wifi_get_macaddr(SOFTAP_IF, (unsigned char *)mac);

  struct softap_config apconfig;
  wifi_softap_get_config(&apconfig);

  memset(apconfig.ssid, 0, sizeof(apconfig.ssid));
  memset(apconfig.password, 0, sizeof(apconfig.password));

  struct espconn *conn;

  if (cfgmode_vars.entertime != 0) return;

  if (supla_esp_cfg.Flags & CFG_FLAG_MQTT_ENABLED) {
#ifdef MQTT_SUPPORT_ENABLED
    supla_esp_mqtt_client_stop();
#endif /*MQTT_SUPPORT_ENABLED*/
  } else {
    supla_esp_devconn_stop();
  }

  cfgmode_vars.entertime = system_get_time();

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

char ICACHE_FLASH_ATTR supla_esp_cfgmode_started(void) {
  return cfgmode_vars.entertime == 0 ? 0 : 1;
}

unsigned int ICACHE_FLASH_ATTR supla_esp_cfgmode_entertime(void) {
  return cfgmode_vars.entertime;
}
