#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "espconn.h"
#include "at_custom.h"
#include "mem.h"
#include "c_math.h"

int UDP_Transmit_flag=0;
struct espconn * connection;


void UDP_sent_callback (void *arg){

}



void   UDP_recv_callback (void *arg,char *pdata,unsigned short len){

	struct espconn * u= (struct espconn *)arg;
    if(UDP_Transmit_flag==0)
	  connection=u;

	uart0_tx_buffer((uint8 *)pdata,len);
    //espconn_sent(connection,pdata,len);

	UDP_Transmit_flag=1;

}


