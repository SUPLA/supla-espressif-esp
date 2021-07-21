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

#include "supla-dev/log.h"
#include "supla_esp.h"
#include "supla_esp_cfg.h"
#include "supla_esp_state.h"

#if !defined(BOARD_CFG_HTML_TEMPLATE) && defined(MQTT_SUPPORT_ENABLED)
#ifndef PGM_READ_INLINED
#define pgm_read_with_offset(addr, res)                                       \
  __asm__(                                                                    \
      "extui\t%0, %1, 0, 2\n\t" /* Extract offset within word (in bytes) */   \
      "sub\t%1, %1, %0\n\t" /* Subtract offset from addr, yielding an aligned \
                               address */                                     \
      "l32i.n\t%1, %1, 0\n\t" /* Load word from aligned address */            \
      "ssa8l\t%0\n\t"         /* Prepare to shift by offset (in bits) */      \
      "src\t%0, %1, %1" /* Shift right; now the requested byte is the first   \
                           one */                                             \
      : "=r"(res), "+r"(addr))

static inline uint8_t pgm_read_byte_inlined(const void *addr) {
  uint32_t res;
  pgm_read_with_offset(addr, res);
  return res; /* Implicit cast to uint8_t masks the lower byte from the returned
                 word */
}
#else
PGM_READ_INLINED
#endif

const char supla_esp_cfgmode_html_header[] ICACHE_RODATA_ATTR =
    "<!DOCTYPE html><html lang=\"en\"><head><meta "
    "http-equiv=\"content-type\" content=\"text/html; "
    "charset=UTF-8\"><meta "
    "name=\"viewport\" "
    "content=\"width=device-width,initial-scale=1,maximum-scale=1,user-"
    "scalable=no\"><title>Configuration "
    "Page</"
    "title><style>body{font-size:14px;font-family:HelveticaNeue,"
    "\"Helvetica "
    "Neue\",HelveticaNeueRoman,HelveticaNeue-Roman,\"Helvetica Neue "
    "Roman\",TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;"
    "font-weight:400;font-stretch:normal;background:#00d151;color:#fff;"
    "line-"
    "height:20px;padding:0}.s{width:460px;margin:0 "
    "auto;margin-top:calc(50vh "
    "- 340px);border:solid 3px #fff;padding:0 10px "
    "10px;border-radius:3px}#l{display:block;max-width:150px;height:155px;"
    "margin:-80px auto 20px;background:#00d151;padding-right:5px}#l "
    "path{fill:#000}.w{margin:3px 0 16px;padding:5px "
    "0;border-radius:3px;background:#fff;box-shadow:0 1px 3px "
    "rgba(0,0,0,.3)}h1,h3{margin:10px "
    "8px;font-family:HelveticaNeueLight,HelveticaNeue-Light,\"Helvetica "
    "Neue "
    "Light\",HelveticaNeue,\"Helvetica "
    "Neue\",TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;"
    "font-weight:300;font-stretch:normal;color:#000;font-size:23px}h1{"
    "margin-"
    "bottom:14px;color:#fff}span{display:block;margin:10px 7px "
    "14px}i{display:block;font-style:normal;position:relative;border-"
    "bottom:"
    "solid 1px "
    "#00d151;height:42px}i:last-child{border:none}label{position:absolute;"
    "display:inline-block;top:0;left:8px;color:#00d151;line-height:41px;"
    "pointer-events:none}input,select{width:calc(100% - "
    "145px);border:none;font-size:16px;line-height:40px;border-radius:0;"
    "letter-spacing:-.5px;background:#fff;color:#000;padding-left:144px;-"
    "webkit-appearance:none;-moz-appearance:none;appearance:none;outline:0!"
    "important;height:40px}select{padding:0;float:right;margin:1px 3px 1px "
    "2px}button{width:100%;border:0;background:#000;padding:5px "
    "10px;font-size:16px;line-height:40px;color:#fff;border-radius:3px;box-"
    "shadow:0 1px 3px "
    "rgba(0,0,0,.3);cursor:pointer}.c{background:#ffe836;position:fixed;"
    "width:100%;line-height:80px;color:#000;top:0;left:0;box-shadow:0 1px "
    "3px "
    "rgba(0,0,0,.3);text-align:center;font-size:26px;z-index:100}@media "
    "all and (max-height:920px){.s{margin-top:80px}}@media all and "
    "(max-width:900px){.s{width:calc(100% - "
    "20px);margin-top:40px;border:none;padding:0 "
    "8px;border-radius:0}#l{max-width:80px;height:auto;margin:10px auto "
    "20px}h1,h3{font-size:19px}i{border:none;height:auto}label{display:"
    "block;"
    "margin:4px 0 "
    "12px;color:#00d151;font-size:13px;position:relative;line-height:18px}"
    "input,select{width:calc(100% - "
    "10px);font-size:16px;line-height:28px;padding:0 "
    "5px;border-bottom:solid "
    "1px "
    "#00d151}select{width:100%;float:none;margin:0}}#proto_supla{display: "
    "none}#proto_mqtt{display: "
    "none}</style><script>setTimeout(function(){var element =  "
    "document.getElementById('msg');if ( element != null ) "
    "element.style.visibility = \"hidden\";},3200); function "
    "protocolChanged(){var proto=document.getElementById('protocol'); var "
    "supla=document.getElementById('proto_supla'); var "
    "mqtt=document.getElementById('proto_mqtt'); if "
    "(proto.value=='1'){supla.style.display=\"none\"; "
    "mqtt.style.display=\"block\";}else{supla.style.display=\"block\"; "
    "mqtt.style.display=\"none\";}}function mAuthChanged(){var "
    "sauth=document.getElementById('sel_mauth'); var "
    "usr=document.getElementById('mauth_usr'); var "
    "pwd=document.getElementById('mauth_pwd'); var style = "
    "sauth.value=='1' ? 'block' : 'none'; usr.style.display=style; "
    "pwd.style.display=style;}function saveAndReboot(){var cfgform = "
    "document.getElementById(\"cfgform\"); cfgform.rbt.value = '2'; "
    "cfgform.submit();}</script><body "
    "onload=\"protocolChanged();mAuthChanged();\">";

#ifndef BOARD_CFG_HTML_CUSTOM_SVG
const char supla_esp_cfgmode_html_svg[] ICACHE_RODATA_ATTR =
    "<svg version=\"1.1\" id=\"l\" x=\"0\" y=\"0\" "
    "viewBox=\"0 0 200 200\" xml:space=\"preserve\"><path "
    "d=\"M59.3,2.5c18.1,0.6,31.8,8,40.2,23.5c3.1,5.7,4.3,11.9,4.1,18.3c-0.1,"
    "3.6-0.7,7.1-1.9,10.6c-0.2,0.7-0.1,1.1,0.6,1.5c12.8,7.7,25.5,15.4,38.3,"
    "23c2.9,1.7,5.8,3.4,8.7,5.3c1,0.6,1.6,0.6,2.5-0.1c4.5-3.6,9.8-5.3,15.7-5."
    "4c12.5-0.1,22.9,7.9,25.2,19c1.9,9.2-2.9,19.2-11.8,23.9c-8.4,4.5-16.9,4."
    "5-25.5,0.2c-0.7-0.3-1-0.2-1.5,0.3c-4.8,4.9-9.7,9.8-14.5,14.6c-5.3,5.3-"
    "10.6,10.7-15.9,16c-1.8,1.8-3.6,3.7-5.4,5.4c-0.7,0.6-0.6,1,0,1.6c3.6,3.4,"
    "5.8,7.5,6.2,12.2c0.7,7.7-2.2,14-8.8,18.5c-12.3,8.6-30.3,3.5-35-10.4c-2."
    "8-8.4,0.6-17.7,8.6-22.8c0.9-0.6,1.1-1,0.8-2c-2-6.2-4.4-12.4-6.6-18.6c-6."
    "3-17.6-12.7-35.1-19-52.7c-0.2-0.7-0.5-1-1.4-0.9c-12.5,0.7-23.6-2.6-33-"
    "10.4c-8-6.6-12.9-15-14.2-25c-1.5-11.5,1.7-21.9,9.6-30.7C32.5,8.9,42.2,4."
    "2,53.7,2.7c0.7-0.1,1.5-0.2,2.2-0.2C57,2.4,58.2,2.5,59.3,2.5z "
    "M76.5,81c0,0.1,0.1,0.3,0.1,0.6c1.6,6.3,3.2,12.6,4.7,18.9c4.5,17.7,8.9,"
    "35.5,13.3,53.2c0.2,0.9,0.6,1.1,1.6,0.9c5.4-1.2,10.7-0.8,15.7,1.6c0.8,0."
    "4,1.2,0.3,1.7-0.4c11.2-12.9,22.5-25.7,33.4-38.7c0.5-0.6,0.4-1,0-1.6c-5."
    "6-7.9-6.1-16.1-1.3-24.5c0.5-0.8,0.3-1.1-0.5-1.6c-9.1-4.7-18.1-9.3-27.2-"
    "14c-6.8-3.5-13.5-7-20.3-10.5c-0.7-0.4-1.1-0.3-1.6,0.4c-1.3,1.8-2.7,3.5-"
    "4.3,5.1c-4.2,4.2-9.1,7.4-14.7,9.7C76.9,80.3,76.4,80.3,76.5,81z "
    "M89,42.6c0.1-2.5-0.4-5.4-1.5-8.1C83,23.1,74.2,16.9,61.7,15.8c-10-0.9-18."
    "6,2.4-25.3,9.7c-8.4,9-9.3,22.4-2.2,32.4c6.8,9.6,19.1,14.2,31.4,11.9C79."
    "2,67.1,89,55.9,89,42.6z "
    "M102.1,188.6c0.6,0.1,1.5-0.1,2.4-0.2c9.5-1.4,15.3-10.9,11.6-19.2c-2.6-5."
    "9-9.4-9.6-16.8-8.6c-8.3,1.2-14.1,8.9-12.4,16.6C88.2,183.9,94.4,188.6,"
    "102.1,188.6z "
    "M167.7,88.5c-1,0-2.1,0.1-3.1,0.3c-9,1.7-14.2,10.6-10.8,18.6c2.9,6.8,11."
    "4,10.3,19,7.8c7.1-2.3,11.1-9.1,9.6-15.9C180.9,93,174.8,88.5,167.7,88."
    "5z\"/></svg>";
#endif /*BOARD_CFG_HTML_CUSTOM_SVG*/

const char supla_esp_cfgmode_html_main[] ICACHE_RODATA_ATTR =
    "<h1>%s</h1><span>LAST STATE: %s<br>Firmware: %s<br>GUID: "
    "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X<br>MAC:"
    " %02X:%02X:%02X:%02X:%02X:%02X</span><form id=\"cfgform\" "
    "method=\"post\"><div class=\"w\"><h3>Wi-Fi Settings</h3><i><input "
    "name=\"sid\" value=\"%s\"><label>Network name</label></i><i><input "
    "name=\"wpw\"><label>Password</label></i></div><div "
    "class=\"w\"><i><select name=\"pro\" onchange=\"protocolChanged();\" "
    "id=\"protocol\"><option value=\"0\" %s>Supla</option><option "
    "value=\"1\" "
    "%s>MQTT</option></select><label>Protocol</label></i></div><div "
    "class=\"w\" id=\"proto_supla\"><h3>Supla Settings</h3><i><input "
    "name=\"svr\" value=\"%s\"><label>Server</label></i><i><input "
    "name=\"eml\" value=\"%s\"><label>E-mail</label></i></div><div "
    "class=\"w\" id=\"proto_mqtt\"><h3>MQTT Settings</h3><i><input "
    "name=\"mvr\" value=\"%s\"><label>Server</label></i><i><input "
    "name=\"prt\" min=\"1\" max=\"65535\" type=\"number\" "
    "value=\"%i\"><label>Port</label></i><i><select name=\"tls\"><option "
    "value=\"0\" %s>NO</option><option value=\"1\" "
    "%s>YES</option></select><label>TLS</label></i><i><select name=\"mau\" "
    "id=\"sel_mauth\" onchange=\"mAuthChanged();\"> <option value=\"0\" "
    "%s>NO</option> <option value=\"1\" "
    "%s>YES</option></select><label>Auth</label></i><i "
    "id=\"mauth_usr\"><input name=\"usr\" value=\"%s\" "
    "maxlength=\"45\"><label>Username</label></i><i id=\"mauth_pwd\"><input "
    "name=\"mwd\"><label>Password (Required)</label></i><i><input name=\"pfx\" "
    "value=\"%s\" maxlength=\"49\"><label>Topic prefix</label></i><i><input "
    "name=\"qos\" min=\"0\" max=\"2\" type=\"number\" "
    "value=\"%i\"><label>QoS</label></i><i><select name=\"ret\"><option "
    "value=\"0\" %s>YES</option><option value=\"1\" "
    "%s>NO</option></select><label>Retain</label></i><i><input name=\"ppd\" "
    "min=\"0\" max=\"%i\" type=\"number\" value=\"%i\"><label>Pool publication "
    "delay (sec.)</label></i></div>";

const char supla_esp_cfgmode_html_footer[] ICACHE_RODATA_ATTR =
    "<button "
    "type=\"submit\">SAVE</button><br><br><button type=\"button\" "
    "onclick=\"saveAndReboot();\">SAVE &amp; RESTART</button><input "
    "type=\"hidden\" name=\"rbt\" value=\"0\" "
    "/></form></div><br><br></body></html>!";

void ICACHE_FLASH_ATTR supla_esp_cfgmode_get_rostring(char *buf,
                                                      const char *addr) {
  char c = 0;
  uint32 n = 0;
  do {
    c = pgm_read_byte_inlined(addr + n);
    buf[n] = c;
    n++;
  } while (c);
}

uint32 ICACHE_FLASH_ATTR supla_esp_cfgmode_get_body(char *buffer,
                                                    uint32 buffer_size,
                                                    char dev_name[25],
                                                    const char mac[6]) {
  char *body_tmpl = malloc(sizeof(supla_esp_cfgmode_html_main));
  if (body_tmpl == NULL) {
    return 0;
  }

  supla_esp_cfgmode_get_rostring(body_tmpl, supla_esp_cfgmode_html_main);

  char c = 0;

  if (!buffer) {
    buffer = &c;
    buffer_size = 1;
  }

  uint32 result = ets_snprintf(
      buffer, buffer_size, body_tmpl, dev_name, supla_esp_get_laststate(),
      SUPLA_ESP_SOFTVER, (unsigned char)supla_esp_cfg.GUID[0],
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
      (unsigned char)supla_esp_cfg.GUID[15], (unsigned char)mac[0],
      (unsigned char)mac[1], (unsigned char)mac[2], (unsigned char)mac[3],
      (unsigned char)mac[4], (unsigned char)mac[5], supla_esp_cfg.WIFI_SSID,
      !(supla_esp_cfg.Flags & CFG_FLAG_MQTT_ENABLED) ? "selected" : "",
      (supla_esp_cfg.Flags & CFG_FLAG_MQTT_ENABLED) ? "selected" : "",
      supla_esp_cfg.Server, supla_esp_cfg.Email, supla_esp_cfg.Server,
      supla_esp_cfg.Port == 0 ? 1883 : supla_esp_cfg.Port,
      !(supla_esp_cfg.Flags & CFG_FLAG_MQTT_TLS) ? "selected" : "",
      (supla_esp_cfg.Flags & CFG_FLAG_MQTT_TLS) ? "selected" : "",
      (supla_esp_cfg.Flags & CFG_FLAG_MQTT_NO_AUTH) ? "selected" : "",
      !(supla_esp_cfg.Flags & CFG_FLAG_MQTT_NO_AUTH) ? "selected" : "",
      supla_esp_cfg.Username, supla_esp_cfg.MqttTopicPrefix,
      supla_esp_cfg.MqttQoS,
      !(supla_esp_cfg.Flags & CFG_FLAG_MQTT_NO_RETAIN) ? "selected" : "",
      (supla_esp_cfg.Flags & CFG_FLAG_MQTT_NO_RETAIN) ? "selected" : "",
      MQTT_POOL_PUBLICATION_MAX_DELAY, supla_esp_cfg.MqttPoolPublicationDelay);

  free(body_tmpl);
  return result;
}

uint32 ICACHE_FLASH_ATTR
supla_esp_board_cfg_html_additional_settings(char *buffer, uint32 buffer_size);

#ifdef BOARD_CFG_HTML_CUSTOM_SVG
uint32 ICACHE_FLASH_ATTR
supla_esp_board_cfg_html_custom_svg(char *buffer, uint32 buffer_size);
#endif /*BOARD_CFG_HTML_CUSTOM_SVG*/

#ifdef BOARD_CFG_HTML_CUSTOM_BG_COLOR
uint8 ICACHE_FLASH_ATTR supla_esp_board_cfg_html_custom_bg_color(char hex[6]);
#endif /*BOARD_CFG_HTML_CUSTOM_BG_COLOR*/

char *ICACHE_FLASH_ATTR supla_esp_cfgmode_get_html_template(
    char dev_name[25], const char mac[6], const char data_saved) {
  const char ds[] = "<div id=\"msg\" class=\"c\">Data saved</div>";
  const char div[] = "<div class=\"s\">";

  uint32 body_size = supla_esp_cfgmode_get_body(NULL, 0, dev_name, mac);

  char c = 0;
  uint32 addsett_size = supla_esp_board_cfg_html_additional_settings(&c, 1);

#ifdef BOARD_CFG_HTML_CUSTOM_SVG
  uint32 svg_size = supla_esp_board_cfg_html_custom_svg(&c, 1);
#else
  uint32 svg_size = sizeof(supla_esp_cfgmode_html_svg) - 1;
#endif /*BOARD_CFG_HTML_CUSTOM_SVG*/

  uint32 html_size = sizeof(supla_esp_cfgmode_html_header) + svg_size +
                     sizeof(ds) + sizeof(div) +
                     sizeof(supla_esp_cfgmode_html_footer) + body_size +
                     addsett_size;

  char *html = malloc(html_size);
  if (!html) {
    return 0;
  }

  uint32 offset = 0;
  memset(html, 0, html_size);

  supla_esp_cfgmode_get_rostring(html, supla_esp_cfgmode_html_header);
  offset += sizeof(supla_esp_cfgmode_html_header) - 1;

#ifdef BOARD_CFG_HTML_CUSTOM_BG_COLOR
  char hex[6] = {};
  if (supla_esp_board_cfg_html_custom_bg_color(hex)) {
    char pattern[7] = {'#', '0', '0', 'd', '1', '5', '1'};
    uint8 m = 0;
    for (uint32 a = 0; a < offset; a++) {
      if (html[a] == pattern[m]) {
        if (m == 6) {
          for (m = 0; m < 6; m++) {
            html[a - 5 + m] = hex[m];
          }
          m = 0;
        } else {
          m++;
        }
      } else {
        m = 0;
      }
    }
  }
#endif /*BOARD_CFG_HTML_CUSTOM_BG_COLOR*/

  if (data_saved) {
    memcpy(&html[offset], ds, sizeof(ds) - 1);
    offset += sizeof(ds) - 1;
  }

  memcpy(&html[offset], div, sizeof(div) - 1);
  offset += sizeof(div) - 1;

#ifdef BOARD_CFG_HTML_CUSTOM_SVG
  supla_esp_board_cfg_html_custom_svg(&html[offset], svg_size + 1);
#else
  supla_esp_cfgmode_get_rostring(&html[offset], supla_esp_cfgmode_html_svg);
#endif /*BOARD_CFG_HTML_CUSTOM_SVG*/

  offset += svg_size;

  supla_esp_cfgmode_get_body(&html[offset], html_size - offset, dev_name, mac);
  offset += body_size;

  if (addsett_size) {
    supla_esp_board_cfg_html_additional_settings(&html[offset],
                                                 html_size - offset);
    offset += addsett_size;
  }

  supla_esp_cfgmode_get_rostring(&html[offset], supla_esp_cfgmode_html_footer);

  return html;
}
#endif /*BOARD_CFG_HTML_TEMPLATE*/
