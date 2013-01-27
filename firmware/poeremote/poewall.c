/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/*---------------------------------------------------------------------------*/
#include <stdlib.h>

//#include "mac_event.h"
#include "uip.h"

#include "contiki.h"
#include "contiki-net.h"
#include "contiki-lib.h"

#include "dev/rs232.h"

#include "./xPLimpl.h"
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include "dev/lcd/lcd.h"
#include "dev/xmega-adc/xmega-adc.h"
#include <xmega-signatures.h>
#include "dev/avr-poe-buttons/avr-poe-buttons.h"

#include "net/eth-enc28j60.h"


#include "net/dhcpc.h"


//#define PRINTF(format, args...) ((void)0)
//#define PRINTF(...) { printf(__VA_ARGS__); }

#define PRINTF(format, ...) { printf_P( PSTR(format), ##__VA_ARGS__);}


// for later
PROCESS(my_dhcp_process, "DHCP");
uint8_t dhcpStarted = 0;














































//AUTOSTART_PROCESSES(&dhcp_process);


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(my_dhcp_process, ev, data)
{

    //turn lights off first!

    
//  static struct etimer timer;
//  static struct uip_udp_conn *udpconn;
  char* hostname = HOSTNAME;
  uint8_t i = 0;
  
  PROCESS_BEGIN();
  //PORTC.DIRSET = 0x03;
  //PORTC.OUTSET = 0x01;


//   udpconn = udp_broadcast_new(UIP_HTONS(67), NULL);
//   udp_bind(udpconn, UIP_HTONS(68));
  
  dhcpc_init(uip_ethaddr.addr, sizeof(uip_ethaddr.addr));
  dhcpc_set_hostname_p(hostname);
  PRINTF("dhcp started with MAC: "); 
  
  for (i=0; i<sizeof(uip_ethaddr.addr); i++){
    PRINTF("%02X ", uip_ethaddr.addr[i] );
  }
  PRINTF("\n");

  //etimer_set(&timer, CLOCK_CONF_SECOND*10);

//    dhcpc_request();
//    PRINTF("dhcp Requesting...\n");


  while(1) {
    PROCESS_WAIT_EVENT();

    if(ev == tcpip_event) {
        if (uip_newdata())
      PRINTF("calling with incomming packet\n");
        dhcpc_appcall(ev, data);
    } else if(ev == PROCESS_EVENT_EXIT) {
        process_exit(&my_dhcp_process);
    } 
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
dhcpc_configured(const struct dhcpc_state *s)
{
  uip_sethostaddr(&s->ipaddr);
  uip_setnetmask(&s->netmask);
  uip_setdraddr(&s->default_router);
  resolv_conf(&s->dnsaddr);
  PRINTF("dhcp Configured.\n");
  
  PRINTF("Got IP address %d.%d.%d.%d\n", uip_ipaddr_to_quad(&s->ipaddr));
  PRINTF("Got netmask %d.%d.%d.%d\n",  uip_ipaddr_to_quad(&s->netmask));
  

  if (!dhcpStarted) {
    dhcpStarted = 1;
    process_start(&xPL_process, "xpl process");
    
  }
  

  
}
/*---------------------------------------------------------------------------*/
void
dhcpc_unconfigured(const struct dhcpc_state *s)
{
  PRINTF("dhcp Unconfigured.\n");
}
/*---------------------------------------------------------------------------*/












//AUTOSTART_PROCESSES(&clock_tick_process,  &enc28j60_process, &tcpip_process, &pixel_client, &xPL_process, &lcd_test_process);
AUTOSTART_PROCESSES(&clock_tick_process,  &enc28j60_process, &tcpip_process, &my_dhcp_process);//&my_dhcp_process, &xPL_process );
//AUTOSTART_PROCESSES(&clock_tick_process,  &enc28j60_process, &tcpip_process, &xPL_process );
//, &pixel_client,

