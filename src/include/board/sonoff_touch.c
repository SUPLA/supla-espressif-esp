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

#define B_RELAY1_PORT 12
#define B_RELAY2_PORT 5
#define B_CFG_PORT 0
#define B_INPUT2_PORT 9

const uint8_t rsa_public_key_bytes[512] = {
    0xc8, 0x8a, 0x22, 0xad, 0x80, 0x18, 0xb0, 0x7f, 0xaa, 0x77, 0x3a, 0x6a,
    0x68, 0xb5, 0xf2, 0x81, 0xf4, 0x88, 0x7d, 0x50, 0x5c, 0x0f, 0x20, 0x18,
    0x71, 0x08, 0x8d, 0x95, 0x19, 0xb2, 0x8c, 0x33, 0x3e, 0x2b, 0xa1, 0xc9,
    0x16, 0x1c, 0x9b, 0xca, 0x41, 0xb6, 0x3c, 0xc5, 0x35, 0xce, 0x55, 0x65,
    0xad, 0xc2, 0x57, 0x08, 0xf5, 0x3c, 0x64, 0xac, 0x86, 0xf7, 0xac, 0x03,
    0x34, 0x33, 0xca, 0xa3, 0xce, 0xb6, 0x29, 0xcf, 0x7d, 0xe4, 0x3d, 0xd0,
    0x30, 0x10, 0xb1, 0x2e, 0x64, 0xc7, 0xb8, 0x70, 0x7b, 0x0d, 0x15, 0x25,
    0x86, 0xd9, 0xef, 0x58, 0x56, 0x66, 0x14, 0x78, 0xd8, 0x5d, 0xa0, 0x69,
    0x49, 0xb4, 0x86, 0x45, 0xc2, 0xf6, 0x2c, 0x2b, 0x93, 0x01, 0xed, 0x86,
    0x89, 0xe0, 0x5c, 0xd5, 0x05, 0xcf, 0x7d, 0x3a, 0x3b, 0xd6, 0xe8, 0xde,
    0x8e, 0xae, 0x06, 0x30, 0x45, 0x42, 0x7f, 0x60, 0x8a, 0x59, 0xf7, 0xd1,
    0x1a, 0x08, 0x7a, 0x79, 0xbe, 0x70, 0x0f, 0xcb, 0xaa, 0x92, 0x5b, 0x5e,
    0x86, 0x63, 0xf1, 0xfd, 0x15, 0x85, 0x1c, 0xf6, 0x09, 0x0a, 0xa1, 0x7a,
    0xb4, 0x64, 0x67, 0x77, 0x3a, 0xe0, 0x6e, 0x70, 0xd2, 0x1a, 0xe5, 0x33,
    0x23, 0xc9, 0xb9, 0xd8, 0x91, 0xff, 0xdb, 0x18, 0x33, 0x9a, 0x6f, 0x78,
    0x41, 0x30, 0x79, 0xc4, 0xeb, 0xc2, 0xa8, 0x69, 0xc2, 0x4d, 0x1a, 0xfb,
    0xfc, 0x67, 0x9d, 0x78, 0xe1, 0x30, 0xa8, 0xb2, 0x4d, 0xc6, 0x7d, 0x31,
    0xaa, 0x0e, 0x03, 0xe9, 0xb7, 0x77, 0x36, 0x1e, 0x76, 0xbf, 0xb8, 0x9d,
    0xee, 0x27, 0x11, 0x3c, 0x5c, 0xcb, 0xb6, 0x66, 0xb6, 0xce, 0x81, 0xa2,
    0xe1, 0x2f, 0xe8, 0x5f, 0xb7, 0x62, 0xf0, 0x90, 0x80, 0xa3, 0xdb, 0x6c,
    0x7e, 0x23, 0x46, 0x5e, 0xa3, 0x1a, 0x82, 0x2b, 0xcb, 0x62, 0x1a, 0xd7,
    0x8f, 0xf2, 0x5a, 0x07, 0xd9, 0xba, 0x43, 0xcc, 0x6b, 0x0d, 0xec, 0xe9,
    0x1e, 0x1e, 0x88, 0x69, 0xc1, 0x19, 0xa1, 0xf4, 0x10, 0x80, 0x14, 0xca,
    0x2f, 0x0a, 0x40, 0x5a, 0xdd, 0x2a, 0xc9, 0x09, 0x5d, 0xc4, 0xde, 0xb4,
    0x2e, 0x95, 0x34, 0x58, 0x03, 0x27, 0x37, 0x34, 0x12, 0x13, 0xe2, 0xfe,
    0x00, 0x78, 0x28, 0xd2, 0x85, 0x9a, 0x69, 0xfa, 0x49, 0x7b, 0x80, 0x5c,
    0x2f, 0x23, 0x32, 0xce, 0x82, 0xf4, 0xb2, 0x5c, 0x45, 0xa3, 0xce, 0x01,
    0x4d, 0x69, 0x24, 0xd8, 0x20, 0xff, 0xa3, 0x17, 0x84, 0xb2, 0x08, 0x0a,
    0x53, 0xde, 0x5f, 0xea, 0x98, 0xf8, 0x6e, 0x74, 0xa5, 0x31, 0x54, 0xd9,
    0x03, 0x20, 0xb3, 0x9f, 0xd6, 0x86, 0xf7, 0xf0, 0x87, 0x6f, 0x97, 0x28,
    0x11, 0xc0, 0x0b, 0xd9, 0x72, 0xe5, 0xcd, 0x2b, 0x59, 0x52, 0xb5, 0xff,
    0x68, 0xa6, 0xca, 0x1c, 0xac, 0xa0, 0xab, 0x31, 0x84, 0x9b, 0x84, 0x8c,
    0x25, 0x2a, 0x32, 0x4a, 0xf4, 0x57, 0xea, 0xb2, 0xf5, 0x46, 0x63, 0x9f,
    0xd0, 0xc5, 0x09, 0xc4, 0x74, 0x82, 0x2c, 0x96, 0x6c, 0x12, 0x2d, 0x14,
    0x0d, 0xe7, 0x55, 0xda, 0x85, 0xcf, 0x59, 0x57, 0xa4, 0xcf, 0x8a, 0x54,
    0x7b, 0x29, 0xcf, 0xe2, 0x47, 0xdd, 0x15, 0x34, 0xb3, 0xd3, 0xac, 0x5a,
    0x66, 0x75, 0x91, 0x26, 0x70, 0xbf, 0x5d, 0x41, 0x32, 0xe1, 0x7f, 0x3c,
    0x4b, 0x55, 0xb1, 0x17, 0x1c, 0x8a, 0x2b, 0x93, 0xfd, 0xcc, 0x72, 0x5d,
    0xec, 0x6c, 0x71, 0x42, 0x22, 0x02, 0x30, 0x3b, 0x6d, 0x25, 0x35, 0xff,
    0xb5, 0x8a, 0x47, 0xf5, 0x31, 0x43, 0x61, 0x5b, 0x36, 0x43, 0xc3, 0x47,
    0x0c, 0xde, 0x66, 0xa4, 0x73, 0x3c, 0x56, 0xbb, 0xb2, 0xb0, 0x3b, 0x99,
    0x09, 0x41, 0xfb, 0x56, 0xca, 0x89, 0xa4, 0x62, 0x36, 0xb6, 0x3d, 0xbb,
    0x72, 0x3f, 0x50, 0x82, 0x57, 0x21, 0xbc, 0xb3};

void supla_esp_board_set_device_name(char *buffer, uint8 buffer_size) {
  ets_snprintf(buffer, buffer_size, "SONOFF-TOUCH");
}

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
      "class=\"w\"><h3>Additional Settings</h3><i><select name=\"led\"><option "
      "value=\"0\" %s>LED "
      "ON<option value=\"1\" %s>LED OFF</select><label>Status - "
      "connected</label></i><i><select name=\"upd\"><option value=\"0\" "
      "%s>NO<option value=\"1\" %s>YES</select><label>Firmware "
      "update</label></i></div><button "
      "type=\"submit\">SAVE</button></form></div><br><br>";

  int bufflen = strlen(supla_esp_devconn_laststate()) + strlen(dev_name) +
                strlen(SUPLA_ESP_SOFTVER) + strlen(supla_esp_cfg.WIFI_SSID) +
                strlen(supla_esp_cfg.Server) + strlen(supla_esp_cfg.Email) +
                strlen(html_template_header) + strlen(html_template) + 200;

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
      supla_esp_cfg.StatusLedOff == 0 ? "selected" : "",
      supla_esp_cfg.StatusLedOff == 1 ? "selected" : "",
      supla_esp_cfg.FirmwareUpdate == 0 ? "selected" : "",
      supla_esp_cfg.FirmwareUpdate == 1 ? "selected" : "");

  return buffer;
}

void supla_esp_board_gpio_init(void) {
  supla_input_cfg[0].type = INPUT_TYPE_BTN_MONOSTABLE;
  supla_input_cfg[0].gpio_id = B_CFG_PORT;
  supla_input_cfg[0].flags = INPUT_FLAG_PULLUP | INPUT_FLAG_CFG_BTN;
  supla_input_cfg[0].relay_gpio_id = B_RELAY1_PORT;
  supla_input_cfg[0].channel = 0;

  // ---------------------------------------
  // ---------------------------------------

  supla_relay_cfg[0].gpio_id = B_RELAY1_PORT;
  supla_relay_cfg[0].flags = RELAY_FLAG_RESTORE_FORCE;
  supla_relay_cfg[0].channel = 0;

#ifdef __BOARD_sonoff_touch_dual

  supla_input_cfg[1].type = INPUT_TYPE_BTN_MONOSTABLE;
  supla_input_cfg[1].gpio_id = B_INPUT2_PORT;
  supla_input_cfg[1].flags = INPUT_FLAG_PULLUP | INPUT_FLAG_CFG_BTN;
  supla_input_cfg[1].relay_gpio_id = B_RELAY2_PORT;
  supla_input_cfg[1].channel = 1;

  supla_relay_cfg[1].gpio_id = B_RELAY2_PORT;
  supla_relay_cfg[1].flags = RELAY_FLAG_RESTORE_FORCE;
  supla_relay_cfg[1].channel = 1;

#endif /*__BOARD_sonoff_touch_dual*/
}

void supla_esp_board_set_channels(TDS_SuplaDeviceChannel_B *channels,
                                  unsigned char *channel_count) {
#ifdef __BOARD_sonoff_touch_dual

  *channel_count = 3;

  channels[0].Number = 0;
  channels[0].Type = SUPLA_CHANNELTYPE_RELAY;

  channels[0].FuncList =
      SUPLA_BIT_FUNC_POWERSWITCH | SUPLA_BIT_FUNC_LIGHTSWITCH;

  channels[0].Default = SUPLA_CHANNELFNC_LIGHTSWITCH;
  channels[0].value[0] = supla_esp_gpio_relay_on(B_RELAY1_PORT);

  channels[1].Number = 1;
  channels[1].Type = channels[0].Type;
  channels[1].FuncList = channels[0].FuncList;
  channels[1].Default = channels[0].Default;
  channels[1].value[0] = supla_esp_gpio_relay_on(B_RELAY2_PORT);

  channels[2].Number = 2;
  channels[2].Type = SUPLA_CHANNELTYPE_THERMOMETERDS18B20;
  channels[2].FuncList = 0;
  channels[2].Default = 0;

  supla_get_temperature(channels[1].value);

#else /*__BOARD_sonoff_touch_dual*/

  *channel_count = 2;

  channels[0].Number = 0;
  channels[0].Type = SUPLA_CHANNELTYPE_RELAY;

  channels[0].FuncList =
      SUPLA_BIT_FUNC_POWERSWITCH | SUPLA_BIT_FUNC_LIGHTSWITCH;

  channels[0].Default = SUPLA_CHANNELFNC_LIGHTSWITCH;

  channels[0].value[0] = supla_esp_gpio_relay_on(B_RELAY1_PORT);

  channels[1].Number = 1;
  channels[1].Type = SUPLA_CHANNELTYPE_THERMOMETERDS18B20;

  channels[1].FuncList = 0;
  channels[1].Default = 0;

  supla_get_temperature(channels[1].value);

#endif
}

void ICACHE_FLASH_ATTR supla_esp_board_on_connect(void) {
  supla_esp_gpio_set_led(supla_esp_cfg.StatusLedOff, 0, 0);
}

void ICACHE_FLASH_ATTR
supla_esp_board_send_channel_values_with_delay(void *srpc) {
  supla_esp_channel_value_changed(0, supla_esp_gpio_relay_on(B_RELAY1_PORT));
}
