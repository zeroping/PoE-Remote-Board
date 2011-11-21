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
#include "dev/avr-poe-buttons/avr-poe-buttons.h"

#include "net/eth-enc28j60.h"


#include "net/dhcpc.h"


//#define PRINTF(format, args...) ((void)0)
//#define PRINTF(...) { printf(__VA_ARGS__); }

#define PRINTF(format, ...) { printf_P( PSTR(format), ##__VA_ARGS__);}


// for later
PROCESS(my_dhcp_process, "DHCP");
uint8_t dhcpStarted = 0;

/*---------------------------------------------------------------------------*/
PROCESS(clock_tick_process, "clock tick");

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(clock_tick_process, ev, data)
{
    static struct etimer timer;
    static uint8_t cnt = 0;

//    uint16_t i;
PROCESS_BEGIN();

    adc_init();

    while (0) {
        // wait here for the timer to expire
        

        //PRINTF("Clock Tick %d, temp: %f, vcc: %f\n",cnt, adc_sample_temperature(), adc_sample_vcc());

        PRINTF("temp: %u\n",adc_sample_temperature () );
//         char serial[6];
//         get_serial_number(serial, sizeof(serial));
//         for (i=0; i<sizeof(serial); i++) {
//             PRINTF("%x ", serial[i]);
//         }
        cnt += 1;
        etimer_set(&timer, CLOCK_CONF_SECOND * 8);
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/






#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

//prints whatever is in the uip_buf
void print_packet(void) {
  uint16_t i = 0;
  PRINTF("packet: ");
  for (i=0;i <uip_len; i++) {
    PRINTF("%x ",uip_buf[i]);
  }
  PRINTF("\n");
}

/*---------------------------------------------------------------------------*/
/*
 * We declare the process that we use to register with the TCP/IP stack,
 * and to check for incoming packets.
 */
PROCESS(enc28j60_process, "enc28j60 driver");
/*---------------------------------------------------------------------------*/
/*
 * Next, we define the function that transmits packets. This function
 * is called from the TCP/IP stack when a packet is to be transmitted.
 * The packet is located in the uip_buf[] buffer, and the length of the
 * packet is in the uip_len variable.
 */
u8_t
example_packet_driver_output(void)
{

  //PRINTF("in example_packet_driver_output, len is %d\n ",uip_len);
    
    //TODO: it looksm like uip_arp_out expect an ethernet-header-sized hole before the IP packet! (see uip_arp.c))
    //TODO what about uip_len?
  memmove((((void*)&uip_buf)) + 14, (&uip_buf), uip_len);

  
  //let_the_hardware_send_the_packet(uip_buf, uip_len);
  //PRINTF("packet to send, len: %d\n", uip_len);
  //print_packet();
  
  uip_arp_out();
  //PRINTF("in example_packet_driver_output, post arp, len is %d\n ",uip_len);

  //print_packet();

  enc28j60PacketSend(uip_len, uip_buf);
  /*
   * A network device driver returns always zero.
   */
  return 0;
}

/* another version, but doesn't do ARP*/
u8_t
example_packet_driver_output_noarp(void)
{

  //PRINTF("packet to send, already with arp, len: %d\n", uip_len);
  //print_packet();
  enc28j60PacketSend(uip_len, uip_buf);
  return 0;
}

static void
pollhandler(void)
{

    uint8_t i;
      for(i = 0; i < UIP_CONNS; i++) {
         //PRINTF("poll UIP %d\n", i);
         uip_periodic(i);
         /* If the above function invocation resulted in data that
            should be sent out on the network, the global variable
            uip_len is set to a value > 0. */
         if(uip_len > 0) {
            PRINTF("con caused send\n");
            example_packet_driver_output();
         }
       }


       for(i = 0; i < UIP_UDP_CONNS; i++) {
         uip_udp_periodic(i);
         /* If the above function invocation resulted in data that
            should be sent out on the network, the global variable
            uip_len is set to a value > 0. */
        if(uip_len > 0) {
            PRINTF("UDP con caused send\n");
            example_packet_driver_output();
         }
       }





  //TODO: is that really the right max packet length?
  uip_len = enc28j60PacketReceive(1500, uip_buf);//check_and_copy_packet();

  if(uip_len > 0) {
    
    //print_packet();
    if(BUF->type == uip_htons(UIP_ETHTYPE_IP)) {
        //PRINTF("RXed an IP packet, len: %d\n", uip_len);

        uip_arp_ipin();
        //PRINTF("RXed an IP packet, post ARP, len: %d, seq %lx\n", uip_len, uip_htonl(*(uint32_t*)(uip_buf + 38)));
        //print_packet();

        //tcpip_input will expect an IP packet, not an ethernet frame

        memmove((&uip_buf), ((void*)&uip_buf) + 14, uip_len);
        uip_len -= 14;
        //PRINTF("RX an IP packet START\n");
        tcpip_input();
        //PRINTF("RX an IP packet STOP\n");
        //uip_input();
/*        if(uip_len > 0) {
            uip_arp_out();
        }*/
//       uip_arp_ipin()
//       uip_len -= sizeof(struct uip_eth_hdr);
//       tcpip_input();
    } else if(BUF->type == uip_htons(UIP_ETHTYPE_ARP)) {
      //PRINTF("RXed an ARP packet, len: %d\n", uip_len);
      //print_packet();
      uip_arp_arpin();
      //PRINTF("RXed an ARP packet\n");
      /* If the above function invocation resulted in data that
         should be sent out on the network, the global variable
         uip_len is set to a value > 0. */
      if (uip_len > 0) {
          //PRINTF("RX of ARP packet causes more send\n");
          //enc28j60PacketSend(uip_len, uip_buf);
          example_packet_driver_output_noarp();
      }
    }
  }


  process_poll(&enc28j60_process);
}
/*---------------------------------------------------------------------------*/
/*
 * Finally, we define the process that does the work.
 */
PROCESS_THREAD(enc28j60_process, ev, data)
{

  PROCESS_POLLHANDLER(pollhandler());

  /*
   * This process has an exit handler, so we declare it here. Note that
   * the PROCESS_EXITHANDLER() macro must come before the PROCESS_BEGIN()
   * macro.
   */
  //PROCESS_EXITHANDLER(exithandler());

  uip_eth_addr our_uip_ethaddr;
//  uip_ipaddr_t addr;
  PROCESS_BEGIN();



  uip_init();
 

//   uip_setethaddr(our_uip_ethaddr);
//   uip_ipaddr(&addr, 192,168,1,223);
//   uip_sethostaddr(&addr);
//   uip_ipaddr(&addr, 255,255,255,0);
//   uip_setnetmask(&addr);
//   

  uip_arp_init();
  tcpip_set_outputfunc(example_packet_driver_output);


   //set the mac address to be our serial number
  uip_80211_addr tempMAC;
  get_serial_number(uip_ethaddr.addr, sizeof(our_uip_ethaddr));
  get_serial_number(&(tempMAC.addr[0]), 6);
  uip_setethaddr(tempMAC);

  PRINTF("e MAC set to: ");
  uint8_t i = 0;
  for (i=0; i<sizeof(uip_ethaddr.addr); i++){
    PRINTF("%2x ", uip_ethaddr.addr[i] );
  }
  PRINTF("\n");



  enc28j60Init(&uip_ethaddr);
  //PRINTF("e enc28j40 inited, rev %d\n", enc28j60getrev());


  
  
  process_poll(&enc28j60_process);
  //PRINTF("e eth starts with %x\n", uip_ethaddr.addr[0]);
  printf("network now initialized\n");
  //process_start(&my_dhcp_process, "dhcp_now");
  
  
  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_EXIT);

  //shutdown_the_hardware();
  PRINTF("shut down ethernet hardware.\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/












































//AUTOSTART_PROCESSES(&dhcp_process);


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(my_dhcp_process, ev, data)
{

//  static struct etimer timer;
//  static struct uip_udp_conn *udpconn;
  char* hostname = "poefan1";
  uint8_t i = 0;
  PROCESS_BEGIN();


//   udpconn = udp_broadcast_new(UIP_HTONS(67), NULL);
//   udp_bind(udpconn, UIP_HTONS(68));
  
  dhcpc_init(uip_ethaddr.addr, sizeof(uip_ethaddr.addr));
  dhcpc_set_hostname_p(hostname);
  PRINTF("dhcp inited with MAC addr: ");
  
  for (i=0; i<sizeof(uip_ethaddr.addr); i++){
    PRINTF("%2x ", uip_ethaddr.addr[i] );
  }
  PRINTF("\n");

  //etimer_set(&timer, CLOCK_CONF_SECOND*10);

//   dhcpc_request();
//   PRINTF("dhcp Requesting...\n");


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

#if 0
CLIF struct process * const autostart_processes[] = {&clock_tick_process, &lcd_test_process, &enc28j60_process,  &tcpip_process, &test_listener, &dhcper};

void
checkpoint_arch_init(void)
{

}
void
checkpoint_arch_checkpoint(void)
{

}
void
checkpoint_arch_rollback(void)
{

}

const struct process *procinit[1];
#endif