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

// Code based on http://tech.scargill.net/esp8266-and-the-dallas-ds18b20-and-ds18b20p/

#include <os_type.h>
#include <osapi.h>
#include <eagle_soc.h>
#include <ets_sys.h>
#include <gpio.h>

#include "supla_ds18b20.h"
#include "supla_w1.h"
#include "supla-dev/log.h"
#include "supla_dht.h"

#include "supla_esp_devconn.h"

#if defined DS18B20 || defined TEMPERATURE_PORT_CHANNEL

int supla_ds18b20_pin;

static double supla_ds18b20_last_temp = -275;

ETSTimer supla_ds18b20_timer1;
ETSTimer supla_ds18b20_timer2;

#define SUPLA_DS_MODEL_UNKNOWN 0
#define SUPLA_DS_MODEL_18B20 1
#define SUPLA_DS_MODEL_1820  2

static char SUPLA_DS_MODEL = SUPLA_DS_MODEL_UNKNOWN;
static float supla_ds18b20_divider;

// TODO: Add support for multiple sensors
// TODO: Add resolution setup

void supla_ds18b20_init(void) {

	supla_w1_init();
}

#ifdef TEMPERATURE_PORT_CHANNEL

void supla_ds18b20_reset(void)
{
    uint8_t retries = 125;
    GPIO_DIS_OUTPUT( supla_ds18b20_pin );
    do {
        if (--retries == 0) return;
        os_delay_us(2);
    } while ( !GPIO_INPUT_GET( supla_ds18b20_pin ));

    GPIO_OUTPUT_SET( supla_ds18b20_pin, 0 );
    os_delay_us(480);
    GPIO_DIS_OUTPUT( supla_ds18b20_pin );
    os_delay_us(480);
}

void supla_ds18b20_write_bit( int v )
{
    GPIO_OUTPUT_SET( supla_ds18b20_pin, 0 );
    if( v ) {
        os_delay_us(10);
        GPIO_OUTPUT_SET( supla_ds18b20_pin, 1 );
        os_delay_us(55);
    } else {
        os_delay_us(65);
        GPIO_OUTPUT_SET( supla_ds18b20_pin, 1 );
        os_delay_us(5);
    }
}


int supla_ds18b20_read_bit(void)
{
    int r;
    GPIO_OUTPUT_SET( supla_ds18b20_pin, 0 );
    os_delay_us(3);
    GPIO_DIS_OUTPUT( supla_ds18b20_pin );
    os_delay_us(10);
    r = GPIO_INPUT_GET( supla_ds18b20_pin );
    os_delay_us(53);
    return r;
}

void supla_ds18b20_write( uint8_t v, int power ) {
    uint8_t bitMask;
    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
    	supla_ds18b20_write_bit( (bitMask & v)?1:0);
    }
    if ( !power) {
        GPIO_DIS_OUTPUT( supla_ds18b20_pin );
        GPIO_OUTPUT_SET( supla_ds18b20_pin, 0 );
    }
}

#else

void supla_ds18b20_reset(void)
{
    uint8_t retries = 125;
    GPIO_DIS_OUTPUT( supla_w1_pin );
    do {
        if (--retries == 0) return;
        os_delay_us(2);
    } while ( !GPIO_INPUT_GET( supla_w1_pin ));

    GPIO_OUTPUT_SET( supla_w1_pin, 0 );
    os_delay_us(480);
    GPIO_DIS_OUTPUT( supla_w1_pin );
    os_delay_us(480);
}

void supla_ds18b20_write_bit( int v )
{
    GPIO_OUTPUT_SET( supla_w1_pin, 0 );
    if( v ) {
        os_delay_us(10);
        GPIO_OUTPUT_SET( supla_w1_pin, 1 );
        os_delay_us(55);
    } else {
        os_delay_us(65);
        GPIO_OUTPUT_SET( supla_w1_pin, 1 );
        os_delay_us(5);
    }
}


int supla_ds18b20_read_bit(void)
{
    int r;
    GPIO_OUTPUT_SET( supla_w1_pin, 0 );
    os_delay_us(3);
    GPIO_DIS_OUTPUT( supla_w1_pin );
    os_delay_us(10);
    r = GPIO_INPUT_GET( supla_w1_pin );
    os_delay_us(53);
    return r;
}

void supla_ds18b20_write( uint8_t v, int power ) {
    uint8_t bitMask;
    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
    	supla_ds18b20_write_bit( (bitMask & v)?1:0);
    }
    if ( !power) {
        GPIO_DIS_OUTPUT( supla_w1_pin );
        GPIO_OUTPUT_SET( supla_w1_pin, 0 );
    }
}
#endif

char* float2String(char* buffer, float value)
{
  os_sprintf(buffer, "%d.%d", (int)(value),(int)((value - (int)value)*100));
  return buffer;
}


uint8_t  supla_ds18b20_read() {
    uint8_t bitMask;
    uint8_t r = 0;
    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
        if ( supla_ds18b20_read_bit()) r |= bitMask;
    }
    return r;
}


void
supla_ds18b20_read_temperatureB(void *timer_arg) {

	os_timer_disarm(&supla_ds18b20_timer2);

	supla_ds18b20_reset();
	supla_ds18b20_write(0xcc,1);
	supla_ds18b20_write(0xbe,1);


    uint8_t data[8];
    int i;
    char d = 0;

    for(i=0;i<8;i++) {
    	data[i] = supla_ds18b20_read();
    	if ( data[i] != 255 ) d = 1;
    }

    double t = -275;

    if ( d == 1 ) {
    	t = ((((int8_t)data[1])<<8) | data[0]) / supla_ds18b20_divider;

		#ifdef DHTSENSOR
		os_timer_disarm(&supla_dht_timer1);
		#endif

    } else {
    	t = -275;
    }

    char buff[20];
    supla_log(LOG_DEBUG, "t = %s",  float2String(buff, t));

    static uint8_t error;
    if (error >= NUMBER_OF_ERRORS) t = -275;
    //if (error != 0) supla_log(LOG_DEBUG, "supla_ds18b20_ERROR = %s",  error);

    if ( supla_ds18b20_last_temp != t ) {
    	if((t >= -55 && t <= 125) || t == -275){
            int difference_temp = supla_ds18b20_last_temp - t;
            
            
            if (( difference_temp <-10 || difference_temp > 10) && t != -275 && supla_ds18b20_last_temp != -275 ) {
                error++;
                
                supla_ds18b20_last_temp = t;
                supla_log(LOG_DEBUG, "ds18b20 error difference!  = %i error = %i",  difference_temp, error);
            }
            else {
                error = 0;
                supla_ds18b20_last_temp = t;
            
            	char value[SUPLA_CHANNELVALUE_SIZE];
            	memset(value, 0, sizeof(SUPLA_CHANNELVALUE_SIZE));
            	supla_get_temperature(value);
                supla_log(LOG_DEBUG, "supla_ds18b20_last_temp = %s", float2String(buff, supla_ds18b20_last_temp));
            #ifdef TEMPERATURE_PORT_CHANNEL
            	supla_esp_channel_value__changed(temperature_channel, value);
            #else
            	supla_esp_channel_value__changed(TEMPERATURE_CHANNEL, value);
            #endif
            }   
        }
    };

}

void
supla_ds18b20_read_temperatureA(void *timer_arg) {

	supla_ds18b20_reset();

	if ( SUPLA_DS_MODEL == SUPLA_DS_MODEL_UNKNOWN ) {

		uint8_t data = 0;

		supla_ds18b20_write(0x33, 1);
	    data = supla_ds18b20_read();

	    if ( data == 0x28 ) {
	    	supla_ds18b20_divider = 16.0;
	    	SUPLA_DS_MODEL = SUPLA_DS_MODEL_18B20;

	    } else if ( data == 0x10 ) {
	    	supla_ds18b20_divider = 2.0;
	    	SUPLA_DS_MODEL = SUPLA_DS_MODEL_1820;
	    }


	    if ( SUPLA_DS_MODEL != SUPLA_DS_MODEL_UNKNOWN )
	    	supla_ds18b20_read_temperatureA(NULL);

	} else {

		supla_ds18b20_write(0xcc,1);
	    supla_ds18b20_write(0x44,1);

	    os_timer_disarm(&supla_ds18b20_timer2);
		os_timer_setfn(&supla_ds18b20_timer2, supla_ds18b20_read_temperatureB, NULL);
	    os_timer_arm (&supla_ds18b20_timer2, 1000, 0);

	}

}

void supla_ds18b20_start(void)
{
	supla_ds18b20_last_temp = -275;

	os_timer_disarm(&supla_ds18b20_timer1);
	os_timer_setfn(&supla_ds18b20_timer1, supla_ds18b20_read_temperatureA, NULL);
	os_timer_arm (&supla_ds18b20_timer1, 5000, 1);
}

void supla_get_temperature(char value[SUPLA_CHANNELVALUE_SIZE]) {
	// Only temperature
	memcpy(value, &supla_ds18b20_last_temp, sizeof(double));
}

#endif

 
