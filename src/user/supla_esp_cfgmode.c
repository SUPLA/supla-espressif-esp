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
#define VAR_PPD 34  // MQTT Pool publication delay

#define VAR_TH1 35  // Overcurrent threshold 1
#define VAR_TH2 36  // Overcurrent threshold 2

#ifdef CFG_TIME_VARIABLES
#define VAR_T10 37
#define VAR_T11 38
#define VAR_T12 39
#define VAR_T13 40
#define VAR_T20 41
#define VAR_T21 42
#define VAR_T22 43
#define VAR_T23 44
#endif /*CFG_TIME_VARIABLES*/

#define VAR_SBT 45  // Staircase Button Type

#define VAR_BP0 46 // ButtonType[0]
#define VAR_BP1 47 // ButtonType[1]
#define VAR_BP2 48 // ButtonType[2]
#define VAR_BP3 49 // ButtonType[3]

#define VAR_BM0 50 // ButtonMode[0]
#define VAR_BM1 51 // ButtonMode[1]
#define VAR_BM2 52 // ButtonMode[2]
#define VAR_BM3 53 // ButtonMode[3]

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
  ETSTimer timer;
  unsigned int entertime;
  bool exit_after_timeout;
} _cfgmode_vars_t;

_cfgmode_vars_t cfgmode_vars = {};

char *ICACHE_FLASH_ATTR supla_esp_cfgmode_get_html_template(
    char dev_name[25], const char mac[6], const char data_saved);

void ICACHE_FLASH_ATTR supla_esp_http_send_response(struct espconn *pespconn,
                                                    const char *code,
                                                    char *html) {
  char header_tmpl[] =
      "HTTP/1.1 %s\r\nAccept-Ranges: bytes\r\nContent-Length: "
      "%i\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: "
      "close\r\n\r\n";

  char c = 0;
  uint32 content_size = html != NULL ? strlen(html) : 0;

  uint32 header_size = ets_snprintf(&c, 1, header_tmpl, code, content_size) + 1;

  char *header = os_malloc(header_size);
  if (!header) {
    return;
  }

  ets_snprintf(header, header_size, header_tmpl, code, content_size);

  c = espconn_sent(pespconn, (uint8 *)header, header_size - 1);

  free(header);

  if (c != 0 || content_size == 0) {
    return;
  }

  espconn_sent(pespconn, (uint8 *)html, content_size);
}

void ICACHE_FLASH_ATTR supla_esp_http_send__response(struct espconn *pespconn,
                                                     const char *code,
                                                     const char *html) {
  char *html_copy = NULL;
  if (html) {
    size_t size = strlen(html) + 1;
    char *html_copy = zalloc(size);
    ets_snprintf(html_copy, size, "%s", html);
  }

  supla_esp_http_send_response(pespconn, code, html_copy);
}

void ICACHE_FLASH_ATTR supla_esp_http_ok(struct espconn *pespconn, char *html) {
  supla_esp_http_send_response(pespconn, "200 OK", html);
}

void ICACHE_FLASH_ATTR supla_esp_http_404(struct espconn *pespconn) {
  supla_esp_http_send__response(pespconn, "404 Not Found", "Not Found");
}

void ICACHE_FLASH_ATTR supla_esp_http_error(struct espconn *pespconn) {
  supla_esp_http_send__response(pespconn, "500 Internal Server Error", "Error");
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

unsigned int ICACHE_FLASH_ATTR cfg_str2int(const char *str) {
  int result = 0;

  short s = 0;
  while (str[s] != 0) {
    if (str[s] >= '0' && str[s] <= '9') {
      result = result * 10 + str[s] - '0';
    }

    s++;
  }

  return result;
}

// Converts float with the precision specified by decLimit to int multiplied by
// 10 raised to the power of decLimit, so if decLimit == 2 then "3.1415" -> 314
unsigned int ICACHE_FLASH_ATTR cfg_str2centInt(const char *str,
                                               unsigned short decLimit) {
  int result = 0;

  int decimalPlaces = -1;
  for (int i = 0; str[i] != 0; i++) {
    if (str[i] >= '0' && str[i] <= '9') {
      result = result * 10 + str[i] - '0';
      if (decimalPlaces >= 0) {
        decimalPlaces++;
      }
    } else if (str[i] == '.' || str[i] == ',') {
      decimalPlaces++;
    }
    if (decimalPlaces >= decLimit) {
      break;
    }
  }

  if (decimalPlaces < 0) {
    decimalPlaces = 0;
  }

  while (decimalPlaces < decLimit) {
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
      pVars->pbuff[pVars->offset] = pdata[a];
      pVars->offset++;

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
  char tempPassword[SUPLA_EMAIL_MAXSIZE];

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
      char ppd[3] = {'p', 'p', 'd'};

      char th1[3] = {'t', 'h', '1'};
      char th2[3] = {'t', 'h', '2'};

      char sbt[3] = {'s', 'b', 't'};

#ifdef CFG_TIME_VARIABLES
      char t10[3] = {'t', '1', '0'};
      char t11[3] = {'t', '1', '1'};
      char t12[3] = {'t', '1', '2'};
      char t13[3] = {'t', '1', '3'};
      char t20[3] = {'t', '2', '0'};
      char t21[3] = {'t', '2', '1'};
      char t22[3] = {'t', '2', '2'};
      char t23[3] = {'t', '2', '3'};
#endif /*CFG_TIME_VARIABLES*/

      char bp0[3] = {'b', 'p', '0'};
      char bp1[3] = {'b', 'p', '1'};
      char bp2[3] = {'b', 'p', '2'};
      char bp3[3] = {'b', 'p', '3'};

      char bm0[3] = {'b', 'm', '0'};
      char bm1[3] = {'b', 'm', '1'};
      char bm2[3] = {'b', 'm', '2'};
      char bm3[3] = {'b', 'm', '3'};

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
          pVars->buff_size = SUPLA_EMAIL_MAXSIZE;
          pVars->pbuff = tempPassword;

        } else if (memcmp(btncfg, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_CFGBTN;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(btn1, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_BTN1;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(sbt, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_SBT;
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
          pVars->buff_size = SUPLA_EMAIL_MAXSIZE;
          pVars->pbuff = tempPassword;

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

        } else if (memcmp(ppd, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_PPD;
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

        } else if (memcmp(bp0, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_BP0;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(bp1, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_BP1;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(bp2, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_BP2;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(bp3, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_BP3;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(bm0, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_BM0;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(bm1, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_BM1;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(bm2, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_BM2;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(bm3, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_BM3;
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

        } else if (memcmp(t12, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_T12;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(t13, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_T13;
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

        } else if (memcmp(t22, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_T22;
          pVars->buff_size = 12;
          pVars->pbuff = pVars->intval;

        } else if (memcmp(t23, &pdata[a], 3) == 0) {
          pVars->current_var = VAR_T23;
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
          cfg->LocationID = cfg_str2int(pVars->intval);

        } else if (pVars->current_var == VAR_CFGBTN) {
          cfg->CfgButtonType = pVars->intval[0] - '0';

        } else if (pVars->current_var == VAR_BTN1) {
          cfg->Button1Type = pVars->intval[0] - '0';

        } else if (pVars->current_var == VAR_BTN2) {
          cfg->Button2Type = pVars->intval[0] - '0';

        } else if (pVars->current_var == VAR_SBT) {
          cfg->StaircaseButtonType = pVars->intval[0] - '0';

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
          int port = cfg_str2int(pVars->intval);
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

        } else if (pVars->current_var == VAR_PPD) {
          int poolPublicationDelay = cfg_str2int(pVars->intval);
          if (poolPublicationDelay >= 0 &&
              poolPublicationDelay <= MQTT_POOL_PUBLICATION_MAX_DELAY) {
            cfg->MqttPoolPublicationDelay = poolPublicationDelay;
          }
        } else if (pVars->current_var == VAR_TH1) {
          cfg->OvercurrentThreshold1 = cfg_str2centInt(pVars->intval, 2);
          supla_log(LOG_DEBUG, "Found TH1 = %d", cfg->OvercurrentThreshold1);

        } else if (pVars->current_var == VAR_TH2) {
          cfg->OvercurrentThreshold2 = cfg_str2centInt(pVars->intval, 2);
          supla_log(LOG_DEBUG, "Found TH2 = %d", cfg->OvercurrentThreshold2);

        } else if (pVars->current_var == VAR_BP0) {
          cfg->ButtonType[0] = pVars->intval[0] - '0';
        } else if (pVars->current_var == VAR_BP1) {
          cfg->ButtonType[1] = pVars->intval[0] - '0';
        } else if (pVars->current_var == VAR_BP2) {
          cfg->ButtonType[2] = pVars->intval[0] - '0';
        } else if (pVars->current_var == VAR_BP3) {
          cfg->ButtonType[3] = pVars->intval[0] - '0';

        } else if (pVars->current_var == VAR_BM0) {
          cfg->ButtonMode[0] = pVars->intval[0] - '0';
        } else if (pVars->current_var == VAR_BM1) {
          cfg->ButtonMode[1] = pVars->intval[0] - '0';
        } else if (pVars->current_var == VAR_BM2) {
          cfg->ButtonMode[2] = pVars->intval[0] - '0';
        } else if (pVars->current_var == VAR_BM3) {
          cfg->ButtonMode[3] = pVars->intval[0] - '0';

        }
#ifdef CFG_TIME_VARIABLES
        else if (pVars->current_var == VAR_T10) {
          if (CFG_TIME1_COUNT > 0) {
            cfg->Time1[0] =
                cfg_str2centInt(pVars->intval, CFG_TIME_VARIABLES_PRECISION);
          }
        } else if (pVars->current_var == VAR_T11) {
          if (CFG_TIME1_COUNT > 1) {
            cfg->Time1[1] =
                cfg_str2centInt(pVars->intval, CFG_TIME_VARIABLES_PRECISION);
          }
        } else if (pVars->current_var == VAR_T12) {
          if (CFG_TIME1_COUNT > 2) {
            cfg->Time1[2] =
                cfg_str2centInt(pVars->intval, CFG_TIME_VARIABLES_PRECISION);
          }
        } else if (pVars->current_var == VAR_T13) {
          if (CFG_TIME1_COUNT > 3) {
            cfg->Time1[3] =
                cfg_str2centInt(pVars->intval, CFG_TIME_VARIABLES_PRECISION);
          }
        } else if (pVars->current_var == VAR_T20) {
          if (CFG_TIME2_COUNT > 0) {
            cfg->Time2[0] =
                cfg_str2centInt(pVars->intval, CFG_TIME_VARIABLES_PRECISION);
          }
        } else if (pVars->current_var == VAR_T21) {
          if (CFG_TIME2_COUNT > 1) {
            cfg->Time2[1] =
                cfg_str2centInt(pVars->intval, CFG_TIME_VARIABLES_PRECISION);
          }
        } else if (pVars->current_var == VAR_T22) {
          if (CFG_TIME2_COUNT > 2) {
            cfg->Time2[2] =
                cfg_str2centInt(pVars->intval, CFG_TIME_VARIABLES_PRECISION);
          }
        } else if (pVars->current_var == VAR_T23) {
          if (CFG_TIME2_COUNT > 3) {
            cfg->Time2[3] =
                cfg_str2centInt(pVars->intval, CFG_TIME_VARIABLES_PRECISION);
          }
        }
#endif /*CFG_TIME_VARIABLES*/
        pVars->matched++;
        pVars->current_var = VAR_NONE;
      }
    }
  }

  // workaround for long passwords - if password is longer than max password
  // field length, then we keep part in Password field, and reset in Email
  // field, after null char
  if (tempPassword[0] != '\0') {
    int newPassLen = strnlen(tempPassword, SUPLA_EMAIL_MAXSIZE);
    int mailLen = strnlen(cfg->Email, SUPLA_EMAIL_MAXSIZE);
    int passwordMaxLength = SUPLA_LOCATION_PWD_MAXSIZE;

    if (newPassLen < passwordMaxLength) {
      strncpy(cfg->Password, tempPassword, newPassLen + 1);
      return;
    }

    memcpy(cfg->Password, tempPassword, passwordMaxLength);
    strncpy(cfg->Email + mailLen + 1, tempPassword + passwordMaxLength,
            SUPLA_EMAIL_MAXSIZE - mailLen - 1);
    cfg->Email[SUPLA_EMAIL_MAXSIZE - 1] = '\0';
    return;
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
  new_cfg.LocationPwd[0] = '\0';

  supla_esp_parse_request(pVars, pdata, len, &new_cfg, &reboot);

  if (pVars->type == TYPE_UNKNOWN) {
    supla_esp_http_404(conn);
    return;
  };

  if (pVars->type == TYPE_POST) {
    supla_log(LOG_DEBUG, "Matched: %i, step: %i reboot: %i", pVars->matched,
              pVars->step, reboot);

    if (pVars->matched < 4) {
      return;
    }

    if (reboot > 0) {
      if (reboot == 2) {
        supla_system_restart_with_delay(2500);
      } else {
        supla_system_restart();
        return;
      }
    }

    // This also works for cfg->Password (union)
    if (new_cfg.LocationPwd[0] == 0) {
      memcpy(new_cfg.LocationPwd, supla_esp_cfg.LocationPwd,
             SUPLA_LOCATION_PWD_MAXSIZE);

      // workaround for long passwords:
      if (supla_esp_cfg.LocationPwd[SUPLA_LOCATION_PWD_MAXSIZE - 1] != '\0') {
        memcpy(new_cfg.LocationPwd, supla_esp_cfg.LocationPwd,
               SUPLA_LOCATION_PWD_MAXSIZE);
        int oldMailLen = strnlen(supla_esp_cfg.Email, SUPLA_EMAIL_MAXSIZE);
        int newMailLen = strnlen(new_cfg.Email, SUPLA_EMAIL_MAXSIZE);
        if (oldMailLen < SUPLA_EMAIL_MAXSIZE &&
            newMailLen < SUPLA_EMAIL_MAXSIZE) {
          int partPasswordLen = strnlen(supla_esp_cfg.Email + oldMailLen + 1,
                                        SUPLA_EMAIL_MAXSIZE - oldMailLen - 1);
          if (partPasswordLen < SUPLA_EMAIL_MAXSIZE - oldMailLen - 1) {
            if (partPasswordLen >= SUPLA_EMAIL_MAXSIZE - newMailLen - 1) {
              partPasswordLen = SUPLA_EMAIL_MAXSIZE - newMailLen - 1;
            }
            memcpy(new_cfg.Email + newMailLen + 1,
                   supla_esp_cfg.Email + oldMailLen + 1, partPasswordLen + 1);
          }
        } else {
          // mail was too long, so truncate password:
          new_cfg.LocationPwd[SUPLA_LOCATION_PWD_MAXSIZE - 1] = '\0';
        }
      }
    }

    if (new_cfg.WIFI_PWD[0] == 0)
      memcpy(new_cfg.WIFI_PWD, supla_esp_cfg.WIFI_PWD, WIFI_PWD_MAXSIZE);

#ifdef _ROLLERSHUTTER_SUPPORT
    bool save_state = false;
    for (int idx = 0; idx < CFG_TIME1_COUNT; idx++) {
      if (supla_esp_gpio_rs_apply_new__times(idx, new_cfg.Time2[idx],
                                             new_cfg.Time1[idx], false)) {
        save_state = true;
      }
    }

    if (save_state) {
      supla_esp_save_state(0);
    }
#endif /*_ROLLERSHUTTER_SUPPORT*/

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

  if (buffer) {
    supla_esp_http_ok((struct espconn *)arg, buffer);
    free(buffer);

    supla_log(LOG_DEBUG, "HTTP OK (%i) Free heap size: %i", buffer != NULL,
              system_get_free_heap_size());
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
  os_timer_disarm(&cfgmode_vars.timer);

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

int ICACHE_FLASH_ATTR supla_esp_cfgmode_generate_ssid_name(char *name,
                                                           int max_length) {
  char APSSID[] = AP_SSID;
  char mac[6];

  wifi_get_macaddr(STATION_IF, (unsigned char *)mac);
  int apssid_len = strnlen(APSSID, max_length);

  memcpy(name, APSSID, apssid_len);

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

  if (apssid_len + mac_str_len > max_length) {
    apssid_len -= apssid_len + mac_str_len - max_length;
  }

  memcpy(&name[apssid_len], mac_str, mac_str_len);

  return mac_str_len + apssid_len;
}

void ICACHE_FLASH_ATTR supla_esp_cfgmode_timeout_exit(void *ptr) {
  supla_log(LOG_DEBUG, "Exit cfgmode after timeout with no AP connection");
  supla_system_restart();
}

void ICACHE_FLASH_ATTR supla_esp_cfgmode_enter_ap_mode(void *ptr) {
  struct softap_config apconfig;
  wifi_softap_get_config(&apconfig);

  memset(apconfig.ssid, 0, sizeof(apconfig.ssid));
  memset(apconfig.password, 0, sizeof(apconfig.password));

  int ssid_name_length = supla_esp_cfgmode_generate_ssid_name(
      (char *)apconfig.ssid, sizeof(apconfig.ssid));

  apconfig.ssid_len = ssid_name_length;
  apconfig.channel = 1;
  apconfig.authmode = AUTH_OPEN;
  apconfig.ssid_hidden = 0;
  apconfig.max_connection = 4;
  apconfig.beacon_interval = 100;

  wifi_set_opmode(SOFTAP_MODE);

  wifi_softap_set_config(&apconfig);

  struct espconn *conn = (struct espconn *)zalloc(sizeof(struct espconn));

  espconn_regist_time(conn, 5, 0);

  conn->type = ESPCONN_TCP;
  conn->state = ESPCONN_NONE;

  conn->proto.tcp = (esp_tcp *)zalloc(sizeof(esp_tcp));
  conn->proto.tcp->local_port = 80;

  espconn_regist_connectcb(conn, supla_esp_connectcb);
  espconn_accept(conn);

  if (cfgmode_vars.exit_after_timeout) {
    cfgmode_vars.exit_after_timeout = false;
    os_timer_disarm(&cfgmode_vars.timer);
    os_timer_setfn(&cfgmode_vars.timer,
        (os_timer_func_t *)supla_esp_cfgmode_timeout_exit, NULL);
    os_timer_arm(&cfgmode_vars.timer, 5*60*1000, 0); // 5 min, don't repeat
  }
}

void ICACHE_FLASH_ATTR supla_esp_cfgmode_start_with_timeout(void) {
  cfgmode_vars.exit_after_timeout = true;
  supla_esp_cfgmode_start();
}


void ICACHE_FLASH_ATTR supla_esp_cfgmode_start() {
#ifdef BOARD_BEFORE_CFGMODE_START
  supla_esp_board_before_cfgmode_start();
#endif

  if (!(supla_esp_cfg.Flags & CFG_FLAG_MQTT_ENABLED)) {
    supla_esp_devconn_before_cfgmode_start();
  }

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

  // Go into AP mode with a delay, otherwise stopping connections may cause a
  // crash.
  os_timer_disarm(&cfgmode_vars.timer);
  os_timer_setfn(&cfgmode_vars.timer,
                 (os_timer_func_t *)supla_esp_cfgmode_enter_ap_mode, NULL);
  os_timer_arm(&cfgmode_vars.timer, 1000, 0);
}

char ICACHE_FLASH_ATTR supla_esp_cfgmode_started(void) {
  return cfgmode_vars.entertime == 0 ? 0 : 1;
}

unsigned int ICACHE_FLASH_ATTR supla_esp_cfgmode_entertime(void) {
  return cfgmode_vars.entertime;
}

// method needed only in UT tests
void ICACHE_FLASH_ATTR supla_esp_cfgmode_clear_vars(void) {
  cfgmode_vars.entertime = 0;
  os_timer_disarm(&cfgmode_vars.timer);
}
