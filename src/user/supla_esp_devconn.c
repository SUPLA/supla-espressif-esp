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

#include <user_interface.h>
#include <espconn.h>
#include <spi_flash.h>
#include <osapi.h>
#include <mem.h>
#include <stdlib.h>

#include "supla_esp_devconn.h"
#include "supla_esp_cfgmode.h"
#include "supla_esp_gpio.h"
#include "supla_esp_cfg.h"
#include "supla_esp_pwm.h"
#include "supla_esp_hw_timer.h"
#include "supla_ds18b20.h"
#include "supla_dht.h"
#include "supla-dev/srpc.h"
#include "supla-dev/log.h"
#include "supla_esp_countdown_timer.h"
#include "supla_esp_dns_client.h"
#include "supla_esp_wifi.h"
#include "supla_esp_state.h"

#ifdef ELECTRICITY_METER_COUNT
#include "supla_esp_electricity_meter.h"
#endif

#ifdef IMPULSE_COUNTER_COUNT
#include "supla_esp_impulse_counter.h"
#endif

#ifdef __FOTA
#include "supla_update.h"
#endif

#ifndef CVD_MAX_COUNT
#define CVD_MAX_COUNT 4
#endif /*CVD_MAX_COUNT*/

#if CVD_MAX_COUNT == 0
#undef CVD_MAX_COUNT
#endif

typedef struct {

	int channel_number;
	ETSTimer timer;
	char value[SUPLA_CHANNELVALUE_SIZE];

}channel_value_delayed;

typedef struct {

	ETSTimer supla_devconn_timer1;
	ETSTimer supla_iterate_timer;
	ETSTimer supla_watchdog_timer;
	ETSTimer supla_value_timer;
	ETSTimer reconnect_delay_timer;
	ETSTimer stop_delay_timer;

	// ESPCONN_INPROGRESS fix
	char esp_send_buffer[SEND_BUFFER_SIZE];
	int esp_send_buffer_len;
	// ---------------------------------------------

	struct espconn ESPConn;
	esp_tcp ESPTCP;
	ip_addr_t ipaddr;

	void *srpc;
	uint8 started;
	char registered;
	uint32 register_time_sec;

	char recvbuff[RECVBUFF_MAXSIZE];
	unsigned int recvbuff_size;

	unsigned int last_response;
	unsigned int last_sent;
	unsigned int next_wd_soft_timeout_challenge;
	int server_activity_timeout;

	#ifdef CVD_MAX_COUNT
	channel_value_delayed cvd[CVD_MAX_COUNT];
	#endif /*CVD_MAX_COUNT*/

	bool resolving_started;


}devconn_params;


static devconn_params *devconn = NULL;

#ifndef SUPLA_SMOOTH_DISABLED
#if defined(RGB_CONTROLLER_CHANNEL) \
    || defined(RGBW_CONTROLLER_CHANNEL) \
    || defined(RGBWW_CONTROLLER_CHANNEL) \
    || defined(DIMMER_CHANNEL)


typedef struct {

	char active;
	int counter;

	int ChannelNumber;
	char smoothly;

	int color;
	int dest_color;

	float color_brightness;
	float color_brightness_step;
	float dest_color_brightness;

	float brightness;
	float brightness_step;
	float dest_brightness;
	char turn_onoff;

}devconn_smooth;

devconn_smooth smooth[SMOOTH_MAX_COUNT];

#endif
#endif /*SUPLA_SMOOTH_DISABLED*/

#if NOSSL == 1
    #define supla_espconn_sent espconn_sent
    #define _supla_espconn_disconnect espconn_disconnect
    #define supla_espconn_connect espconn_connect
#else
    #define supla_espconn_sent espconn_secure_sent
	#define _supla_espconn_disconnect espconn_secure_disconnect
	#define supla_espconn_connect espconn_secure_connect
#endif


void DEVCONN_ICACHE_FLASH supla_esp_devconn_timer1_cb(void *timer_arg);
void DEVCONN_ICACHE_FLASH supla_esp_devconn_iterate(void *timer_arg);
void DEVCONN_ICACHE_FLASH supla_esp_devconn_reconnect(void);
void DEVCONN_ICACHE_FLASH supla_esp_devconn_reconnect_with_delay(uint32 time_ms);
void DEVCONN_ICACHE_FLASH supla_esp_devconn_stop_with_delay(void);

void DEVCONN_ICACHE_FLASH
supla_esp_devconn_before_system_restart(void) {

    if ( supla_esp_cfgmode_started() == 0 ) {

    	os_timer_disarm(&devconn->supla_watchdog_timer);
    	os_timer_disarm(&devconn->supla_devconn_timer1);
    	os_timer_disarm(&devconn->supla_iterate_timer);
    	os_timer_disarm(&devconn->supla_value_timer);

		#ifdef IMPULSE_COUNTER_COUNT
		supla_esp_ic_stop();
		#endif /*IMPULSE_COUNTER_COUNT*/

		#ifdef ELECTRICITY_METER_COUNT
		supla_esp_em_stop();
		#endif /*ELECTRICITY_METER_COUNT*/
    }
}

uint8 DEVCONN_ICACHE_FLASH supla_esp_devconn_any_outgoingdata_exists(void) {
  if (devconn) {
    if (devconn->esp_send_buffer_len > 0) {
      return 1;
    }

    if (devconn->srpc && (srpc_out_queue_item_count(devconn->srpc) > 0 ||
                          srpc_output_dataexists(devconn->srpc) > 0)) {
      return 1;
    }
  }

  return 0;
}

#pragma GCC diagnostic pop

int DEVCONN_ICACHE_FLASH
supla_esp_data_read(void *buf, int count, void *dcd) {


	if ( devconn->recvbuff_size > 0 ) {

		count = devconn->recvbuff_size > count ? count : devconn->recvbuff_size;
		os_memcpy(buf, devconn->recvbuff, count);

		if ( count == devconn->recvbuff_size ) {

			devconn->recvbuff_size = 0;

		} else {

			unsigned int a;

			for(a=0;a<devconn->recvbuff_size-count;a++)
				devconn->recvbuff[a] = devconn->recvbuff[count+a];

			devconn->recvbuff_size-=count;
		}

		return count;
	}

	return -1;
}

void DEVCONN_ICACHE_FLASH
supla_esp_devconn_recv_cb (void *arg, char *pdata, unsigned short len) {

	if ( len == 0 || pdata == NULL )
		return;

	//supla_log(LOG_DEBUG, "sproto recv %i bytes, heap_size: %i", len, system_get_free_heap_size());

	if ( len <= RECVBUFF_MAXSIZE-devconn->recvbuff_size ) {

		os_memcpy(&devconn->recvbuff[devconn->recvbuff_size], pdata, len);
		devconn->recvbuff_size += len;

		supla_esp_devconn_iterate(NULL);


	} else {
		supla_log(LOG_ERR, "Recv buffer size exceeded");
	}

}

int DEVCONN_ICACHE_FLASH
supla_esp_data_write_append_buffer(void *buf, int count) {

	if ( count > 0 ) {

		if ( devconn->esp_send_buffer_len+count > SEND_BUFFER_SIZE ) {

			supla_log(LOG_ERR, "Send buffer size exceeded");
			return -1;

		} else {

			memcpy(&devconn->esp_send_buffer[devconn->esp_send_buffer_len], buf, count);
			devconn->esp_send_buffer_len+=count;

			return 0;


		}
	}

	return 0;
}


int DEVCONN_ICACHE_FLASH
supla_esp_data_write(void *buf, int count, void *dcd) {

	int r;

	if ( devconn->esp_send_buffer_len > 0 ) {
		if ((r = supla_espconn_sent(&devconn->ESPConn,
				(unsigned char*)devconn->esp_send_buffer, devconn->esp_send_buffer_len)) == 0) {
			devconn->esp_send_buffer_len = 0;
			devconn->last_sent = uptime_sec();
		}

		//supla_log(LOG_DEBUG, "sproto send count: %i result: %i", count, r);
	};

	if ( devconn->esp_send_buffer_len > 0 ) {
		return supla_esp_data_write_append_buffer(buf, count);
	}

	if ( count > 0 ) {

		r = supla_espconn_sent(&devconn->ESPConn, buf, count);
		//supla_log(LOG_DEBUG, "sproto send count: %i result: %i", count, r);

		if ( ESPCONN_INPROGRESS == r  || ESPCONN_MAXNUM == r ) {
			return supla_esp_data_write_append_buffer(buf, count);
		} else {

			if ( r == 0 )
				devconn->last_sent = uptime_sec();

			return r == 0 ? count : -1;
		}

	}


	return 0;
}

void DEVCONN_ICACHE_FLASH
supla_esp_on_version_error(TSDC_SuplaVersionError *version_error) {

	supla_esp_set_state(LOG_ERR, "Protocol version error");
	supla_esp_devconn_stop_with_delay();
}

void DEVCONN_ICACHE_FLASH
supla_esp_devconn_send_channel_values_cb(void *ptr) {

	if ( supla_esp_devconn_is_registered() == 1 ) {

#ifdef _ROLLERSHUTTER_SUPPORT
		int a;

		for(a=0; a<RS_MAX_COUNT; a++) {
			if ( supla_rs_cfg[a].up != NULL
				   && supla_rs_cfg[a].down != NULL
				   && supla_rs_cfg[a].up->channel != 255 ) {

				supla_esp_channel_value_changed(supla_rs_cfg[a].up->channel, supla_esp_gpio_rs_get_current_position(&supla_rs_cfg[a]));
			}
		}
#endif /*_ROLLERSHUTTER_SUPPORT*/

		supla_esp_board_send_channel_values_with_delay(devconn->srpc);

	}

}

void DEVCONN_ICACHE_FLASH
supla_esp_devconn_send_channel_values_with__delay(int time_ms) {
	if (devconn) {
		os_timer_disarm(&devconn->supla_value_timer);
		os_timer_setfn(&devconn->supla_value_timer, (os_timer_func_t *)supla_esp_devconn_send_channel_values_cb, NULL);
		os_timer_arm(&devconn->supla_value_timer, time_ms, 0);
	}
}

void DEVCONN_ICACHE_FLASH
supla_esp_devconn_send_channel_values_with_delay(void) {
	supla_esp_devconn_send_channel_values_with__delay(1500);
}

void DEVCONN_ICACHE_FLASH
supla_esp_on_register_result(TSD_SuplaRegisterDeviceResult *register_device_result) {

	char *buff = NULL;

	switch(register_device_result->result_code) {
	case SUPLA_RESULTCODE_BAD_CREDENTIALS:
		supla_esp_set_state(LOG_ERR, "Bad credentials!");
		break;
	case SUPLA_RESULTCODE_TEMPORARILY_UNAVAILABLE:
		supla_esp_set_state(LOG_NOTICE, "Temporarily unavailable!");
		break;

	case SUPLA_RESULTCODE_LOCATION_CONFLICT:
		supla_esp_set_state(LOG_ERR, "Location conflict!");
		break;

	case SUPLA_RESULTCODE_CHANNEL_CONFLICT:
		supla_esp_set_state(LOG_ERR, "Channel conflict!");
		break;

	case SUPLA_RESULTCODE_REGISTRATION_DISABLED:
		supla_esp_set_state(LOG_ERR, "Registration disabled!");
		break;
	case SUPLA_RESULTCODE_AUTHKEY_ERROR:
		supla_esp_set_state(LOG_NOTICE, "Incorrect device AuthKey!");
		break;
	case SUPLA_RESULTCODE_NO_LOCATION_AVAILABLE:
		supla_esp_set_state(LOG_ERR, "No location available!");
		break;
	case SUPLA_RESULTCODE_USER_CONFLICT:
		supla_esp_set_state(LOG_ERR, "User conflict!");
		break;

	case SUPLA_RESULTCODE_TRUE:

		devconn->server_activity_timeout = register_device_result->activity_timeout;
		devconn->registered = 1;
		devconn->register_time_sec = uptime_sec();

		// supla_esp_gpio_state_connected()
		// should be called after setting
		// devconn->registered to 1
		supla_esp_gpio_state_connected();

		supla_esp_set_state(LOG_DEBUG, "Registered and ready.");
		supla_log(LOG_DEBUG, "Free heap size: %i", system_get_free_heap_size());

		if ( devconn->server_activity_timeout != ACTIVITY_TIMEOUT ) {

			TDCS_SuplaSetActivityTimeout at;
			at.activity_timeout = ACTIVITY_TIMEOUT;
			srpc_dcs_async_set_activity_timeout(devconn->srpc, &at);

		}

		#ifdef __FOTA
		supla_esp_check_updates(devconn->srpc);
		#endif

#ifndef COUNTDOWN_TIMER_DISABLED
#if ESP8266_SUPLA_PROTO_VERSION >= 12
    for (uint8 a = 0; a < RELAY_MAX_COUNT; a++)
      if (supla_relay_cfg[a].gpio_id != 255 &&
          supla_relay_cfg[a].channel_flags &
              SUPLA_CHANNEL_FLAG_COUNTDOWN_TIMER_SUPPORTED) {
        TSuplaChannelExtendedValue *ev = (TSuplaChannelExtendedValue *)malloc(
            sizeof(TSuplaChannelExtendedValue));
        if (ev != NULL) {
          supla_esp_countdown_get_state_ev(supla_relay_cfg[a].channel, ev);
          supla_esp_channel_extendedvalue_changed(supla_relay_cfg[a].channel, ev);
          free(ev);
        }
      }
#endif /*ESP8266_SUPLA_PROTO_VERSION >= 12*/
#endif /*COUNTDOWN_TIMER_DISABLED*/

		supla_esp_devconn_send_channel_values_with_delay();

		#ifdef ELECTRICITY_METER_COUNT
		supla_esp_em_device_registered();
		#endif

		#ifdef IMPULSE_COUNTER_COUNT
		supla_esp_ic_device_registered();
		#endif

		#ifdef BOARD_ON_DEVICE_REGISTERED
			BOARD_ON_DEVICE_REGISTERED;
		#endif

		return;

	case SUPLA_RESULTCODE_DEVICE_DISABLED:
		supla_esp_set_state(LOG_NOTICE, "Device is disabled!");
		break;

	case SUPLA_RESULTCODE_LOCATION_DISABLED:
		supla_esp_set_state(LOG_NOTICE, "Location is disabled!");
		break;

	case SUPLA_RESULTCODE_DEVICE_LIMITEXCEEDED:
		supla_esp_set_state(LOG_NOTICE, "Device limit exceeded!");
		break;

	case SUPLA_RESULTCODE_GUID_ERROR:
		supla_esp_set_state(LOG_NOTICE, "Incorrect device GUID!");
		break;

	default:
		buff = os_malloc(30);
		ets_snprintf(buff, 30, "Unknown code %i", register_device_result->result_code);
		supla_esp_set_state(LOG_NOTICE, buff);
		os_free(buff);
		buff = NULL;

		break;
	}

	supla_esp_devconn_stop_with_delay();
}

void DEVCONN_ICACHE_FLASH
supla_esp_channel_set_activity_timeout_result(TSDC_SuplaSetActivityTimeoutResult *result) {
	devconn->server_activity_timeout = result->activity_timeout;
}

char DEVCONN_ICACHE_FLASH supla_esp_devconn_is_registered(void) {
	return devconn != NULL
			&& devconn->srpc != NULL
			&& devconn->registered == 1 ? 1 : 0;
}

void DEVCONN_ICACHE_FLASH
supla_esp_channel_value__changed(int channel_number, char value[SUPLA_CHANNELVALUE_SIZE]) {

	if ( supla_esp_devconn_is_registered() ) {
		srpc_ds_async_channel_value_changed(devconn->srpc, channel_number, value);
	}

}

void DEVCONN_ICACHE_FLASH
supla_esp_channel_value_changed(int channel_number, char v) {

	if ( supla_esp_devconn_is_registered() == 1 ) {

		//supla_log(LOG_DEBUG, "supla_esp_channel_value_changed(%i, %i)", channel_number, v);

		char value[SUPLA_CHANNELVALUE_SIZE];
		memset(value, 0, SUPLA_CHANNELVALUE_SIZE);
		value[0] = v;

		srpc_ds_async_channel_value_changed(devconn->srpc, channel_number, value);
	}

}

void DEVCONN_ICACHE_FLASH
supla_esp_channel_extendedvalue_changed(unsigned char channel_number, TSuplaChannelExtendedValue *value) {

	if ( supla_esp_devconn_is_registered() == 1 ) {
        #ifdef BOARD_BEFORE_SENDING_THE_EXTENDEDVALUE
		   BOARD_BEFORE_SENDING_THE_EXTENDEDVALUE;
        #endif /*BOARD_BEFORE_SENDING_THE_EXTENDEDVALUE*/
		srpc_ds_async_channel_extendedvalue_changed(devconn->srpc, channel_number, value);
	}
}

#if defined(RGBW_CONTROLLER_CHANNEL) \
    || defined(RGBWW_CONTROLLER_CHANNEL) \
	|| defined(RGB_CONTROLLER_CHANNEL) \
    || defined(DIMMER_CHANNEL)

void DEVCONN_ICACHE_FLASH
supla_esp_channel_rgbw_to_value(char value[SUPLA_CHANNELVALUE_SIZE], int color, char color_brightness, char brightness) {

	memset(value, 0, SUPLA_CHANNELVALUE_SIZE);

	value[0] = brightness;
	value[1] = color_brightness;
	value[2] = (char)((color & 0x000000FF));       // BLUE
	value[3] = (char)((color & 0x0000FF00) >> 8);  // GREEN
	value[4] = (char)((color & 0x00FF0000) >> 16); // RED

}


void DEVCONN_ICACHE_FLASH
supla_esp_channel_value_changed_delayed_cb(void *timer_arg) {

	if ( supla_esp_devconn_is_registered() ) {
		srpc_ds_async_channel_value_changed(devconn->srpc, ((channel_value_delayed*)timer_arg)->channel_number, ((channel_value_delayed*)timer_arg)->value);
	}

}

#ifdef CVD_MAX_COUNT
void DEVCONN_ICACHE_FLASH
supla_esp_channel_rgbw_value_changed(int channel_number, int color, char color_brightness, char brightness) {

	if ( channel_number >= CVD_MAX_COUNT )
		return;

	devconn->cvd[channel_number].channel_number = channel_number;

	supla_esp_channel_rgbw_to_value(devconn->cvd[channel_number].value, color, color_brightness, brightness);

	os_timer_disarm(&devconn->cvd[channel_number].timer);
	os_timer_setfn(&devconn->cvd[channel_number].timer, (os_timer_func_t *)supla_esp_channel_value_changed_delayed_cb, &devconn->cvd[channel_number]);
	os_timer_arm(&devconn->cvd[channel_number].timer, 1500, 0);

}
#endif /*CVD_MAX_COUNT*/

#endif

char DEVCONN_ICACHE_FLASH
_supla_esp_channel_set_value(int port, char v, int channel_number) {

	char _v = v == 1 ? HI_VALUE : LO_VALUE;

	supla_esp_gpio_relay_hi(port, _v, 1);

	_v = supla_esp_gpio_relay_is_hi(port);

	supla_esp_channel_value_changed(channel_number, _v == HI_VALUE ? 1 : 0);

	return (v == 1 ? HI_VALUE : LO_VALUE) == _v;
}

#ifndef COUNTDOWN_TIMER_DISABLED
void DEVCONN_ICACHE_FLASH supla_esp_devconn_on_countdown_on_disarm(uint8 channel_number) {
	for(uint8 a=0;a<RELAY_MAX_COUNT;a++)
		if ( supla_relay_cfg[a].gpio_id != 255
			 && channel_number == supla_relay_cfg[a].channel ) {
			if (supla_relay_cfg[a].channel_flags & SUPLA_CHANNEL_FLAG_COUNTDOWN_TIMER_SUPPORTED) {
				TSuplaChannelExtendedValue *ev =
				     (TSuplaChannelExtendedValue *)malloc(sizeof(TSuplaChannelExtendedValue));
				if (ev != NULL) {
					supla_esp_countdown_get_state_ev(channel_number, ev);
					supla_esp_channel_extendedvalue_changed(channel_number, ev);
					free(ev);
				}
			}
			break;
		}
}
#endif /*COUNTDOWN_TIMER_DISABLED*/

void DEVCONN_ICACHE_FLASH supla_esp_devconn_on_countdown_timer_finish(uint8 gpio_id,
		uint8 channel_number, char target_value[SUPLA_CHANNELVALUE_SIZE]) {
	_supla_esp_channel_set_value(gpio_id, target_value[0] == 0 ? LO_VALUE : HI_VALUE, channel_number);
}

#if defined(RGB_CONTROLLER_CHANNEL) \
    || defined(RGBW_CONTROLLER_CHANNEL) \
    || defined(RGBWW_CONTROLLER_CHANNEL) \
    || defined(DIMMER_CHANNEL)


hsv DEVCONN_ICACHE_FLASH rgb2hsv(int rgb)
{
    hsv         out;
    double      min, max, delta;

    unsigned char r = (unsigned char)((rgb & 0x00FF0000) >> 16);
    unsigned char g = (unsigned char)((rgb & 0x0000FF00) >> 8);
    unsigned char b = (unsigned char)(rgb & 0x000000FF);

    min = r < g ? r : g;
    min = min  < b ? min  : b;

    max = r > g ? r : g;
    max = max  > b ? max  : b;

    out.v = max;
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0;
        return out;
    }
    if( max > 0.0 ) {
        out.s = (delta / max);
    } else {
        out.s = 0.0;
        out.h = -1;
        return out;
    }
    if( r >= max )
        out.h = ( g - b ) / delta;
    else
    if( g >= max )
        out.h = 2.0 + ( b - r ) / delta;
    else
        out.h = 4.0 + ( r - g ) / delta;

    out.h *= 60.0;

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}

int DEVCONN_ICACHE_FLASH hsv2rgb(hsv in)
{
    double      hh, p, q, t, ff;
    long        i;

    unsigned char r,g,b;
    int rgb = 0;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        r = in.v;
        g = in.v;
        b = in.v;

        rgb = r & 0xFF; rgb<<=8;
        rgb |= g & 0xFF; rgb<<=8;
        rgb |= b & 0xFF;

        return rgb;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        r = in.v; g = t; b = p;
        break;
    case 1:
        r = q; g = in.v; b = p;
        break;
    case 2:
        r = p; g = in.v; b = t;
        break;
    case 3:
        r = p; g = q; b = in.v;
        break;
    case 4:
        r = t; g = p; b = in.v;
        break;
    case 5:
    default:
        r = in.v; g = p; b = q;
        break;
    }

    rgb = r & 0xFF; rgb<<=8;
    rgb |= g & 0xFF; rgb<<=8;
    rgb |= b & 0xFF;

    return rgb;
}

#ifndef SUPLA_SMOOTH_DISABLED
void DEVCONN_ICACHE_FLASH supla_esp_devconn_smooth_brightness(float *brightness, float *dest_brightness, float *step) {

	if ( *brightness > *dest_brightness ) {

		 *brightness=*brightness - *step;

		 if ( *brightness < 0 )
			 *brightness = 0;

		 if ( *brightness < *dest_brightness )
			 *brightness = *dest_brightness;

	 } else if ( *brightness < *dest_brightness ) {

		 *brightness=*brightness + *step;

		 if ( *brightness > 100 )
			 *brightness = 100;

		 if ( *brightness > *dest_brightness )
			 *brightness = *dest_brightness;
	 }

	*step = (*step) * 1.05;

}


void DEVCONN_ICACHE_FLASH supla_esp_devconn_smooth_cb(devconn_smooth *_smooth) {


	 if ( _smooth->smoothly == 0 || _smooth->counter >= 100 ) {
		 _smooth->color = _smooth->dest_color;
		 _smooth->color_brightness = _smooth->dest_color_brightness;
		 _smooth->brightness = _smooth->dest_brightness;
	 }


	 supla_esp_devconn_smooth_brightness(&_smooth->color_brightness, &_smooth->dest_color_brightness, &_smooth->color_brightness_step);
	 supla_esp_devconn_smooth_brightness(&_smooth->brightness, &_smooth->dest_brightness, &_smooth->brightness_step);



	 if ( _smooth->color != _smooth->dest_color ) {

		 hsv c = rgb2hsv(_smooth->color);
		 hsv dc = rgb2hsv(_smooth->dest_color);

		 c.s = dc.s;
		 c.v = dc.v;

		 if ( c.h < dc.h ) {

			 if ( 360-dc.h+c.h < dc.h-c.h ) {

				 c.h -= 3.6;

				 if ( c.h < 0 )
					 c.h = 359;

			 } else {

				 c.h += 3.6;

				 if ( c.h >= dc.h )
					 c.h = dc.h;
			 }

		 } else if ( c.h > dc.h ) {

			 if ( 360-c.h+dc.h < c.h-dc.h ) {

				 c.h += 3.6;

				 if ( c.h > 359 )
					 c.h = 0;

			 } else {

				 c.h -= 3.6;

				 if ( c.h < dc.h )
					 c.h = dc.h;

			 }

		 }

		 if ( c.h == dc.h ) {
			 _smooth->color = _smooth->dest_color;
		 } else {
			 _smooth->color = hsv2rgb(c);
		 }

	 }


	 #ifdef RGBW_ONOFF_SUPPORT
		 supla_esp_board_set_rgbw_value(_smooth->ChannelNumber, &_smooth->color, &_smooth->color_brightness, &_smooth->brightness, _smooth->turn_onoff);
	 #else
		 supla_esp_board_set_rgbw_value(_smooth->ChannelNumber, &_smooth->color, &_smooth->color_brightness, &_smooth->brightness);
 	 #endif /*RGBW_ONOFF_SUPPORT*/
	 _smooth->counter++;

	 if ( _smooth->color == _smooth->dest_color
		  && _smooth->color_brightness == _smooth->dest_color_brightness
		  && _smooth->brightness == _smooth->dest_brightness ) {

		 _smooth->active = 0;
	 }


}

void
_supla_esp_devconn_smooth_cb(void) {


	int a;
	char job = 0;

	for(a=0;a<SMOOTH_MAX_COUNT;a++)
		if ( smooth[a].active == 1 ) {

			job = 1;
			supla_esp_devconn_smooth_cb(&smooth[a]);

		}

	if ( job == 0 ) {
		supla_esp_hw_timer_disarm();
	}

}

#endif /*SUPLA_SMOOTH_DISABLED*/

#ifdef RGBW_ONOFF_SUPPORT
void DEVCONN_ICACHE_FLASH
supla_esp_channel_set_rgbw_value(int ChannelNumber, int Color, char ColorBrightness, char Brightness, char TurnOnOff, char smoothly, char send_value_changed) {
#else
void DEVCONN_ICACHE_FLASH
supla_esp_channel_set_rgbw_value(int ChannelNumber, int Color, char ColorBrightness, char Brightness, char smoothly, char send_value_changed) {
#endif /*RGBW_ONOFF_SUPPORT*/

	RGBW_CHANNEL_LIMIT

	if ( ColorBrightness < 0 ) {
		ColorBrightness = 0;
	} else if ( ColorBrightness > 100 ) {
		ColorBrightness = 100;
	}

	if ( Brightness < 0 ) {
		Brightness = 0;
	} else if ( Brightness > 100 ) {
		Brightness = 100;
	}

#if defined(SUPLA_PWM_COUNT) || defined(SUPLA_SMOOTH_DISABLED)
	float _ColorBrightness = ColorBrightness;
	float _Brightness = Brightness;
	#ifdef RGBW_ONOFF_SUPPORT
	  supla_esp_board_set_rgbw_value(ChannelNumber, &Color, &_ColorBrightness, &_Brightness, TurnOnOff);
	#else
	  supla_esp_board_set_rgbw_value(ChannelNumber, &Color, &_ColorBrightness, &_Brightness);
	#endif /*RGBW_ONOFF_SUPPORT*/
    #ifdef CVD_MAX_COUNT
	 if ( send_value_changed ) {
		 supla_esp_channel_rgbw_value_changed(ChannelNumber, Color, ColorBrightness, Brightness);
	 }
    #endif /*CVD_MAX_COUNT*/
#else
	supla_esp_hw_timer_disarm();

	devconn_smooth *_smooth = &smooth[ChannelNumber];

	 _smooth->active = 1;
	 _smooth->counter = 0;
	 _smooth->ChannelNumber = ChannelNumber;
	 _smooth->smoothly = smoothly;

	 supla_esp_board_get_rgbw_value(ChannelNumber, &_smooth->color, &_smooth->color_brightness, &_smooth->brightness);

	 _smooth->color_brightness_step = abs(_smooth->color_brightness-ColorBrightness)/100.00;
	 _smooth->brightness_step = abs(_smooth->brightness-Brightness)/100.00;

	 _smooth->dest_color = Color;
	 _smooth->dest_color_brightness = ColorBrightness;
	 _smooth->dest_brightness = Brightness;

	 #ifdef RGBW_ONOFF_SUPPORT
	 _smooth->turn_onoff = TurnOnOff;
	 #endif /*RGBW_ONOFF_SUPPORT*/

     #ifdef CVD_MAX_COUNT
	 if ( send_value_changed ) {
		 supla_esp_channel_rgbw_value_changed(ChannelNumber, Color, ColorBrightness, Brightness);
	 }
     #endif /*CVD_MAX_COUNT*/

	supla_esp_hw_timer_init(FRC1_SOURCE, 1, NULL, _supla_esp_devconn_smooth_cb);
	supla_esp_hw_timer_arm(10000);
#endif

}

#endif

void DEVCONN_ICACHE_FLASH
supla_esp_channel_set_value(TSD_SuplaChannelNewValue *new_value) {

#ifdef BOARD_ON_CHANNEL_VALUE_SET
	 BOARD_ON_CHANNEL_VALUE_SET
#endif /*BOARD_ON_CHANNEL_VALUE_SET*/

#if defined(RGBW_CONTROLLER_CHANNEL) \
	|| defined(RGBWW_CONTROLLER_CHANNEL) \
	|| defined(RGB_CONTROLLER_CHANNEL) \
	|| defined(DIMMER_CHANNEL)

	unsigned char rgb_cn = 255;
	unsigned char dimmer_cn = 255;

    #ifdef RGBW_CONTROLLER_CHANNEL
	rgb_cn = RGBW_CONTROLLER_CHANNEL;
    #endif

	#ifdef RGBWW_CONTROLLER_CHANNEL
	rgb_cn = RGBWW_CONTROLLER_CHANNEL;
	dimmer_cn = RGBWW_CONTROLLER_CHANNEL+1;
	#endif

	#ifdef RGB_CONTROLLER_CHANNEL
	rgb_cn = RGB_CONTROLLER_CHANNEL;
	#endif

	#ifdef DIMMER_CHANNEL
	dimmer_cn = DIMMER_CHANNEL;
	#endif

	if ( new_value->ChannelNumber == rgb_cn
			|| new_value->ChannelNumber == dimmer_cn
			RGBW_CHANNEl_CMP ) {

		int Color = 0;
		char ColorBrightness = 0;
		char Brightness = 0;

		#ifdef RGBW_ONOFF_SUPPORT
		char TurnOnOff = new_value->value[5];
		#endif /*RGBW_ONOFF_SUPPORT*/

		Brightness = new_value->value[0];
		ColorBrightness = new_value->value[1];

		Color = ((int)new_value->value[4] << 16) & 0x00FF0000; // BLUE
		Color |= ((int)new_value->value[3] << 8) & 0x0000FF00; // GREEN
		Color |= (int)new_value->value[2] & 0x00000FF;         // RED

		if ( Brightness > 100 )
			Brightness = 0;

		if ( ColorBrightness > 100 )
			ColorBrightness = 0;

        #ifndef DONT_SAVE_STATE
		if (new_value->ChannelNumber < RS_MAX_COUNT) {
			#ifdef RGBW_ONOFF_SUPPORT
			if ( new_value->ChannelNumber == rgb_cn ) {
				supla_esp_state.color[new_value->ChannelNumber] = Color;

				if (TurnOnOff == 0) {
					supla_esp_state.color_brightness[new_value->ChannelNumber] = ColorBrightness;
					supla_esp_state.brightness[new_value->ChannelNumber] = Brightness;
					supla_esp_state.turnedOff[new_value->ChannelNumber] = 0;
				} else {
					supla_esp_state.turnedOff[new_value->ChannelNumber] = 0;

					if (ColorBrightness > 0) {
						ColorBrightness = supla_esp_state.color_brightness[new_value->ChannelNumber];
					} else {
						supla_esp_state.turnedOff[new_value->ChannelNumber] |= 0x1;
					}

					if ( Brightness > 0) {
						Brightness = supla_esp_state.brightness[new_value->ChannelNumber];
					} else {
						supla_esp_state.turnedOff[new_value->ChannelNumber] |= 0x2;
					}
				}
			} else if ( new_value->ChannelNumber == dimmer_cn
					RGBW_CHANNEl_CMP ) {

				if (TurnOnOff == 0) {
					supla_esp_state.brightness[new_value->ChannelNumber] = Brightness;
					supla_esp_state.turnedOff[new_value->ChannelNumber] = 0;
				} else {
					if ( Brightness > 0) {
						Brightness = supla_esp_state.brightness[new_value->ChannelNumber];
					} else {
						supla_esp_state.turnedOff[new_value->ChannelNumber] |= 0x2;
					}
				}
			}
			#else
		    if (new_value->ChannelNumber == rgb_cn) {
			    supla_esp_state.color[new_value->ChannelNumber] = Color;
			    supla_esp_state.color_brightness[new_value->ChannelNumber] =
			        ColorBrightness;
			    supla_esp_state.brightness[new_value->ChannelNumber] = Brightness;

			} else if (new_value->ChannelNumber == dimmer_cn RGBW_CHANNEl_CMP) {
			    supla_esp_state.brightness[new_value->ChannelNumber] = Brightness;
			}
			#endif /*RGBW_ONOFF_SUPPORT*/
		}
        #endif /*DONT_SAVE_STATE*/

		#ifdef RGBW_ONOFF_SUPPORT
		   supla_esp_channel_set_rgbw_value(new_value->ChannelNumber, Color, ColorBrightness, Brightness, TurnOnOff, 1, 1);
		#else
		   supla_esp_channel_set_rgbw_value(new_value->ChannelNumber, Color, ColorBrightness, Brightness, 1, 1);
		#endif /*RGBW_ONOFF_SUPPORT*/

		supla_esp_save_state(1000);

		return;
	}

#endif


	char v = new_value->value[0];
	int a;
	char Success = 0;

/*
    char buff[200];
    ets_snprintf(buff, 200, "set_value %i,%i,%i", new_value->value[0], new_value->ChannelNumber, new_value->SenderID);
	supla_esp_write_log(buff);
*/

	#ifdef _ROLLERSHUTTER_SUPPORT
		for(a=0;a<RS_MAX_COUNT;a++)
			if ( supla_rs_cfg[a].up != NULL
				 && supla_rs_cfg[a].down != NULL
				 && supla_rs_cfg[a].up->channel == new_value->ChannelNumber ) {


				int ct = (new_value->DurationMS & 0xFFFF)*100;
				int ot = ((new_value->DurationMS >> 16) & 0xFFFF)*100;

				if ( ct < 0 )
					ct = 0;

				if ( ot < 0 )
					ot = 0;

        supla_esp_gpio_rs_apply_new_times(a, ct, ot);

				if ( v >= 10 && v <= 110 ) {
					supla_esp_gpio_rs_add_task(a, v-10);
        } else if ( v == 1 ) {
          supla_esp_gpio_rs_set_relay(&supla_rs_cfg[a], RS_RELAY_DOWN, 1, 0);
        } else if ( v == 2 ) {
          supla_esp_gpio_rs_set_relay(&supla_rs_cfg[a], RS_RELAY_UP, 1, 0);
        } else {
          supla_esp_gpio_rs_set_relay(&supla_rs_cfg[a], RS_RELAY_OFF, 1, 0);
        }

				Success = 1;
				return;
			}
	#endif /*_ROLLERSHUTTER_SUPPORT*/

	for(a=0;a<RELAY_MAX_COUNT;a++)
		if ( supla_relay_cfg[a].gpio_id != 255
			 && new_value->ChannelNumber == supla_relay_cfg[a].channel ) {

			Success = _supla_esp_channel_set_value(supla_relay_cfg[a].gpio_id, v, new_value->ChannelNumber);
			break;
		}

	srpc_ds_async_set_channel_result(devconn->srpc, new_value->ChannelNumber, new_value->SenderID, Success);

#ifndef COUNTDOWN_TIMER_DISABLED

	supla_esp_countdown_timer_disarm(new_value->ChannelNumber);

	if ( new_value->DurationMS > 0 ) {

		for(a=0;a<RELAY_MAX_COUNT;a++)
			if ( supla_relay_cfg[a].gpio_id != 255
				 && new_value->ChannelNumber == supla_relay_cfg[a].channel ) {

				if (v == 1
					|| supla_relay_cfg[a].channel_flags & SUPLA_CHANNEL_FLAG_COUNTDOWN_TIMER_SUPPORTED) {

					char target_value[SUPLA_CHANNELVALUE_SIZE];
					memcpy(target_value, new_value->value, SUPLA_CHANNELVALUE_SIZE);
					target_value[0] = v ? 0 : 1;

					supla_esp_countdown_timer_countdown(new_value->DurationMS,
							supla_relay_cfg[a].gpio_id,
							supla_relay_cfg[a].channel,
							target_value,
							new_value->SenderID);

	                #if ESP8266_SUPLA_PROTO_VERSION >= 12
					if (supla_relay_cfg[a].channel_flags & SUPLA_CHANNEL_FLAG_COUNTDOWN_TIMER_SUPPORTED) {
						TSuplaChannelExtendedValue *ev =
						     (TSuplaChannelExtendedValue *)malloc(sizeof(TSuplaChannelExtendedValue));
						if (ev != NULL) {
							supla_esp_countdown_get_state_ev(new_value->ChannelNumber, ev);
							supla_esp_channel_extendedvalue_changed(new_value->ChannelNumber, ev);
							free(ev);
						}
					}
	                #endif /*ESP8266_SUPLA_PROTO_VERSION >= 12*/
				}
				break;
			}
	}
#endif /*COUNTDOWN_TIMER_DISABLED*/
}

#if ESP8266_SUPLA_PROTO_VERSION >= 13
void DEVCONN_ICACHE_FLASH supla_esp_channelgroup_set_value(
    TSD_SuplaChannelGroupNewValue *cg_new_value) {
#ifdef BOARD_ON_CHANNELGROUP_VALUE_SET
  BOARD_ON_CHANNELGROUP_VALUE_SET
#endif /*BOARD_ON_CHANNELGROUP_VALUE_SET*/

  TSD_SuplaChannelNewValue new_value;
  memset(&new_value, 0, sizeof(TSD_SuplaChannelNewValue));

  // Do not pass the SenderID to TSD_SuplaChannelNewValue
  new_value.SenderID = 0;
  new_value.ChannelNumber = cg_new_value->ChannelNumber;
  new_value.DurationMS = cg_new_value->DurationMS;
  memcpy(new_value.value, cg_new_value->value, SUPLA_CHANNELVALUE_SIZE);

  supla_esp_channel_set_value(&new_value);
}
#endif /*ESP8266_SUPLA_PROTO_VERSION >= 13*/

void DEVCONN_ICACHE_FLASH supla_esp_set_channel_result(
    unsigned char ChannelNumber, _supla_int_t SenderID, char Success) {
  if (supla_esp_devconn_is_registered()) {
    srpc_ds_async_set_channel_result(devconn->srpc, ChannelNumber, SenderID,
                                     Success);
  }
}

#if ESP8266_SUPLA_PROTO_VERSION >= 12 || defined(CHANNEL_STATE_TOOLS)
void DEVCONN_ICACHE_FLASH
supla_esp_get_channel_state(_supla_int_t ChannelNumber, _supla_int_t ReceiverID,
                            TDSC_ChannelState *state) {
  memset(state, 0, sizeof(TDSC_ChannelState));

  state->ChannelNumber = ChannelNumber;
  state->ReceiverID = ReceiverID;

  state->Fields = SUPLA_CHANNELSTATE_FIELD_UPTIME |
                  SUPLA_CHANNELSTATE_FIELD_CONNECTIONUPTIME;

  struct ip_info ipconfig;
  if (wifi_get_ip_info(STATION_IF, &ipconfig) && ipconfig.ip.addr != 0) {
    state->Fields |= SUPLA_CHANNELSTATE_FIELD_IPV4;
    state->IPv4 = ipconfig.ip.addr;
  }

  state->Uptime = uptime_sec();
  state->ConnectionUptime = uptime_sec() - devconn->register_time_sec;

  if (wifi_get_macaddr(STATION_IF, (unsigned char *)state->MAC)) {
    state->Fields |= SUPLA_CHANNELSTATE_FIELD_MAC;
  }

  sint8 rssi = wifi_station_get_rssi();
  if (rssi < 10) {
    state->Fields |= SUPLA_CHANNELSTATE_FIELD_WIFIRSSI;
    state->WiFiRSSI = rssi;

    state->Fields |= SUPLA_CHANNELSTATE_FIELD_WIFISIGNALSTRENGTH;
    if (rssi > -50) {
    	state->WiFiSignalStrength = 100;
    } else if (rssi <= -100) {
    	state->WiFiSignalStrength = 0;
    } else {
    	state->WiFiSignalStrength = 2 * (rssi + 100);
    }
  }

#ifdef BOARD_ON_CHANNEL_STATE_PREPARE
  BOARD_ON_CHANNEL_STATE_PREPARE
#endif /*BOARD_ON_CHANNEL_STATE_PREPARE*/
}
#endif /*ESP8266_SUPLA_PROTO_VERSION >= 12 || defined(CHANNEL_STATE_TOOLS)*/

#if ESP8266_SUPLA_PROTO_VERSION >= 12
void DEVCONN_ICACHE_FLASH
supla_esp_get_channel__state(void *_srpc, TCSD_ChannelStateRequest *request) {
  if (request == NULL || _srpc == NULL) {
    return;
  }

  TDSC_ChannelState state;
  supla_esp_get_channel_state(request->ChannelNumber, request->SenderID,
                              &state);

  srpc_csd_async_channel_state_result(_srpc, &state);
}

void DEVCONN_ICACHE_FLASH supla_esp_get_channel_functions(void) {
  if (supla_esp_devconn_is_registered()) {
    srpc_ds_async_get_channel_functions(devconn->srpc);
  }
}

void DEVCONN_ICACHE_FLASH supla_esp_channel_value__changed_b(
    int channel_number, char value[SUPLA_CHANNELVALUE_SIZE],
    unsigned char offline) {
  if (supla_esp_devconn_is_registered()) {
    srpc_ds_async_channel_value_changed_b(devconn->srpc, channel_number, value,
                                          offline);
  }
}

void DEVCONN_ICACHE_FLASH supla_esp_channel_value__changed_c(
    int channel_number, char value[SUPLA_CHANNELVALUE_SIZE],
    unsigned char offline, unsigned _supla_int_t validity_time_sec) {
  if (supla_esp_devconn_is_registered()) {
    srpc_ds_async_channel_value_changed_c(devconn->srpc, channel_number, value,
                                          offline, validity_time_sec);
  }
}
#endif /*ESP8266_SUPLA_PROTO_VERSION >= 12*/

void DEVCONN_ICACHE_FLASH supla_esp_on_remote_call_received(
    void *_srpc, unsigned int rr_id, unsigned int call_type, void *_dcd,
    unsigned char proto_version) {
  TsrpcReceivedData rd;
  char result;

  devconn->last_response = uptime_sec();

  // supla_log(LOG_DEBUG, "call_received");

  if (SUPLA_RESULT_TRUE == (result = srpc_getdata(_srpc, &rd, 0))) {
    switch (rd.call_type) {
      case SUPLA_SDC_CALL_VERSIONERROR:
        supla_esp_on_version_error(rd.data.sdc_version_error);
        break;
      case SUPLA_SD_CALL_REGISTER_DEVICE_RESULT:
        supla_esp_on_register_result(rd.data.sd_register_device_result);
        break;
      case SUPLA_SD_CALL_CHANNEL_SET_VALUE:
        supla_esp_channel_set_value(rd.data.sd_channel_new_value);
        break;
#if ESP8266_SUPLA_PROTO_VERSION >= 13
      case SUPLA_SD_CALL_CHANNELGROUP_SET_VALUE:
        supla_esp_channelgroup_set_value(rd.data.sd_channelgroup_new_value);
        break;
#endif /*ESP8266_SUPLA_PROTO_VERSION >= 13*/
      case SUPLA_SDC_CALL_SET_ACTIVITY_TIMEOUT_RESULT:
        supla_esp_channel_set_activity_timeout_result(
            rd.data.sdc_set_activity_timeout_result);
        break;
#ifdef __FOTA
      case SUPLA_SD_CALL_GET_FIRMWARE_UPDATE_URL_RESULT:
        supla_esp_update_url_result(rd.data.sc_firmware_update_url_result);
        break;
#endif /*__FOTA*/
#ifdef BOARD_CALCFG
      case SUPLA_SD_CALL_DEVICE_CALCFG_REQUEST:
        supla_esp_board_calcfg_request(rd.data.sd_device_calcfg_request);
        break;
#endif /*BOARD_CALCFG*/
#ifdef BOARD_ON_USER_LOCALTIME_RESULT
      case SUPLA_DCS_CALL_GET_USER_LOCALTIME_RESULT:
        supla_esp_board_on_user_localtime_result(
            rd.data.sdc_user_localtime_result);
        break;
#endif /*BOARD_ON_USER_LOCALTIME_RESULT*/
#if defined(BOARD_ON_CHANNEL_INT_PARAMS_RESULT) && ESP8266_SUPLA_PROTO_VERSION >= 14
      case SUPLA_SD_CALL_GET_CHANNEL_INT_PARAMS_RESULT:
        supla_esp_board_on_channel_int_params_result(
            rd.data.sd_channel_int_params);
        break;
#endif /*BOARD_ON_CHANNEL_INT_PARAMS_RESULT*/
#if ESP8266_SUPLA_PROTO_VERSION >= 12
      case SUPLA_CSD_CALL_GET_CHANNEL_STATE:
        supla_esp_get_channel__state(_srpc, rd.data.csd_channel_state_request);
        break;
#ifdef BOARD_ON_GET_CHANNEL_FUNCTIONS_RESULT
      case SUPLA_SD_CALL_GET_CHANNEL_FUNCTIONS_RESULT:
        supla_esp_board_on_get_channel_functions_result(
            rd.data.sd_channel_functions);
        break;
#endif /*BOARD_ON_GET_CHANNEL_FUNCTIONS_RESULT*/
#endif /*ESP8266_SUPLA_PROTO_VERSION >= 12*/
    }

    srpc_rd_free(&rd);

  } else if (result == SUPLA_RESULT_DATA_ERROR) {
    supla_log(LOG_DEBUG, "DATA ERROR!");
  }
}

void
supla_esp_devconn_iterate(void *timer_arg) {

	if ( devconn->srpc != NULL ) {

		if ( devconn->registered == 0 ) {
			devconn->registered = -1;

			if ( strlen(supla_esp_cfg.Email) > 0 ) {

				#if ESP8266_SUPLA_PROTO_VERSION >= 10
					TDS_SuplaRegisterDevice_E srd;
					memset(&srd, 0, sizeof(TDS_SuplaRegisterDevice_E));

					srd.ManufacturerID = MANUFACTURER_ID;
					srd.ProductID = PRODUCT_ID;
					srd.Flags = DEVICE_FLAGS;
					srd.channel_count = 0;
					ets_snprintf(srd.Email, SUPLA_EMAIL_MAXSIZE, "%s", supla_esp_cfg.Email);
					ets_snprintf(srd.ServerName, SUPLA_SERVER_NAME_MAXSIZE, "%s", supla_esp_cfg.Server);

					supla_esp_board_set_device_name(srd.Name, SUPLA_DEVICE_NAME_MAXSIZE);

					strcpy(srd.SoftVer, SUPLA_ESP_SOFTVER);
					os_memcpy(srd.GUID, supla_esp_cfg.GUID, SUPLA_GUID_SIZE);
					os_memcpy(srd.AuthKey, supla_esp_cfg.AuthKey, SUPLA_AUTHKEY_SIZE);

					supla_esp_board_set_channels(srd.channels, &srd.channel_count);

					for(uint8 a=0;a<RELAY_MAX_COUNT;a++) {
						if ( supla_relay_cfg[a].gpio_id != 255 ) {
							for(uint8 b=0;b<srd.channel_count;b++) {
								if (srd.channels[b].Number == supla_relay_cfg[a].channel) {
									supla_relay_cfg[a].channel_flags = srd.channels[b].Flags;
									break;
								}
							}
						}
					}


					srpc_ds_async_registerdevice_e(devconn->srpc, &srd);
				#else
					TDS_SuplaRegisterDevice_D srd;
					memset(&srd, 0, sizeof(TDS_SuplaRegisterDevice_C));

					srd.channel_count = 0;
					ets_snprintf(srd.Email, SUPLA_EMAIL_MAXSIZE, "%s", supla_esp_cfg.Email);
					ets_snprintf(srd.ServerName, SUPLA_SERVER_NAME_MAXSIZE, "%s", supla_esp_cfg.Server);

					supla_esp_board_set_device_name(srd.Name, SUPLA_DEVICE_NAME_MAXSIZE);

					strcpy(srd.SoftVer, SUPLA_ESP_SOFTVER);
					os_memcpy(srd.GUID, supla_esp_cfg.GUID, SUPLA_GUID_SIZE);
					os_memcpy(srd.AuthKey, supla_esp_cfg.AuthKey, SUPLA_AUTHKEY_SIZE);

					supla_esp_board_set_channels(srd.channels, &srd.channel_count);

					srpc_ds_async_registerdevice_d(devconn->srpc, &srd);
				#endif
			} else {
				#if ESP8266_SUPLA_PROTO_VERSION < 10
					TDS_SuplaRegisterDevice_C srd;
					memset(&srd, 0, sizeof(TDS_SuplaRegisterDevice_B));

					srd.channel_count = 0;
					srd.LocationID = supla_esp_cfg.LocationID;
					ets_snprintf(srd.LocationPWD, SUPLA_LOCATION_PWD_MAXSIZE, "%s", supla_esp_cfg.LocationPwd);
					ets_snprintf(srd.ServerName, SUPLA_SERVER_NAME_MAXSIZE, "%s", supla_esp_cfg.Server);

					supla_esp_board_set_device_name(srd.Name, SUPLA_DEVICE_NAME_MAXSIZE);

					strcpy(srd.SoftVer, SUPLA_ESP_SOFTVER);
					os_memcpy(srd.GUID, supla_esp_cfg.GUID, SUPLA_GUID_SIZE);

					supla_esp_board_set_channels(srd.channels, &srd.channel_count);

					srpc_ds_async_registerdevice_c(devconn->srpc, &srd);
                #else
					supla_log(LOG_DEBUG, "iterate fail");
				#endif
			}


		};

		supla_esp_data_write(NULL, 0, NULL);

		if( srpc_iterate(devconn->srpc) == SUPLA_RESULT_FALSE ) {
			supla_log(LOG_DEBUG, "iterate fail");
			supla_system_restart();
		}

	}

}

void DEVCONN_ICACHE_FLASH
supla_esp_srpc_free(void) {
	if ( devconn->srpc != NULL ) {
		srpc_free(devconn->srpc);
		devconn->srpc = NULL;
	}
}


void DEVCONN_ICACHE_FLASH
supla_esp_srpc_init(void) {

	supla_esp_srpc_free();

	TsrpcParams srpc_params;
	srpc_params_init(&srpc_params);
	srpc_params.data_read = &supla_esp_data_read;
	srpc_params.data_write = &supla_esp_data_write;
	srpc_params.on_remote_call_received = &supla_esp_on_remote_call_received;

	devconn->srpc = srpc_init(&srpc_params);

	srpc_set_proto_version(devconn->srpc, ESP8266_SUPLA_PROTO_VERSION);

	os_timer_setfn(&devconn->supla_iterate_timer, (os_timer_func_t *)supla_esp_devconn_iterate, NULL);
	os_timer_arm(&devconn->supla_iterate_timer, 100, 1);
}

void DEVCONN_ICACHE_FLASH supla_espconn_disconnect(struct espconn *espconn) {
  // supla_log(LOG_DEBUG, "Disconnect %i", espconn->state);

  // !!! Do not call this function in any espconn callback.
  _supla_espconn_disconnect(espconn);
}

void DEVCONN_ICACHE_FLASH
supla_esp_devconn_connect_cb(void *arg) {
	supla_esp_srpc_init();
}

void DEVCONN_ICACHE_FLASH supla_esp_devconn_disconnect_cb(void *arg) {
  supla_esp_gpio_state_ipreceived();  // We go back to the state after
                                      // connecting to wifi, and before
                                      // connecting to the server

  devconn->esp_send_buffer_len = 0;
  devconn->recvbuff_size = 0;

  if (devconn->started) {
    supla_esp_devconn_reconnect_with_delay(RECONNECT_DELAY_MSEC);
  }
}

void DEVCONN_ICACHE_FLASH supla_esp_devconn_dns__found(ip_addr_t *ip) {
  // supla_log(LOG_DEBUG, "supla_esp_devconn_dns_found_cb");

  if (devconn == NULL) {
    return;
  }

  devconn->resolving_started = false;

  if (ip == NULL) {
    supla_esp_set_state(LOG_NOTICE, "Domain not found.");
    return;
  }

  supla_espconn_disconnect(&devconn->ESPConn);

  devconn->ESPConn.proto.tcp = &devconn->ESPTCP;
  devconn->ESPConn.type = ESPCONN_TCP;
  devconn->ESPConn.state = ESPCONN_NONE;

  os_memcpy(devconn->ESPConn.proto.tcp->remote_ip, ip, 4);
  devconn->ESPConn.proto.tcp->local_port = espconn_port();

#if NOSSL == 1
  devconn->ESPConn.proto.tcp->remote_port = 2015;
#else
  devconn->ESPConn.proto.tcp->remote_port = 2016;
#endif

  espconn_regist_recvcb(&devconn->ESPConn, supla_esp_devconn_recv_cb);
  espconn_regist_connectcb(&devconn->ESPConn, supla_esp_devconn_connect_cb);
  espconn_regist_disconcb(&devconn->ESPConn, supla_esp_devconn_disconnect_cb);

  supla_espconn_connect(&devconn->ESPConn);
}

void DEVCONN_ICACHE_FLASH supla_esp_devconn_dns_found_cb(const char *name,
                                                         ip_addr_t *ip,
                                                         void *arg) {

#ifndef ADDITIONAL_DNS_CLIENT_DISABLED
  if (ip == NULL) {
	supla_esp_dns_resolve(supla_esp_cfg.Server, supla_esp_devconn_dns__found);
    return;
  }
#endif /*ADDITIONAL_DNS_CLIENT_DISABLED*/

  supla_esp_devconn_dns__found(ip);
}

void DEVCONN_ICACHE_FLASH supla_esp_devconn_resolvandconnect(void) {
  if (!devconn || devconn->resolving_started) {
    // Calling espconn_gethostbyname twice causes Fatal exception 29
    // (StoreProhibitedCause)
    supla_log(LOG_DEBUG, "Resolving already started!");
    return;
  }

  devconn->resolving_started = true;

  // supla_log(LOG_DEBUG, "supla_esp_devconn_resolvandconnect");
  supla_espconn_disconnect(&devconn->ESPConn);

  uint32_t _ip = ipaddr_addr(supla_esp_cfg.Server);

  if (_ip == -1) {
    supla_log(LOG_DEBUG, "Resolv %s", supla_esp_cfg.Server);

    espconn_gethostbyname(&devconn->ESPConn, supla_esp_cfg.Server,
                          &devconn->ipaddr, supla_esp_devconn_dns_found_cb);
  } else {
    supla_esp_devconn_dns_found_cb(supla_esp_cfg.Server, (ip_addr_t *)&_ip,
                                   NULL);
  }
}

void DEVCONN_ICACHE_FLASH supla_esp_devconn_watchdog_cb(void *timer_arg) {
  if (supla_esp_cfgmode_started() == 0 && supla_esp_update_started() == 0) {
    if (uptime_sec() > devconn->last_response) {
      if (uptime_sec() - devconn->last_response > WATCHDOG_TIMEOUT_SEC) {
        supla_log(LOG_DEBUG, "WATCHDOG TIMEOUT");
        supla_system_restart();
      } else {
        unsigned int t = uptime_sec() - devconn->last_response;
        if (t >= WATCHDOG_SOFT_TIMEOUT_SEC &&
            t > devconn->server_activity_timeout &&
            uptime_sec() > devconn->next_wd_soft_timeout_challenge) {
          supla_log(LOG_DEBUG, "WATCHDOG SOFT TIMEOUT");
          supla_esp_devconn_reconnect();
        }
      }
    }
  }
}

void DEVCONN_ICACHE_FLASH
supla_esp_devconn_before_cfgmode_start(void) {

	if (!devconn) {
		return;
	}

    #ifndef SUPLA_SMOOTH_DISABLED
	#if defined(RGB_CONTROLLER_CHANNEL) \
		|| defined(RGBW_CONTROLLER_CHANNEL) \
		|| defined(RGBWW_CONTROLLER_CHANNEL) \
		|| defined(DIMMER_CHANNEL)

		int a;

		supla_esp_hw_timer_disarm();

		for(a=0;a<SMOOTH_MAX_COUNT;a++) {
			smooth[a].active = 0;
		}

	#endif
    #endif /*SUPLA_SMOOTH_DISABLED*/

	os_timer_disarm(&devconn->supla_watchdog_timer);
}

void DEVCONN_ICACHE_FLASH
supla_esp_devconn_before_update_start(void) {

	os_timer_disarm(&devconn->supla_watchdog_timer);

}

void DEVCONN_ICACHE_FLASH
supla_esp_devconn_init(void) {

	devconn = (devconn_params*)malloc(sizeof(devconn_params));
	memset(devconn, 0, sizeof(devconn_params));
	memset(&devconn->ESPConn, 0, sizeof(struct espconn));
	memset(&devconn->ESPTCP, 0, sizeof(esp_tcp));

	os_timer_disarm(&devconn->supla_watchdog_timer);
	os_timer_setfn(&devconn->supla_watchdog_timer, (os_timer_func_t *)supla_esp_devconn_watchdog_cb, NULL);
	os_timer_arm(&devconn->supla_watchdog_timer, 1000, 1);

#ifndef COUNTDOWN_TIMER_DISABLED
	supla_esp_countdown_set_on_disarm_cb(supla_esp_devconn_on_countdown_on_disarm);
	supla_esp_countdown_set_finish_cb(supla_esp_devconn_on_countdown_timer_finish);
#endif /*COUNTDOWN_TIMER_DISABLED*/
}

void DEVCONN_ICACHE_FLASH
supla_esp_devconn_on_wifi_status_changed(uint8 status) {
  if (devconn->started && devconn->srpc == NULL && status == STATION_GOT_IP) {
	  supla_esp_devconn_resolvandconnect();
  }
}

void DEVCONN_ICACHE_FLASH supla_esp_devconn_start(void) {
  if (!devconn) {
    return;
  }

  supla_esp_gpio_state_ipreceived();  // We go back to the state after
                                      // connecting to wifi, and before
                                      // connecting to the server

  devconn->started = 1;
  supla_esp_wifi_station_connect(supla_esp_devconn_on_wifi_status_changed);

  os_timer_disarm(&devconn->reconnect_delay_timer);
  os_timer_disarm(&devconn->supla_devconn_timer1);
  os_timer_setfn(&devconn->supla_devconn_timer1,
                 (os_timer_func_t *)supla_esp_devconn_timer1_cb, NULL);
  os_timer_arm(&devconn->supla_devconn_timer1, 1000, 1);
}

void DEVCONN_ICACHE_FLASH supla_esp_devconn__stop(void *ptr) {
  if (!devconn) {
	  return;
  }
  devconn->registered = 0;
  devconn->started = 0;

  os_timer_disarm(&devconn->supla_devconn_timer1);
  os_timer_disarm(&devconn->supla_iterate_timer);

  supla_espconn_disconnect(&devconn->ESPConn);

  supla_esp_srpc_free();
}

void DEVCONN_ICACHE_FLASH supla_esp_devconn_stop(void) {
	supla_esp_devconn__stop(NULL);
}

void DEVCONN_ICACHE_FLASH supla_esp_devconn_stop_with_delay(void) {
  // Use this function if you want to stop the connection from the espconn
  // callback.

  os_timer_disarm(&devconn->stop_delay_timer);
  os_timer_setfn(&devconn->stop_delay_timer,
                 (os_timer_func_t *)supla_esp_devconn__stop, NULL);
  os_timer_arm(&devconn->stop_delay_timer, 5, 0);
}

void DEVCONN_ICACHE_FLASH supla_esp_devconn__reconnect(void *ptr) {
  devconn->next_wd_soft_timeout_challenge =
      uptime_sec() + WATCHDOG_SOFT_TIMEOUT_SEC;

  if (supla_esp_cfgmode_started() == 0 && supla_esp_update_started() == 0) {
    supla_esp_devconn_stop();
    supla_esp_devconn_start();
  }
}

void DEVCONN_ICACHE_FLASH supla_esp_devconn_reconnect(void) {
  supla_esp_devconn__reconnect(NULL);
}

void DEVCONN_ICACHE_FLASH
supla_esp_devconn_reconnect_with_delay(uint32 time_ms) {
  os_timer_disarm(&devconn->reconnect_delay_timer);
  os_timer_setfn(&devconn->reconnect_delay_timer,
                 (os_timer_func_t *)supla_esp_devconn__reconnect, NULL);
  os_timer_arm(&devconn->reconnect_delay_timer, time_ms, 0);
}

void DEVCONN_ICACHE_FLASH
supla_esp_devconn_timer1_cb(void *timer_arg) {
	unsigned int t1;
	unsigned int t2;

	if ( supla_esp_devconn_is_registered() == 1
		 && devconn->server_activity_timeout > 0
		 && devconn->srpc != NULL ) {

		    t1 = uptime_sec()-devconn->last_sent;
		    t2 = uptime_sec()-devconn->last_response;

		    if ( t2 >= (devconn->server_activity_timeout+10) ) {
		    	supla_log(LOG_DEBUG, "ACTIVITY TIMEOUT");
		    	supla_esp_devconn_reconnect();
		    } else if ( ( t1 >= (devconn->server_activity_timeout-5)
		    		      && t1 <= devconn->server_activity_timeout )
		    		    || ( t2 >= (devconn->server_activity_timeout-5)
		    		         && t2 <= devconn->server_activity_timeout ) ) {

				srpc_dcs_async_ping_server(devconn->srpc);

			}

	}
}

#ifdef ELECTRICITY_METER_COUNT
void DEVCONN_ICACHE_FLASH supla_esp_channel_em_value_changed(unsigned char channel_number, TElectricityMeter_ExtendedValue_V2 *em_ev) {
	TSuplaChannelExtendedValue *ev =
	     (TSuplaChannelExtendedValue *)malloc(sizeof(TSuplaChannelExtendedValue));
	if (ev != NULL) {
		srpc_evtool_v2_emextended2extended(em_ev, ev);
		supla_esp_channel_extendedvalue_changed(channel_number, ev);
		free(ev);
	}
}
#endif /*ELECTRICITY_METER_COUNT*/

#ifdef BOARD_CALCFG
void DEVCONN_ICACHE_FLASH supla_esp_calcfg_result(TDS_DeviceCalCfgResult *result) {
	if (supla_esp_devconn_is_registered() == 1) {
		srpc_ds_async_device_calcfg_result(devconn->srpc, result);
	}
}
#endif /*BOARD_CALCFG*/

#ifdef BOARD_ON_USER_LOCALTIME_RESULT
void DEVCONN_ICACHE_FLASH supla_esp_devconn_get_user_localtime(void) {
  if (supla_esp_devconn_is_registered() == 1) {
    srpc_dcs_async_get_user_localtime(devconn->srpc);
  }
}
#endif /*BOARD_ON_USER_LOCALTIME_RESULT*/

#ifdef BOARD_ON_CHANNEL_INT_PARAMS_RESULT
void DEVCONN_ICACHE_FLASH
supla_esp_devconn_get_channel_int_params(unsigned char channel_number) {
  if (supla_esp_devconn_is_registered() == 1) {
	  srpc_ds_async_get_channel_int_params(devconn->srpc, channel_number);
  }
}
#endif /*BOARD_ON_CHANNEL_INT_PARAMS_RESULT*/
