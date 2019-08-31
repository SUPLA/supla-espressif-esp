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

#include <gpio.h>

#define STORAGE_COUNT 2

typedef struct {
  char tag[3];
  _supla_int64_t counter[IMPULSE_COUNTER_COUNT];
  _supla_int64_t copy[IMPULSE_COUNTER_COUNT];
} _ic_storage_t;

typedef struct {
  uint8 gpio;
  uint8 ref_led_gpio;
  _supla_int64_t counter;
  uint8 input_last_value;
  uint32 input_last_time;
  uint32 impulse_time;
  ETSTimer ref_led_timer;
} _ic_counter;

uint8 storage_offset;
uint8 counter_changed;

ETSTimer storage_timer1;

_ic_counter counter[IMPULSE_COUNTER_COUNT];

// moj klucz
const uint8_t rsa_public_key_bytes[512] = {
    0xf1, 0xa2, 0x3b, 0xb2, 0x4d, 0x2e, 0x22, 0x42,
    0xa1, 0x75, 0x60, 0xc3, 0xa9, 0xbc, 0xb3, 0x77,
    0x82, 0xff, 0x55, 0x76, 0x09, 0x2d, 0x95, 0x06,
    0x32, 0x75, 0xc7, 0x8d, 0xda, 0x7f, 0x99, 0xb6,
    0xce, 0xd2, 0x32, 0x87, 0x6e, 0x6a, 0x71, 0x52,
    0x36, 0x5d, 0x17, 0xdb, 0x56, 0x30, 0xda, 0x44,
    0xa1, 0x01, 0x5a, 0x74, 0x9e, 0xaa, 0xdd, 0x79,
    0x46, 0xf2, 0x3c, 0x61, 0x3a, 0xd3, 0x5b, 0xaf,
    0xdd, 0x13, 0x1b, 0x6c, 0x41, 0xb3, 0x91, 0x3f,
    0x53, 0x09, 0xfb, 0xed, 0xa1, 0x13, 0x76, 0xb1,
    0x3d, 0x3d, 0xf3, 0x05, 0x56, 0x42, 0x3b, 0x31,
    0xc9, 0x46, 0x79, 0x4f, 0xd5, 0xef, 0x77, 0xcb,
    0xce, 0xa7, 0x1c, 0xdb, 0x59, 0x49, 0x7d, 0xd5,
    0x11, 0xd2, 0xec, 0xb2, 0x81, 0xd1, 0x5c, 0xa6,
    0x20, 0x7b, 0x9a, 0x16, 0x31, 0x92, 0x13, 0xfd,
    0xa4, 0xd2, 0x53, 0x89, 0x69, 0x6d, 0x18, 0xd3,
    0x18, 0x18, 0xe1, 0x35, 0xbb, 0xa9, 0xa4, 0xc7,
    0x89, 0x39, 0xc3, 0x21, 0xd3, 0xa2, 0x9a, 0x85,
    0xbc, 0x8c, 0x32, 0x69, 0xac, 0xf5, 0x0e, 0x41,
    0x42, 0xcc, 0xc3, 0xba, 0x69, 0x44, 0x49, 0x0f,
    0xd3, 0xd4, 0x32, 0xd4, 0x33, 0xf5, 0x85, 0xfd,
    0x5f, 0xe3, 0xf1, 0x24, 0x1d, 0xb2, 0x95, 0xee,
    0xc8, 0x3d, 0x40, 0xba, 0xc1, 0xf6, 0xe9, 0x1a,
    0xaa, 0xbb, 0x4c, 0x31, 0x4b, 0x33, 0xf6, 0xaa,
    0xad, 0x16, 0x5a, 0x02, 0xfb, 0xf0, 0x3e, 0xf5,
    0xba, 0xfa, 0xd4, 0x9b, 0xb3, 0x22, 0xf5, 0x68,
    0x85, 0x41, 0x90, 0x5c, 0x88, 0x33, 0x4c, 0x6e,
    0x6d, 0xed, 0xa2, 0x4a, 0xfe, 0xf1, 0x39, 0xca,
    0xbd, 0x7c, 0x6c, 0xb0, 0x2a, 0x2e, 0x91, 0x86,
    0xb6, 0x5a, 0x25, 0x23, 0x8f, 0xd2, 0xd2, 0xb5,
    0x91, 0x50, 0xff, 0x5b, 0xd3, 0x0f, 0x50, 0xf1,
    0x6f, 0x66, 0xbe, 0x0b, 0x60, 0x95, 0x24, 0x6e,
    0x7b, 0x48, 0xd3, 0x74, 0x5e, 0xf3, 0x91, 0x5c,
    0x69, 0xbf, 0xf5, 0x8a, 0xe8, 0xe3, 0xfd, 0xb2,
    0x19, 0x37, 0x2e, 0xc8, 0xe8, 0x2e, 0x28, 0xfe,
    0x63, 0x85, 0x1d, 0x1c, 0x40, 0xfe, 0xf0, 0x9a,
    0x60, 0x21, 0x76, 0x54, 0xdd, 0xd7, 0xd0, 0xf2,
    0x97, 0xa2, 0x98, 0xc3, 0x10, 0x60, 0x72, 0xbb,
    0xbc, 0xf6, 0x19, 0x6d, 0x58, 0x8d, 0x79, 0xe9,
    0xdf, 0xc5, 0x81, 0xe4, 0xb8, 0xb9, 0x92, 0x16,
    0xb8, 0xd1, 0x28, 0xb3, 0x35, 0x8d, 0xf3, 0x9e,
    0x8d, 0x1f, 0x6d, 0xaf, 0x5f, 0xef, 0x6f, 0x99,
    0xb8, 0xc1, 0xca, 0x5a, 0x8a, 0x3d, 0x25, 0x27,
    0xff, 0xc6, 0x09, 0xe6, 0x0c, 0x34, 0x72, 0x98,
    0x3b, 0x39, 0x14, 0x77, 0xbc, 0x96, 0x1a, 0xe4,
    0xc4, 0x97, 0x19, 0x56, 0xf9, 0xdb, 0xe7, 0x88,
    0x83, 0x00, 0x5e, 0xad, 0x3f, 0xca, 0x8a, 0x21,
    0x31, 0xca, 0x8e, 0xf2, 0x13, 0xd2, 0xec, 0x80,
    0xf4, 0x85, 0x8f, 0x5e, 0xd3, 0xf8, 0x56, 0x0a,
    0x9b, 0x2a, 0xee, 0xf6, 0x2a, 0xa5, 0x0c, 0x84,
    0x07, 0x48, 0xf6, 0x2f, 0xd4, 0xfe, 0x6d, 0xf3,
    0x0b, 0x8a, 0x53, 0xda, 0xfc, 0x4c, 0x24, 0xaf,
    0x24, 0xab, 0x4c, 0xf2, 0x24, 0x48, 0x35, 0xac,
    0x6d, 0x53, 0xf0, 0x69, 0x47, 0xcb, 0xf6, 0xb2,
    0x3b, 0xf2, 0x3c, 0x96, 0x3d, 0x71, 0x81, 0xd9,
    0x89, 0x25, 0xe2, 0x68, 0xb9, 0x93, 0x7a, 0x1f,
    0x36, 0x40, 0xb4, 0x04, 0x0c, 0x6a, 0x46, 0x30,
    0x56, 0x1a, 0x84, 0x88, 0x12, 0x6c, 0x61, 0xeb,
    0x99, 0x6e, 0xb8, 0xdb, 0x4d, 0x0f, 0xdc, 0xd9,
    0x43, 0x0d, 0xf1, 0x99, 0xc1, 0xf6, 0x98, 0xe4,
    0xa6, 0x0c, 0x2d, 0x8f, 0x35, 0x77, 0x6d, 0xd4,
    0x4f, 0x8f, 0x72, 0x9d, 0xc5, 0x1a, 0xe7, 0x0e,
    0x4b, 0x3d, 0x60, 0x1c, 0x1f, 0x01, 0x6a, 0x49,
    0x26, 0xb3, 0x92, 0x90, 0xa5, 0xd7, 0x4c, 0xd7
};

char *ICACHE_FLASH_ATTR supla_esp_board_cfg_html_template(
    char dev_name[25], const char mac[6], const char data_saved) {
  static char html_template_header[] =
      "<!DOCTYPE html><meta http-equiv=\"content-type\" content=\"text/html; "
      "charset=UTF-8\"><meta name=\"viewport\" "
      "content=\"width=device-width,initial-scale=1,maximum-scale=1,user-"
      "scalable=no\"><style>body{font-size:14px;font-family:HelveticaNeue,"
      "\"Helvetica Neue\",HelveticaNeueRoman,HelveticaNeue-Roman,\"Helvetica "
      "Neue "
      "Roman\",TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;"
      "font-weight:400;font-stretch:normal;background:#00d151;color:#fff;line-"
      "height:20px;padding:0}.s{width:460px;margin:0 auto;margin-top:calc(50vh "
      "- 340px);border:solid 3px #fff;padding:0 10px "
      "10px;border-radius:3px}#l{display:block;max-width:150px;height:155px;"
      "margin:-80px auto 20px;background:#00d151;padding-right:5px}#l "
      "path{fill:#000}.w{margin:3px 0 16px;padding:5px "
      "0;border-radius:3px;background:#fff;box-shadow:0 1px 3px "
      "rgba(0,0,0,.3)}h1,h3{margin:10px "
      "8px;font-family:HelveticaNeueLight,HelveticaNeue-Light,\"Helvetica Neue "
      "Light\",HelveticaNeue,\"Helvetica "
      "Neue\",TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;"
      "font-weight:300;font-stretch:normal;color:#000;font-size:23px}h1{margin-"
      "bottom:14px;color:#fff}span{display:block;margin:10px 7px "
      "14px}i{display:block;font-style:normal;position:relative;border-bottom:"
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
      "3px rgba(0,0,0,.3);text-align:center;font-size:26px;z-index:100}@media "
      "all and (max-height:920px){.s{margin-top:80px}}@media all and "
      "(max-width:900px){.s{width:calc(100% - "
      "20px);margin-top:40px;border:none;padding:0 "
      "8px;border-radius:0}#l{max-width:80px;height:auto;margin:10px auto "
      "20px}h1,h3{font-size:19px}i{border:none;height:auto}label{display:block;"
      "margin:4px 0 "
      "12px;color:#00d151;font-size:13px;position:relative;line-height:18px}"
      "input,select{width:calc(100% - "
      "10px);font-size:16px;line-height:28px;padding:0 5px;border-bottom:solid "
      "1px #00d151}select{width:100%;float:none;margin:0}}</style><script "
      "type=\"text/javascript\">setTimeout(function(){var element =  "
      "document.getElementById('msg');if ( element != null ) "
      "element.style.visibility = \"hidden\";},3200);</script>";
  static char html_template[] =
      "%s%s<div class=\"s\"><svg version=\"1.1\" id=\"l\" x=\"0\" y=\"0\" "
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
      "5z\"/></svg><h1>%s</h1><span>LAST STATE: %s<br>Firmware: %s<br>GUID: "
      "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X<br>MAC:"
      " %02X:%02X:%02X:%02X:%02X:%02X</span><form method=\"post\"><div "
      "class=\"w\"><h3>Wi-Fi Settings</h3><i><input name=\"sid\" "
      "value=\"%s\"><label>Network name</label></i><i><input "
      "name=\"wpw\"><label>Password</label></i></div><div "
      "class=\"w\"><h3>Supla Settings</h3><i><input name=\"svr\" "
      "value=\"%s\"><label>Server</label></i><i><input name=\"eml\" "
      "value=\"%s\"><label>E-mail</label></i></div><div "
      "class=\"w\"><h3>Additional Settings</h3><i><input name=\"t10\" "
      "type=\"number\" value=\"%i\"><label>IMPULSE TIME(us.) "
      "CH#0</label></i><i><input name=\"t11\" type=\"number\" "
      "value=\"%i\"><label>IMPULSE TIME(us.) CH#1</label></i><i><input "
      "name=\"t20\" "
      "type=\"number\" value=\"%i\"><label>IMPULSE TIME(us.) "
      "CH#2</label></i><i><select name=\"led\"><option value=\"0\" %s>LED "
      "ON<option value=\"1\" %s>LED OFF</select><label>Status - "
      "connected</label></i><i><select name=\"upd\"><option value=\"0\" "
      "%s>NO<option value=\"1\" %s>YES</select><label>Firmware "
      "update</label></i><i><input name=\"cmd\" "
      "value=\"%s\"><label>Reset</label></i></div><button "
      "type=\"submit\">SAVE</button></form></div><br><br>";

  int bufflen = strlen(supla_esp_devconn_laststate()) + strlen(dev_name) +
                strlen(SUPLA_ESP_SOFTVER) + strlen(supla_esp_cfg.WIFI_SSID) +
                strlen(supla_esp_cfg.Server) + strlen(supla_esp_cfg.Email) +
                strlen(html_template_header) + strlen(html_template) +
                (user_cmd ? strlen(user_cmd) : 0) + 200;

  char *buffer = (char *)malloc(bufflen);

  ets_snprintf(
      buffer, bufflen, html_template, html_template_header,
      data_saved == 1 ? "<div id=\"msg\" class=\"c\">Data saved</div>" : "",
      dev_name, supla_esp_devconn_laststate(), SUPLA_ESP_SOFTVER,
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
      (unsigned char)supla_esp_cfg.GUID[15], (unsigned char)mac[0],
      (unsigned char)mac[1], (unsigned char)mac[2], (unsigned char)mac[3],
      (unsigned char)mac[4], (unsigned char)mac[5], supla_esp_cfg.WIFI_SSID,
      supla_esp_cfg.Server, supla_esp_cfg.Email,
      supla_esp_cfg.Time1[0] > 0 ? supla_esp_cfg.Time1[0] : 50000,
      supla_esp_cfg.Time1[1] > 0 ? supla_esp_cfg.Time1[1] : 50000,
      supla_esp_cfg.Time2[0] > 0 ? supla_esp_cfg.Time2[0] : 50000,
      supla_esp_cfg.StatusLedOff == 0 ? "selected" : "",
      supla_esp_cfg.StatusLedOff == 1 ? "selected" : "",
      supla_esp_cfg.FirmwareUpdate == 0 ? "selected" : "",
      supla_esp_cfg.FirmwareUpdate == 1 ? "selected" : "",
      user_cmd ? user_cmd : "");

  return buffer;
}

uint8 ICACHE_FLASH_ATTR supla_esp_board_load(uint8 offset);
void ICACHE_FLASH_ATTR supla_esp_board_on_storage_timer(void *ptr);

void supla_esp_board_set_device_name(char *buffer, uint8 buffer_size) {
  ets_snprintf(buffer, buffer_size, "IMPULSE COUNTER 3");
}

void ICACHE_FLASH_ATTR supla_esp_board_starting(void) {
  storage_offset = 1;
  counter_changed = 0;

  if (supla_esp_board_load(1) == 0) {
    supla_esp_board_load(2);
  }
}

void supla_esp_board_gpio_init(void) {

  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);

  supla_esp_gpio_set_hi(REF_LED_PORT1, 0);
  supla_esp_gpio_set_hi(REF_LED_PORT2, 0);
  supla_esp_gpio_set_hi(REF_LED_PORT3, 0);

  memset(counter, 0, sizeof(_ic_counter));

  counter[0].gpio = IMPULSE_PORT1;
  counter[0].ref_led_gpio = REF_LED_PORT1;
  counter[0].impulse_time =
      supla_esp_cfg.Time1[0] > 0 ? supla_esp_cfg.Time1[0] : 50000;

  counter[1].gpio = IMPULSE_PORT2;
  counter[1].ref_led_gpio = REF_LED_PORT2;
  counter[1].impulse_time =
      supla_esp_cfg.Time1[1] > 0 ? supla_esp_cfg.Time1[1] : 50000;

  counter[2].gpio = IMPULSE_PORT3;
  counter[2].ref_led_gpio = REF_LED_PORT3;
  counter[2].impulse_time =
      supla_esp_cfg.Time2[0] > 0 ? supla_esp_cfg.Time2[0] : 50000;

  gpio16_output_conf();

  supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
  supla_input_cfg[0].gpio_id = B_CFG_PORT;
  supla_input_cfg[0].flags = INPUT_FLAG_PULLUP | INPUT_FLAG_CFG_BTN;

  for (int a = 0; a < IMPULSE_COUNTER_COUNT; a++) {
    supla_input_cfg[a + 1].type = INPUT_TYPE_CUSTOM;
    supla_input_cfg[a + 1].gpio_id = counter[a].gpio;

    counter[a].input_last_value = gpio__input_get(counter[a].gpio);
  }

  os_timer_disarm(&storage_timer1);
  os_timer_setfn(&storage_timer1,
                 (os_timer_func_t *)supla_esp_board_on_storage_timer, NULL);
  os_timer_arm(&storage_timer1, SAVE_INTERVAL, 1);
}

void ICACHE_FLASH_ATTR supla_esp_board_on_storage_timer(void *ptr) {
  if (counter_changed) {
    counter_changed = 0;
    storage_offset = storage_offset == 1 ? 2 : 1;

    _ic_storage_t storage;
    storage.tag[0] = 'I';
    storage.tag[1] = 'C';
    storage.tag[2] = 2;

    for (int a = 0; a < IMPULSE_COUNTER_COUNT; a++) {
      storage.counter[a] = counter[a].counter;
      storage.copy[a] = storage.counter[a];
    }

    ets_intr_lock();
    spi_flash_erase_sector(CFG_SECTOR + STATE_SECTOR_OFFSET + storage_offset);
    spi_flash_write((CFG_SECTOR + STATE_SECTOR_OFFSET + storage_offset) *
                        SPI_FLASH_SEC_SIZE,
                    (uint32 *)&storage, sizeof(_ic_storage_t));
    ets_intr_unlock();
  }
}

uint8 ICACHE_FLASH_ATTR supla_esp_board_load(uint8 offset) {
  _ic_storage_t storage;
  memset(&storage, 0, sizeof(_ic_storage_t));
  if (SPI_FLASH_RESULT_OK ==
      spi_flash_read(
          (CFG_SECTOR + STATE_SECTOR_OFFSET + offset) * SPI_FLASH_SEC_SIZE,
          (uint32 *)&storage, sizeof(_ic_storage_t))) {
    if (storage.tag[0] == 'I' && storage.tag[1] == 'C' && storage.tag[2] == 2) {
      for (int a = 0; a < IMPULSE_COUNTER_COUNT; a++) {
        if (storage.counter[a] != storage.copy[a]) {
          return 0;
        }
        counter[a].counter = storage.counter[a];
      }

      counter_changed = 0;
      return 1;
    }
  }

  return 0;
}

void ICACHE_FLASH_ATTR supla_esp_board_clear_measurements() {
  os_timer_disarm(&storage_timer1);
  supla_esp_ic_set_measurement_frequency(0);  // Disabled

  int a;
  for (a = 0; a < IMPULSE_COUNTER_COUNT; a++) {
    counter[a].counter = 0;
  }

  for (a = 0; a < 4; a++) {
    counter_changed = 1;
    supla_esp_board_on_storage_timer(NULL);
  }
}

void ICACHE_FLASH_ATTR supla_esp_ref_led_timer(void *ptr) {
  supla_esp_gpio_set_hi(((_ic_counter *)ptr)->ref_led_gpio, 0);
}

uint8 supla_esp_board_intr_handler(uint32 gpio_status) {
  uint8 result = 0;

  for (int a = 0; a < IMPULSE_COUNTER_COUNT; a++) {
    if (gpio_status & BIT(counter[a].gpio)) {
      gpio_pin_intr_state_set(GPIO_ID_PIN(counter[a].gpio),
                              GPIO_PIN_INTR_DISABLE);
      GPIO_REG_WRITE(
          GPIO_STATUS_W1TC_ADDRESS,
          gpio_status & BIT(counter[a].gpio));  // clear interrupt status

      uint8 v = gpio__input_get(counter[a].gpio);
      if (v != counter[a].input_last_value) {
        if (v == IMPULSE_TRIGGER_VALUE &&
            system_get_time() - counter[a].input_last_time >=
                counter[a].impulse_time) {
          counter[a].counter++;
          counter_changed = 1;

          if (counter[a].ref_led_gpio <= 16) {
            supla_esp_gpio_set_hi(counter[a].ref_led_gpio, 1);

            os_timer_disarm(&counter[a].ref_led_timer);
            os_timer_setfn(&counter[a].ref_led_timer,
                           (os_timer_func_t *)supla_esp_ref_led_timer,
                           &counter[a]);
            os_timer_arm(&counter[a].ref_led_timer, 100, 0);
          }
        }
        counter[a].input_last_value = v;
        counter[a].input_last_time = system_get_time();
      }

      gpio_pin_intr_state_set(GPIO_ID_PIN(counter[a].gpio),
                              GPIO_PIN_INTR_ANYEDGE);

      result = 1;
    }
  }

  return result;
}

uint8 ICACHE_FLASH_ATTR supla_esp_board_get_impulse_counter(
    unsigned char channel_number, TDS_ImpulseCounter_Value *icv) {
  if (channel_number < IMPULSE_COUNTER_COUNT) {
    icv->counter = counter[channel_number].counter;
    return 1;
  }
  return 0;
}

void supla_esp_board_set_channels(TDS_SuplaDeviceChannel_C *channels,
                                  unsigned char *channel_count) {
  *channel_count = IMPULSE_COUNTER_COUNT;

  for (int a = 0; a < IMPULSE_COUNTER_COUNT; a++) {
    channels[a].Number = a;
    channels[a].Type = SUPLA_CHANNELTYPE_IMPULSE_COUNTER;

    supla_esp_ic_get_value(a, channels[a].value);
  }

  channels[0].Default = SUPLA_CHANNELFNC_ELECTRICITY_METER;
}

void supla_esp_board_send_channel_values_with_delay(void *srpc) {}

void ICACHE_FLASH_ATTR supla_esp_board_on_connect(void) {
  supla_esp_gpio_set_led(!supla_esp_cfg.StatusLedOff, 0, 0);
}

void ICACHE_FLASH_ATTR supla_esp_board_on_cfg_saved(void) {
  if (user_cmd != NULL && strncmp(user_cmd, "RESET", CMD_MAXSIZE) == 0) {
    supla_esp_board_clear_measurements();
    snprintf(user_cmd, CMD_MAXSIZE, "OK");
  }
}
