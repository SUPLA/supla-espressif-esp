/*
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 */

#include <os_type.h>
#include <osapi.h>
#include <eagle_soc.h>
#include <ets_sys.h>

#include "supla_pow_R2.h"
#include "supla_esp_gpio.h"
#include "supla_w1.h"
#include "supla-dev/log.h"
#include "gpio.h"
#include "user_config.h"
#include "supla_esp_devconn.h"
#include "driver/uart.h"

#ifdef POWSENSOR2

int counter;
int test_sel = 0;
uint32_t voltage_time;
int supla_micros_gpio_state = 0;
uint8_t buffer[128];

uint32_t voltage = 0;
uint32_t current = 0;
uint32_t power   = 0;

uint8_t cse_receive_flag = 0;

long voltage_cycle = 0;
long current_cycle = 0;
long power_cycle = 0;
unsigned long power_cycle_first = 0;
long cf_pulses = 0;

void CseReceived(int start, unsigned int relay_laststate)
{
  uint8_t header = buffer[0+start];
  if ((header & 0xFC) == 0xFC) {
    supla_log(LOG_DEBUG, "CSE: Abnormal hardware");
    return;
  }

	if ( relay_laststate == 1) {
		long voltage_coefficient;
		voltage_coefficient = buffer[2+start]*65536 + buffer[3+start]*256 + buffer[4+start];
		voltage_cycle = buffer[5+start]*65536 + buffer[6+start]*256 + buffer[7+start];
		if ((buffer[start] & 0xF8) == 0xF8) {
			voltage = 0;
		} else {
			voltage = (voltage_coefficient * 10) / voltage_cycle;
		}

		long current_coefficient;
		current_coefficient = buffer[8+start]*65536 + buffer[9+start]*256 + buffer[10+start];
		current_cycle = buffer[11+start]*65536 + buffer[12+start]*256 + buffer[13+start];
		if ((buffer[start] & 0xF4) == 0xF4) {
		current = 0;
		} else {
		current = (current_coefficient * 1000) / current_cycle;
		}
		if (current < 100) current = 0;

		long power_coefficient;
		power_coefficient = buffer[14+start]*65536 + buffer[15+start]*256 + buffer[16+start];
		power_cycle = buffer[17+start]*65536 + buffer[18+start]*256 + buffer[19+start];
		uint8_t adjustment = buffer[20+start];
		cf_pulses = buffer[21+start]*256 + buffer[22+start];
		if (adjustment & 0x10) {
		if ((buffer[start] & 0xF2) == 0xF2) {
			power = 0;
		} else {
			power = power_coefficient / power_cycle;
		}
		} else {
			power = 0;  
		}
	} else {
		voltage = 0;
		current = 0;
		power = 0;  
	}
}

//-------------------------------------------------------
void ICACHE_FLASH_ATTR
supla_getVoltage(char value[SUPLA_CHANNELVALUE_SIZE]) {
	memcpy(value, &voltage, sizeof(uint32_t));
}

//-------------------------------------------------------
void ICACHE_FLASH_ATTR
supla_getCurrent(char value[SUPLA_CHANNELVALUE_SIZE]) {
	memcpy(value, &current, sizeof(uint32_t));
}

//-------------------------------------------------------
void ICACHE_FLASH_ATTR
supla_getPower(char value[SUPLA_CHANNELVALUE_SIZE]) {
	memcpy(value, &power, sizeof(uint32_t));
}

void ICACHE_FLASH_ATTR
supla_pow_R2_init(void) {
	uart_div_modify(0, UART_CLK_FREQ / 4800);
	os_printf("UART Init\n");
}

int ICACHE_FLASH_ATTR
UART_Recv(uint8 uart_no, uint8_t *buffer, int max_buf_len)
{
    uint8 max_unload;
	int index = -1;

    uint8 fifo_len = (READ_PERI_REG(UART_STATUS(uart_no))>>UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;

    if (fifo_len)
    {
        max_unload = (fifo_len<max_buf_len ? fifo_len : max_buf_len);
        os_printf("Rx Fifo contains %d characters have to unload %d\r\n", fifo_len , max_unload);

        for (index=0;index<max_unload; index++)
        {
            *(buffer+index) = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
        }
        WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_RXFIFO_FULL_INT_CLR);
    }
	else
        os_printf("Rx Fifo is empty\r\n");

    return index;
}

void ICACHE_FLASH_ATTR
uart_status(unsigned int relay_laststate) {
	int recv_len = -1;
	int index;

    recv_len = UART_Recv(0, buffer, sizeof(buffer)-2);

    if (recv_len>0)
    {
        buffer[recv_len] = 0;
        os_printf("Received: [%d]\r\n", recv_len);
		
        for (index = 0; index < recv_len-1; index ++) {
			if ((buffer[index] == 0xF2) && (buffer[index+1] == 0x5A)) break;
			if ((buffer[index] == 0x55) && (buffer[index+1] == 0x5A)) break;
		}
		if (index < recv_len - 24){
			os_printf("Index: %d\r\n", index);
			if (index>0) CseReceived(index, relay_laststate);
		}
    }
}

#endif
