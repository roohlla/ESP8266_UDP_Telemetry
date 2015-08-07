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

#define Station_Mode 49
#define AP_Mode 49
#define Terminator_none 49
#define Terminator_SR   48

#define MODE_SEL GPIO_INPUT_GET(4)

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);
//TCP & UDP Functions:

//TCP:
extern void connect_callback (void *arg);
extern void disconnect_callback (void *arg);
extern void TCP_sent_callback (void *arg);
extern void TCP_recv_callback (void *arg,char *pdata,unsigned short len);
//UDP:
extern void UDP_sent_callback (void *arg);
extern void UDP_recv_callback (void *arg,char *pdata,unsigned short len);

extern UartDevice UartDev;

void TCP_Init(void);
void UDP_Init();
void wifi_Reset_Mode(void);
void Wifi_Init_Station_Mode();

struct UartBuffer * uart;

//Parameter Structure
struct DStrct1{


	int32  Mode;

	    char Soft_AP_ssid[32] ;
	    char Soft_AP_password[64] ;

	    char Router_ssid[32] ;
	    char Router_password[64] ;

	    int32 UDP_Client_Port;
	    int32 UDP_Server_Port;

	    int32 Sending_Packet_Size;
	    int32 Terminator;
}info1;
//Parameter Pay load
union {
	uint32 data[53];
	struct DStrct1 data_struct;
}conv1;

void Wifi_Init_AP_Mode();

//Main code function
static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{


	//os_printf("\n\rData Recieved: %d",UartDev.rcv_buff.pReadPos-UartDev.rcv_buff.pWritePos );
    system_os_post(user_procTaskPrio, 0, 0 );
}

//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
	os_delay_us(10000);
    uart_init(BIT_RATE_230400,BIT_RATE_230400);
    uart_rx_intr_enable(1);
    spi_flash_read(0x3D000,conv1.data, sizeof(info1) );

   if(MODE_SEL==1 )
   {
	 wifi_Reset_Mode();

     os_printf("\n\rRID = ");
     uart0_sendStr(conv1.data_struct.Router_ssid);

     os_printf("\n\rRpwd = ");
     uart0_sendStr(conv1.data_struct.Router_password);

     os_printf("\n\rRID = ");
     uart0_sendStr(conv1.data_struct.Soft_AP_ssid);

      os_printf("\n\rRID = ");
      uart0_sendStr(conv1.data_struct.Soft_AP_password);

      os_printf("\n\r Local port: %d ",conv1.data_struct.UDP_Client_Port);
      os_printf("\n\r Remot port: %d ",conv1.data_struct.UDP_Server_Port);

      os_printf("\n\r heater: %d \n\r",conv1.data_struct.Terminator);


     os_printf("\n\rData is : %d ",conv1.data_struct.Mode);
	 TCP_Init();
   }else{
	   system_set_os_print(0);




     	 if(conv1.data_struct.Mode==Station_Mode)
     		 Wifi_Init_Station_Mode();
     	 else
     		 Wifi_Init_AP_Mode();
		 //wifi_Reset_Mode();

     	 UDP_Init();


     }

    //Start os task
	  system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);

	  system_os_post(user_procTaskPrio, 0, 0 );

}


void UDP_Init(){

	//system_set_os_print(0);
	struct espconn *UDP_P =(struct espconn *) os_zalloc(sizeof(struct espconn));
		UDP_P->proto.udp =(esp_udp *) os_zalloc(sizeof (esp_udp));
		UDP_P->state=ESPCONN_NONE;
		UDP_P->type=ESPCONN_UDP;

		UDP_P->proto.udp->local_port=(int)conv1.data_struct.UDP_Server_Port ;   //The port on which we want the esp to serve
		UDP_P->proto.udp->remote_port=conv1.data_struct.UDP_Client_Port ;   //The port on which we want the esp to serve

		//Set The call back functions
		espconn_regist_recvcb(UDP_P,UDP_recv_callback);
		espconn_regist_sentcb(UDP_P,UDP_sent_callback);
		espconn_create(UDP_P);



}
//This Function Creates A TCP Server On Port 80 (WebServer)
void TCP_Init(){

	struct espconn *TCP_P =(struct espconn *) os_zalloc(sizeof(struct espconn));
	TCP_P->proto.tcp =(esp_tcp *) os_zalloc(sizeof (esp_tcp));
	TCP_P->state=ESPCONN_NONE;
	TCP_P->type=ESPCONN_TCP;

	TCP_P->proto.tcp->local_port=80 ;   //The port on which we want the esp to serve
	//Set The call back functions
	espconn_regist_connectcb(TCP_P,connect_callback);
	espconn_regist_disconcb(TCP_P,disconnect_callback);
	espconn_regist_recvcb(TCP_P,TCP_recv_callback);
	espconn_regist_sentcb(TCP_P,TCP_sent_callback);

	espconn_accept(TCP_P);

}

void wifi_Reset_Mode(){

	    char ssid[32] = "NanoStation";
	    char password[64] = "0123456789";
	    struct softap_config softAp;

		wifi_softap_get_config(&softAp);

	    softAp.authmode=AUTH_WPA2_PSK;
	    softAp.ssid_len=strlen(ssid);
	    softAp.channel=0;
	    softAp.ssid_hidden=0;
	    softAp.beacon_interval=200;

	    os_memcpy(&softAp.ssid, ssid, 32);
	    os_memcpy(&softAp.password, password, 64);

	    //Set SoftAP mode
	    wifi_set_opmode(SOFTAP_MODE);

	    ETS_UART_INTR_DISABLE();
	    wifi_softap_set_config(&softAp);
	    ETS_UART_INTR_ENABLE();

	    wifi_softap_dhcps_start();

}


void Wifi_Init_AP_Mode(){


		    struct softap_config softAp;

			wifi_softap_get_config(&softAp);

		    softAp.authmode=AUTH_WPA2_PSK;
		    softAp.ssid_len=strlen(conv1.data_struct.Soft_AP_ssid);
		    softAp.channel=0;
		    softAp.ssid_hidden=0;
		    softAp.beacon_interval=150;

		    os_memcpy(&softAp.ssid, conv1.data_struct.Soft_AP_ssid, 32);
		    os_memcpy(&softAp.password, conv1.data_struct.Soft_AP_password, 64);

		    //Set SoftAP mode
		    wifi_set_opmode(2);

		    ETS_UART_INTR_DISABLE();
		    wifi_softap_set_config(&softAp);
		    ETS_UART_INTR_ENABLE();

		    wifi_softap_dhcps_start();


}

void Wifi_Init_Station_Mode(){
	            struct station_config station;
				wifi_station_get_config(&station);

			    os_memcpy(&station.ssid, conv1.data_struct.Router_ssid, 32);
			    os_memcpy(&station.password, conv1.data_struct.Router_password, 64);

			    //Set station mode
			    wifi_set_opmode(1);

			    ETS_UART_INTR_DISABLE();
			    wifi_station_set_config(&station);
			    ETS_UART_INTR_ENABLE();

			    wifi_station_dhcpc_start();
}
