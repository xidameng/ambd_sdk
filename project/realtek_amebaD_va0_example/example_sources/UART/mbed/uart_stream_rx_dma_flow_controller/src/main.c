/*
 *  Routines to access hardware
 *
 *  Copyright (c) 2013 Realtek Semiconductor Corp.
 *
 *  This module is a confidential and proprietary property of RealTek and
 *  possession or use of this module requires written permission of RealTek.
 */

#include "device.h"
#include "serial_api.h"
#include "serial_ex_api.h"

//UART pin location:     
//KM4 UART0: 
//PA_18  (TX)
//PA_19  (RX)

#define UART_TX    PA_18	//UART0  TX
#define UART_RX    PA_19   //UART0  RX

#define SRX_BUF_SZ 32*5      /*note that dma mode buffer length should be integral multiple of 32 bytes*/
char rx_buf[SRX_BUF_SZ]__attribute__((aligned(32)))={0};  /*note that dma mode buffer address should be 32 bytes aligned*/

volatile uint32_t tx_busy=0;
volatile uint32_t rx_done=0;

void uart_send_string_done(uint32_t id)
{
	serial_t    *sobj = (void*)id;
	tx_busy = 0;
}

void uart_recv_string_done(uint32_t id)
{
	serial_t    *sobj = (void*)id;
	rx_done = 1;
	DiagPrintf("recv timeout\n");
}

void uart_send_string(serial_t *sobj, char *pstr)
{
	int32_t ret=0;

	if (tx_busy) {
		return;
	}
    
	tx_busy = 1;
	ret = serial_send_stream_dma(sobj, pstr, _strlen(pstr));
	if (ret != 0) {
		DiagPrintf("%s Error(%d)\n", __FUNCTION__, ret);        
		tx_busy = 0;
	}
}

void maintask(void)
{
	serial_t    sobj;
	int ret;
	int i=0;
	int len;
	
	serial_init(&sobj,UART_TX,UART_RX);
	serial_baud(&sobj,38400);
	serial_format(&sobj, 8, ParityNone, 1);

	serial_send_comp_handler(&sobj, (void*)uart_send_string_done, (uint32_t) &sobj);
	serial_recv_comp_handler(&sobj, (void*)uart_recv_string_done, (uint32_t) &sobj);

	for(i=0;i<SRX_BUF_SZ;i++){
		rx_buf[i]=0;
    	}
	while (1) {
		if (rx_done) {
			uart_send_string(&sobj, rx_buf);            
			rx_done = 0;
  
		}
		if(!tx_busy){
			/*uart is flow controller and you can set rx timeout (unit is ms)*/
			ret = serial_recv_stream_dma_timeout(&sobj, rx_buf, 0,500,NULL);
		}
	}
}

void main(void)
{
	if (pdTRUE != xTaskCreate( (TaskFunction_t)maintask, "RAW_GTIMER_DEMO_TASK", (2048 /4), (void *)NULL, (tskIDLE_PRIORITY + 1), NULL))
	{
		DiagPrintf("Create RAW_GTIMER_DEMO_TASK Err!!\n");
	}
	
	vTaskStartScheduler();
}

