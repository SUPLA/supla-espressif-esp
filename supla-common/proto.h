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

#ifndef supla_proto_H_
#define supla_proto_H_

#ifdef _WIN32

#include <WinSock2.h>
#define _supla_int_t int

#elif defined(__AVR__)

#ifndef _TIMEVAL_DEFINED
#define _TIMEVAL_DEFINED

typedef long suseconds_t;
#define _TIME_T_ long
typedef _TIME_T_ time_t;

struct timeval {
  time_t tv_sec[2];
  suseconds_t tv_usec[2];
};
#endif
#define _supla_int_t long

#else
#include <sys/time.h>
#define _supla_int_t int
#endif

#ifdef __cplusplus
extern "C" {
#endif

// DCS - device/client -> server
// SDC - server -> device/client
// DS  - device -> server
// SD  - server -> device
// CS  - client -> server
// SC  - server -> client

#define SUPLA_PROTO_VERSION 9
#define SUPLA_PROTO_VERSION_MIN 1
#define SUPLA_TAG_SIZE 5
#if defined(__AVR__)
#define SUPLA_MAX_DATA_SIZE 1024
#define SUPLA_CHANNELGROUP_RELATION_PACK_MAXCOUNT 25
#elif defined(ESP8266)
#define SUPLA_MAX_DATA_SIZE 1536
#else
#define SUPLA_MAX_DATA_SIZE 10240
#endif
#define SUPLA_RC_MAX_DEV_COUNT 50
#define SUPLA_SOFTVER_MAXSIZE 21

#define SUPLA_GUID_SIZE 16
#define SUPLA_GUID_HEXSIZE 33
#define SUPLA_LOCATION_PWDHEX_MAXSIZE 65
#define SUPLA_LOCATION_PWD_MAXSIZE 33
#define SUPLA_ACCESSID_PWDHEX_MAXSIZE 65
#define SUPLA_ACCESSID_PWD_MAXSIZE 33
#define SUPLA_LOCATION_CAPTION_MAXSIZE 401
#define SUPLA_LOCATIONPACK_MAXCOUNT 20
#define SUPLA_CHANNEL_CAPTION_MAXSIZE 401
#define SUPLA_CHANNELPACK_MAXCOUNT 20
#define SUPLA_URL_HOST_MAXSIZE 101
#define SUPLA_URL_PATH_MAXSIZE 101
#define SUPLA_SERVER_NAME_MAXSIZE 65
#define SUPLA_EMAIL_MAXSIZE 256                 // ver. >= 7
#define SUPLA_EMAILHEX_MAXSIZE 513              // ver. >= 7
#define SUPLA_AUTHKEY_SIZE 16                   // ver. >= 7
#define SUPLA_AUTHKEY_HEXSIZE 33                // ver. >= 7
#define SUPLA_OAUTH_SERVER_MAXSIZE 256          // ver. >= 7
#define SUPLA_OAUTH_CLIENTID_MAXSIZE 276        // ver. >= 7
#define SUPLA_OAUTH_SECRET_MAXSIZE 276          // ver. >= 7
#define SUPLA_OAUTH_USERNAME_MAXSIZE 65         // ver. >= 7
#define SUPLA_OAUTH_PASSWORD_MAXSIZE 65         // ver. >= 7
#define SUPLA_CHANNELGROUP_PACK_MAXCOUNT 20     // ver. >= 9
#define SUPLA_CHANNELGROUP_CAPTION_MAXSIZE 401  // ver. >= 9
#define SUPLA_CHANNELVALUE_PACK_MAXCOUNT 20     // ver. >= 9

#ifndef SUPLA_CHANNELGROUP_RELATION_PACK_MAXCOUNT
#define SUPLA_CHANNELGROUP_RELATION_PACK_MAXCOUNT 100  // ver. >= 9
#endif /*SUPLA_CHANNELGROUP_RELATION_PACK_MAXCOUNT*/

#define SUPLA_DCS_CALL_GETVERSION 10
#define SUPLA_SDC_CALL_GETVERSION_RESULT 20
#define SUPLA_SDC_CALL_VERSIONERROR 30
#define SUPLA_DCS_CALL_PING_SERVER 40
#define SUPLA_SDC_CALL_PING_SERVER_RESULT 50
#define SUPLA_DS_CALL_REGISTER_DEVICE 60
#define SUPLA_DS_CALL_REGISTER_DEVICE_B 65  // ver. >= 2
#define SUPLA_DS_CALL_REGISTER_DEVICE_C 67  // ver. >= 6
#define SUPLA_DS_CALL_REGISTER_DEVICE_D 68  // ver. >= 7
#define SUPLA_SD_CALL_REGISTER_DEVICE_RESULT 70
#define SUPLA_CS_CALL_REGISTER_CLIENT 80
#define SUPLA_CS_CALL_REGISTER_CLIENT_B 85  // ver. >= 6
#define SUPLA_CS_CALL_REGISTER_CLIENT_C 86  // ver. >= 7
#define SUPLA_SC_CALL_REGISTER_CLIENT_RESULT 90
#define SUPLA_SC_CALL_REGISTER_CLIENT_RESULT_B 92  // ver. >= 9
#define SUPLA_DS_CALL_DEVICE_CHANNEL_VALUE_CHANGED 100
#define SUPLA_SD_CALL_CHANNEL_SET_VALUE 110
#define SUPLA_DS_CALL_CHANNEL_SET_VALUE_RESULT 120
#define SUPLA_SC_CALL_LOCATION_UPDATE 130
#define SUPLA_SC_CALL_LOCATIONPACK_UPDATE 140
#define SUPLA_SC_CALL_CHANNEL_UPDATE 150
#define SUPLA_SC_CALL_CHANNELPACK_UPDATE 160
#define SUPLA_SC_CALL_CHANNEL_VALUE_UPDATE 170
#define SUPLA_CS_CALL_GET_NEXT 180
#define SUPLA_SC_CALL_EVENT 190
#define SUPLA_CS_CALL_CHANNEL_SET_VALUE 200
#define SUPLA_CS_CALL_CHANNEL_SET_VALUE_B 205                // ver. >= 3
#define SUPLA_DCS_CALL_SET_ACTIVITY_TIMEOUT 210              // ver. >= 2
#define SUPLA_SDC_CALL_SET_ACTIVITY_TIMEOUT_RESULT 220       // ver. >= 2
#define SUPLA_DS_CALL_GET_FIRMWARE_UPDATE_URL 300            // ver. >= 5
#define SUPLA_SD_CALL_GET_FIRMWARE_UPDATE_URL_RESULT 310     // ver. >= 5
#define SUPLA_DCS_CALL_GET_REGISTRATION_ENABLED 320          // ver. >= 7
#define SUPLA_SDC_CALL_GET_REGISTRATION_ENABLED_RESULT 330   // ver. >= 7
#define SUPLA_CS_CALL_GET_OAUTH_PARAMETERS 340               // ver. >= 7
#define SUPLA_SC_CALL_GET_OAUTH_PARAMETERS_RESULT 350        // ver. >= 7
#define SUPLA_SC_CALL_CHANNELPACK_UPDATE_B 360               // ver. >= 8
#define SUPLA_SC_CALL_CHANNEL_UPDATE_B 370                   // ver. >= 8
#define SUPLA_SC_CALL_CHANNELGROUP_PACK_UPDATE 380           // ver. >= 9
#define SUPLA_SC_CALL_CHANNELGROUP_RELATION_PACK_UPDATE 390  // ver. >= 9
#define SUPLA_SC_CALL_CHANNELVALUE_PACK_UPDATE 400           // ver. >= 9
#define SUPLA_CS_CALL_SET_VALUE 410                          // ver. >= 9

#define SUPLA_RESULT_CALL_NOT_ALLOWED -5
#define SUPLA_RESULT_DATA_TOO_LARGE -4
#define SUPLA_RESULT_BUFFER_OVERFLOW -3
#define SUPLA_RESULT_DATA_ERROR -2
#define SUPLA_RESULT_VERSION_ERROR -1
#define SUPLA_RESULT_FALSE 0
#define SUPLA_RESULT_TRUE 1

#define SUPLA_RESULTCODE_NONE 0
#define SUPLA_RESULTCODE_UNSUPORTED 1
#define SUPLA_RESULTCODE_FALSE 2
#define SUPLA_RESULTCODE_TRUE 3
#define SUPLA_RESULTCODE_TEMPORARILY_UNAVAILABLE 4
#define SUPLA_RESULTCODE_BAD_CREDENTIALS 5
#define SUPLA_RESULTCODE_LOCATION_CONFLICT 6
#define SUPLA_RESULTCODE_CHANNEL_CONFLICT 7
#define SUPLA_RESULTCODE_DEVICE_DISABLED 8
#define SUPLA_RESULTCODE_ACCESSID_DISABLED 9
#define SUPLA_RESULTCODE_LOCATION_DISABLED 10
#define SUPLA_RESULTCODE_CLIENT_DISABLED 11
#define SUPLA_RESULTCODE_CLIENT_LIMITEXCEEDED 12
#define SUPLA_RESULTCODE_DEVICE_LIMITEXCEEDED 13
#define SUPLA_RESULTCODE_GUID_ERROR 14
#define SUPLA_RESULTCODE_HOSTNOTFOUND 15           // ver. >= 5
#define SUPLA_RESULTCODE_CANTCONNECTTOHOST 16      // ver. >= 5
#define SUPLA_RESULTCODE_REGISTRATION_DISABLED 17  // ver. >= 7
#define SUPLA_RESULTCODE_ACCESSID_NOT_ASSIGNED 18  // ver. >= 7
#define SUPLA_RESULTCODE_AUTHKEY_ERROR 19          // ver. >= 7
#define SUPLA_RESULTCODE_NO_LOCATION_AVAILABLE 20  // ver. >= 7
#define SUPLA_RESULTCODE_USER_CONFLICT 21          // ver. >= 7

#define SUPLA_DEVICE_NAME_MAXSIZE 201
#define SUPLA_DEVICE_NAMEHEX_MAXSIZE 401
#define SUPLA_CLIENT_NAME_MAXSIZE 201
#define SUPLA_CLIENT_NAMEHEX_MAXSIZE 401
#define SUPLA_SENDER_NAME_MAXSIZE 201

#define SUPLA_DEVICE_CFGDATA_MAXSIZE 512

#ifdef __AVR__
#ifdef __AVR_ATmega2560__
#define SUPLA_CHANNELMAXCOUNT 32
#else
#define SUPLA_CHANNELMAXCOUNT 1
#endif
#else
#define SUPLA_CHANNELMAXCOUNT 128
#endif

#define SUPLA_CHANNELVALUE_SIZE 8

#define SUPLA_CHANNELTYPE_SENSORNO 1000
#define SUPLA_CHANNELTYPE_SENSORNC 1010        // ver. >= 4
#define SUPLA_CHANNELTYPE_DISTANCESENSOR 1020  // ver. >= 5
#define SUPLA_CHANNELTYPE_CALLBUTTON 1500      // ver. >= 4
#define SUPLA_CHANNELTYPE_RELAYHFD4 2000
#define SUPLA_CHANNELTYPE_RELAYG5LA1A 2010
#define SUPLA_CHANNELTYPE_2XRELAYG5LA1A 2020
#define SUPLA_CHANNELTYPE_RELAY 2900
#define SUPLA_CHANNELTYPE_THERMOMETERDS18B20 3000
#define SUPLA_CHANNELTYPE_DHT11 3010   // ver. >= 4
#define SUPLA_CHANNELTYPE_DHT22 3020   // ver. >= 4
#define SUPLA_CHANNELTYPE_DHT21 3022   // ver. >= 5
#define SUPLA_CHANNELTYPE_AM2302 3030  // ver. >= 4
#define SUPLA_CHANNELTYPE_AM2301 3032  // ver. >= 5

#define SUPLA_CHANNELTYPE_THERMOMETER 3034            // ver. >= 8
#define SUPLA_CHANNELTYPE_HUMIDITYSENSOR 3036         // ver. >= 8
#define SUPLA_CHANNELTYPE_HUMIDITYANDTEMPSENSOR 3038  // ver. >= 8
#define SUPLA_CHANNELTYPE_WINDSENSOR 3042             // ver. >= 8
#define SUPLA_CHANNELTYPE_PRESSURESENSOR 3044         // ver. >= 8
#define SUPLA_CHANNELTYPE_RAINSENSOR 3048             // ver. >= 8
#define SUPLA_CHANNELTYPE_WEIGHTSENSOR 3050           // ver. >= 8
#define SUPLA_CHANNELTYPE_WEATHER_STATION 3100        // ver. >= 8

#define SUPLA_CHANNELTYPE_DIMMER 4000            // ver. >= 4
#define SUPLA_CHANNELTYPE_RGBLEDCONTROLLER 4010  // ver. >= 4
#define SUPLA_CHANNELTYPE_DIMMERANDRGBLED 4020   // ver. >= 4

#define SUPLA_CHANNELDRIVER_MCP23008 2

#define SUPLA_CHANNELFNC_NONE 0
#define SUPLA_CHANNELFNC_CONTROLLINGTHEGATEWAYLOCK 10
#define SUPLA_CHANNELFNC_CONTROLLINGTHEGATE 20
#define SUPLA_CHANNELFNC_CONTROLLINGTHEGARAGEDOOR 30
#define SUPLA_CHANNELFNC_THERMOMETER 40
#define SUPLA_CHANNELFNC_HUMIDITY 42
#define SUPLA_CHANNELFNC_HUMIDITYANDTEMPERATURE 45
#define SUPLA_CHANNELFNC_OPENINGSENSOR_GATEWAY 50
#define SUPLA_CHANNELFNC_OPENINGSENSOR_GATE 60
#define SUPLA_CHANNELFNC_OPENINGSENSOR_GARAGEDOOR 70
#define SUPLA_CHANNELFNC_NOLIQUIDSENSOR 80
#define SUPLA_CHANNELFNC_CONTROLLINGTHEDOORLOCK 90
#define SUPLA_CHANNELFNC_OPENINGSENSOR_DOOR 100
#define SUPLA_CHANNELFNC_CONTROLLINGTHEROLLERSHUTTER 110
#define SUPLA_CHANNELFNC_OPENINGSENSOR_ROLLERSHUTTER 120
#define SUPLA_CHANNELFNC_POWERSWITCH 130
#define SUPLA_CHANNELFNC_LIGHTSWITCH 140
#define SUPLA_CHANNELFNC_RING 150
#define SUPLA_CHANNELFNC_ALARM 160
#define SUPLA_CHANNELFNC_NOTIFICATION 170
#define SUPLA_CHANNELFNC_DIMMER 180
#define SUPLA_CHANNELFNC_RGBLIGHTING 190
#define SUPLA_CHANNELFNC_DIMMERANDRGBLIGHTING 200
#define SUPLA_CHANNELFNC_DEPTHSENSOR 210           // ver. >= 5
#define SUPLA_CHANNELFNC_DISTANCESENSOR 220        // ver. >= 5
#define SUPLA_CHANNELFNC_OPENINGSENSOR_WINDOW 230  // ver. >= 8
#define SUPLA_CHANNELFNC_MAILSENSOR 240            // ver. >= 8
#define SUPLA_CHANNELFNC_WINDSENSOR 250            // ver. >= 8
#define SUPLA_CHANNELFNC_PRESSURESENSOR 260        // ver. >= 8
#define SUPLA_CHANNELFNC_RAINSENSOR 270            // ver. >= 8
#define SUPLA_CHANNELFNC_WEIGHTSENSOR 280          // ver. >= 8
#define SUPLA_CHANNELFNC_WEATHER_STATION 290       // ver. >= 8
#define SUPLA_CHANNELFNC_STAIRCASETIMER 300        // ver. >= 8

#define SUPLA_BIT_RELAYFUNC_CONTROLLINGTHEGATEWAYLOCK 0x0001
#define SUPLA_BIT_RELAYFUNC_CONTROLLINGTHEGATE 0x0002
#define SUPLA_BIT_RELAYFUNC_CONTROLLINGTHEGARAGEDOOR 0x0004
#define SUPLA_BIT_RELAYFUNC_CONTROLLINGTHEDOORLOCK 0x0008
#define SUPLA_BIT_RELAYFUNC_CONTROLLINGTHEROLLERSHUTTER 0x0010
#define SUPLA_BIT_RELAYFUNC_POWERSWITCH 0x0020
#define SUPLA_BIT_RELAYFUNC_LIGHTSWITCH 0x0040
#define SUPLA_BIT_RELAYFUNC_STAIRCASETIMER 0x0080  // ver. >= 8

#define SUPLA_EVENT_CONTROLLINGTHEGATEWAYLOCK 10
#define SUPLA_EVENT_CONTROLLINGTHEGATE 20
#define SUPLA_EVENT_CONTROLLINGTHEGARAGEDOOR 30
#define SUPLA_EVENT_CONTROLLINGTHEDOORLOCK 40
#define SUPLA_EVENT_CONTROLLINGTHEROLLERSHUTTER 50
#define SUPLA_EVENT_POWERONOFF 60
#define SUPLA_EVENT_LIGHTONOFF 70
#define SUPLA_EVENT_STAIRCASETIMERONOFF 80  // ver. >= 9

#define SUPLA_URL_PROTO_HTTP 0x01
#define SUPLA_URL_PROTO_HTTPS 0x02

#define SUPLA_PLATFORM_UNKNOWN 0
#define SUPLA_PLATFORM_ESP8266 1

#define SUPLA_NEW_VALUE_TARGET_CHANNEL 0
#define SUPLA_NEW_VALUE_TARGET_GROUP 1

#pragma pack(push, 1)

typedef struct {
  char tag[SUPLA_TAG_SIZE];
  unsigned char version;
  unsigned _supla_int_t rr_id;  // Request/Response ID
  unsigned _supla_int_t call_type;
  unsigned _supla_int_t data_size;
  char data[SUPLA_MAX_DATA_SIZE];  // Last variable in struct!
} TSuplaDataPacket;

typedef struct {
  // server -> device|client
  unsigned char proto_version_min;
  unsigned char proto_version;
  char SoftVer[SUPLA_SOFTVER_MAXSIZE];
} TSDC_SuplaGetVersionResult;

typedef struct {
  // server -> device|client
  unsigned char server_version_min;
  unsigned char server_version;
} TSDC_SuplaVersionError;

typedef struct {
  // device|client -> server
  struct timeval now;
} TDCS_SuplaPingServer;

typedef struct {
  // server -> device|client
  struct timeval now;
} TSDC_SuplaPingServerResult;

typedef struct {
  // device|client -> server
  unsigned char activity_timeout;
} TDCS_SuplaSetActivityTimeout;

typedef struct {
  // server -> device|client
  unsigned char activity_timeout;
  unsigned char min;
  unsigned char max;
} TSDC_SuplaSetActivityTimeoutResult;

typedef struct {
  char value[SUPLA_CHANNELVALUE_SIZE];
  char sub_value[SUPLA_CHANNELVALUE_SIZE];  // For example sensor value
} TSuplaChannelValue;

typedef struct {
  // server -> client
  char EOL;  // End Of List
  _supla_int_t Id;
  unsigned _supla_int_t
      CaptionSize;  // including the terminating null byte ('\0')
  char Caption[SUPLA_LOCATION_CAPTION_MAXSIZE];  // Last variable in struct!
} TSC_SuplaLocation;

typedef struct {
  // server -> client
  _supla_int_t count;
  _supla_int_t total_left;
  TSC_SuplaLocation
      items[SUPLA_LOCATIONPACK_MAXCOUNT];  // Last variable in struct!
} TSC_SuplaLocationPack;

typedef struct {
  // device -> server
  unsigned char Number;
  _supla_int_t Type;
  char value[SUPLA_CHANNELVALUE_SIZE];
} TDS_SuplaDeviceChannel;

typedef struct {
  // device -> server

  _supla_int_t LocationID;
  char LocationPWD[SUPLA_LOCATION_PWD_MAXSIZE];  // UTF8

  char GUID[SUPLA_GUID_SIZE];
  char Name[SUPLA_DEVICE_NAME_MAXSIZE];  // UTF8
  char SoftVer[SUPLA_SOFTVER_MAXSIZE];

  unsigned char channel_count;
  TDS_SuplaDeviceChannel
      channels[SUPLA_CHANNELMAXCOUNT];  // Last variable in struct!
} TDS_SuplaRegisterDevice;

typedef struct {
  // device -> server

  unsigned char Number;
  _supla_int_t Type;

  _supla_int_t FuncList;
  _supla_int_t Default;

  char value[SUPLA_CHANNELVALUE_SIZE];
} TDS_SuplaDeviceChannel_B;  // ver. >= 2

typedef struct {
  // device -> server

  _supla_int_t LocationID;
  char LocationPWD[SUPLA_LOCATION_PWD_MAXSIZE];  // UTF8

  char GUID[SUPLA_GUID_SIZE];
  char Name[SUPLA_DEVICE_NAME_MAXSIZE];  // UTF8
  char SoftVer[SUPLA_SOFTVER_MAXSIZE];

  unsigned char channel_count;
  TDS_SuplaDeviceChannel_B
      channels[SUPLA_CHANNELMAXCOUNT];  // Last variable in struct!
} TDS_SuplaRegisterDevice_B;            // ver. >= 2

typedef struct {
  // device -> server

  _supla_int_t LocationID;
  char LocationPWD[SUPLA_LOCATION_PWD_MAXSIZE];  // UTF8

  char GUID[SUPLA_GUID_SIZE];
  char Name[SUPLA_DEVICE_NAME_MAXSIZE];  // UTF8
  char SoftVer[SUPLA_SOFTVER_MAXSIZE];

  char ServerName[SUPLA_SERVER_NAME_MAXSIZE];

  unsigned char channel_count;
  TDS_SuplaDeviceChannel_B
      channels[SUPLA_CHANNELMAXCOUNT];  // Last variable in struct!
} TDS_SuplaRegisterDevice_C;            // ver. >= 6

typedef struct {
  // device -> server

  char Email[SUPLA_EMAIL_MAXSIZE];  // UTF8
  char AuthKey[SUPLA_AUTHKEY_SIZE];

  char GUID[SUPLA_GUID_SIZE];

  char Name[SUPLA_DEVICE_NAME_MAXSIZE];  // UTF8
  char SoftVer[SUPLA_SOFTVER_MAXSIZE];

  char ServerName[SUPLA_SERVER_NAME_MAXSIZE];

  unsigned char channel_count;
  TDS_SuplaDeviceChannel_B
      channels[SUPLA_CHANNELMAXCOUNT];  // Last variable in struct!
} TDS_SuplaRegisterDevice_D;            // ver. >= 7

typedef struct {
  // server -> device

  _supla_int_t result_code;
  unsigned char activity_timeout;
  unsigned char version;
  unsigned char version_min;
} TSD_SuplaRegisterDeviceResult;

typedef struct {
  // device -> server

  unsigned char ChannelNumber;
  char value[SUPLA_CHANNELVALUE_SIZE];
} TDS_SuplaDeviceChannelValue;

typedef struct {
  // server -> device
  _supla_int_t SenderID;
  unsigned char ChannelNumber;
  unsigned _supla_int_t DurationMS;

  char value[SUPLA_CHANNELVALUE_SIZE];
} TSD_SuplaChannelNewValue;

typedef struct {
  // device -> server
  unsigned char ChannelNumber;
  _supla_int_t SenderID;
  char Success;
} TDS_SuplaChannelNewValueResult;

typedef struct {
  // server -> client

  char EOL;  // End Of List
  _supla_int_t Id;
  char online;

  TSuplaChannelValue value;
} TSC_SuplaChannelValue;

typedef struct {
  // server -> client

  _supla_int_t count;
  _supla_int_t total_left;

  TSC_SuplaChannelValue
      items[SUPLA_CHANNELVALUE_PACK_MAXCOUNT];  // Last variable in struct!
} TSC_SuplaChannelValuePack;                    // ver. >= 9

typedef struct {
  // server -> client
  char EOL;  // End Of List

  _supla_int_t Id;
  _supla_int_t LocationID;
  _supla_int_t Func;
  char online;

  TSuplaChannelValue value;

  unsigned _supla_int_t
      CaptionSize;  // including the terminating null byte ('\0')
  char Caption[SUPLA_CHANNEL_CAPTION_MAXSIZE];  // Last variable in struct!
} TSC_SuplaChannel;

typedef struct {
  // server -> client

  _supla_int_t count;
  _supla_int_t total_left;
  TSC_SuplaChannel
      items[SUPLA_CHANNELPACK_MAXCOUNT];  // Last variable in struct!
} TSC_SuplaChannelPack;

typedef struct {
  // server -> client
  char EOL;  // End Of List

  _supla_int_t Id;
  _supla_int_t LocationID;
  _supla_int_t Func;
  _supla_int_t AltIcon;
  unsigned _supla_int_t Flags;
  unsigned char ProtocolVersion;
  char online;

  TSuplaChannelValue value;

  unsigned _supla_int_t
      CaptionSize;  // including the terminating null byte ('\0')
  char Caption[SUPLA_CHANNEL_CAPTION_MAXSIZE];  // Last variable in struct!
} TSC_SuplaChannel_B;                           // ver. >= 8

typedef struct {
  // server -> client

  _supla_int_t count;
  _supla_int_t total_left;
  TSC_SuplaChannel_B
      items[SUPLA_CHANNELPACK_MAXCOUNT];  // Last variable in struct!
} TSC_SuplaChannelPack_B;                 // ver. >= 8

typedef struct {
  // server -> client
  char EOL;  // End Of List

  _supla_int_t Id;
  _supla_int_t LocationID;
  _supla_int_t Func;
  _supla_int_t AltIcon;
  unsigned _supla_int_t Flags;

  unsigned _supla_int_t
      CaptionSize;  // including the terminating null byte ('\0')
  char Caption[SUPLA_CHANNELGROUP_CAPTION_MAXSIZE];  // Last variable in struct!
} TSC_SuplaChannelGroup;                             // ver. >= 9

typedef struct {
  // server -> client

  _supla_int_t count;
  _supla_int_t total_left;
  TSC_SuplaChannelGroup
      items[SUPLA_CHANNELGROUP_PACK_MAXCOUNT];  // Last variable in struct!
} TSC_SuplaChannelGroupPack;                    // ver. >= 9

typedef struct {
  // server -> client
  char EOL;  // End Of List

  _supla_int_t ChannelGroupID;
  _supla_int_t ChannelID;
} TSC_SuplaChannelGroupRelation;  // ver. >= 9

typedef struct {
  // server -> client

  _supla_int_t count;
  _supla_int_t total_left;
  TSC_SuplaChannelGroupRelation items
      [SUPLA_CHANNELGROUP_RELATION_PACK_MAXCOUNT];  // Last variable in struct!
} TSC_SuplaChannelGroupRelationPack;                // ver. >= 9

typedef struct {
  // client -> server

  _supla_int_t AccessID;
  char AccessIDpwd[SUPLA_ACCESSID_PWD_MAXSIZE];  // UTF8

  char GUID[SUPLA_GUID_SIZE];
  char Name[SUPLA_CLIENT_NAME_MAXSIZE];  // UTF8
  char SoftVer[SUPLA_SOFTVER_MAXSIZE];
} TCS_SuplaRegisterClient;

typedef struct {
  // client -> server

  _supla_int_t AccessID;
  char AccessIDpwd[SUPLA_ACCESSID_PWD_MAXSIZE];  // UTF8

  char GUID[SUPLA_GUID_SIZE];
  char Name[SUPLA_CLIENT_NAME_MAXSIZE];  // UTF8
  char SoftVer[SUPLA_SOFTVER_MAXSIZE];

  char ServerName[SUPLA_SERVER_NAME_MAXSIZE];
} TCS_SuplaRegisterClient_B;  // ver. >= 6

typedef struct {
  // client -> server

  char Email[SUPLA_EMAIL_MAXSIZE];  // UTF8
  char AuthKey[SUPLA_AUTHKEY_SIZE];

  char GUID[SUPLA_GUID_SIZE];
  char Name[SUPLA_CLIENT_NAME_MAXSIZE];  // UTF8
  char SoftVer[SUPLA_SOFTVER_MAXSIZE];

  char ServerName[SUPLA_SERVER_NAME_MAXSIZE];
} TCS_SuplaRegisterClient_C;  // ver. >= 7

typedef struct {
  // server -> client

  _supla_int_t result_code;
  _supla_int_t ClientID;
  _supla_int_t LocationCount;
  _supla_int_t ChannelCount;
  unsigned char activity_timeout;
  unsigned char version;
  unsigned char version_min;
} TSC_SuplaRegisterClientResult;

typedef struct {
  // server -> client

  _supla_int_t result_code;
  _supla_int_t ClientID;
  _supla_int_t LocationCount;
  _supla_int_t ChannelCount;
  _supla_int_t ChannelGroupCount;
  _supla_int_t Flags;
  unsigned char activity_timeout;
  unsigned char version;
  unsigned char version_min;
} TSC_SuplaRegisterClientResult_B;  // ver. >= 9

typedef struct {
  // client -> server
  unsigned char ChannelId;
  char value[SUPLA_CHANNELVALUE_SIZE];
} TCS_SuplaChannelNewValue;  // Deprecated

typedef struct {
  // client -> server
  _supla_int_t ChannelId;
  char value[SUPLA_CHANNELVALUE_SIZE];
} TCS_SuplaChannelNewValue_B;

typedef struct {
  // client -> server
  _supla_int_t Id;
  char Target;  // SUPLA_NEW_VALUE_TARGET_
  char value[SUPLA_CHANNELVALUE_SIZE];
} TCS_SuplaNewValue;  // ver. >= 9

typedef struct {
  // server -> client
  _supla_int_t Event;
  _supla_int_t ChannelID;
  unsigned _supla_int_t DurationMS;

  _supla_int_t SenderID;
  unsigned _supla_int_t
      SenderNameSize;  // including the terminating null byte ('\0')
  char
      SenderName[SUPLA_SENDER_NAME_MAXSIZE];  // Last variable in struct! | UTF8
} TSC_SuplaEvent;

typedef struct {
  char Platform;

  int Param1;
  int Param2;
  int Param3;
  int Param4;
} TDS_FirmwareUpdateParams;

typedef struct {
  char available_protocols;
  char host[SUPLA_URL_HOST_MAXSIZE];
  int port;
  char path[SUPLA_URL_PATH_MAXSIZE];
} TSD_FirmwareUpdate_Url;

typedef struct {
  char exists;
  TSD_FirmwareUpdate_Url url;
} TSD_FirmwareUpdate_UrlResult;

typedef struct {
  unsigned int client_timestamp;    // time >= now == enabled
  unsigned int iodevice_timestamp;  // time >= now == enabled
} TSDC_RegistrationEnabled;

typedef struct {
  char password[SUPLA_OAUTH_PASSWORD_MAXSIZE];
} TCS_OAuthParametersRequest;

typedef struct {
  char server[SUPLA_OAUTH_SERVER_MAXSIZE];
  char username[SUPLA_OAUTH_USERNAME_MAXSIZE];
  char client_id[SUPLA_OAUTH_CLIENTID_MAXSIZE];
  char secret[SUPLA_OAUTH_SECRET_MAXSIZE];
} TSC_OAuthParameters;

#pragma pack(pop)

void *sproto_init(void);
void sproto_free(void *spd_ptr);

char sproto_in_buffer_append(void *spd_ptr, char *data,
                             unsigned _supla_int_t data_size);
char sproto_out_buffer_append(void *spd_ptr, TSuplaDataPacket *sdp);

char sproto_pop_in_sdp(void *spd_ptr, TSuplaDataPacket *sdp);
unsigned _supla_int_t sproto_pop_out_data(void *spd_ptr, char *buffer,
                                          unsigned _supla_int_t buffer_size);
char sproto_out_dataexists(void *spd_ptr);
char sproto_in_dataexists(void *spd_ptr);

unsigned char sproto_get_version(void *spd_ptr);
void sproto_set_version(void *spd_ptr, unsigned char version);
void sproto_sdp_init(void *spd_ptr, TSuplaDataPacket *sdp);
char sproto_set_data(TSuplaDataPacket *sdp, char *data,
                     unsigned _supla_int_t data_size,
                     unsigned _supla_int_t call_type);
TSuplaDataPacket *sproto_sdp_malloc(void *spd_ptr);
void sproto_sdp_free(TSuplaDataPacket *sdp);

void sproto_log_summary(void *spd_ptr);
void sproto_buffer_dump(void *spd_ptr, unsigned char in);

#ifdef __cplusplus
}
#endif

#endif /* supla_proto_H_ */
