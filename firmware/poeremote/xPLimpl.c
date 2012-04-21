
/**
 * \file
 *         A network application that listens on UDP port 50000 and echoes.
 * \author
 *         Clément Burin des Roziers <clement.burin-des-roziers@inrialpes.fr>
 */

#include "xPLimpl.h"

#include "contiki.h"
#include "contiki-net.h"

#include "dev/leds.h"
#include "dev/lcd/lcd.h"
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include <math.h>
#include <xmega-signatures.h>
#include <xmega-adc.h>

#define PRINTF(format, ...) { printf_P( PSTR(format), ##__VA_ARGS__);}

#define UDP_DATA_LEN 120
#define UDP_HDR ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

//TODO this is doubled for some reason - probably means our clock is wrong
#define HBEAT_INTERVAL (150 * CLOCK_SECOND)


#define BUTTON_0_PORT PORTD
#define BUTTON_0_BM (1<<3)


#define BUTTON_1_PORT PORTR
#define BUTTON_1_BM (1<<0)

#if 0

#define PWMPORT PORTC
#define PWMBIT (1<<2)
#define PWMTIMER TCC0
#define PWMSUBTIMER TC0_CCCEN_bm
#define PWMSUBCOUNTER CCC

#elif 1

#define PWMPORT PORTE
#define PWMBIT (1<<1)
#define PWMTIMER TCE0
#define PWMSUBTIMER TC0_CCBEN_bm
#define PWMSUBCOUNTER CCB
#endif



char xplname[] PROGMEM = XPLNAME;

static struct uip_udp_conn *udpconn;
static uint8_t udpdata[UDP_DATA_LEN] = "rx=";
static clock_time_t last_heartbeat = 0;

uip_ipaddr_t daddr;

/* take a character in buf and make an int out of it, so long as it int is no larger than max */
static uint8_t str_to_int(char* buf, uint8_t max){
    uint8_t out = 0;
    while(max) {
        if ((*buf >= '0') && (*buf<='9')) {
            out  = (out * 10 ) + (*buf - '0');
        } else {return out;}
        
        buf+=1;
        max--;
    }
    return out;
};

void initNL(void) {
    TCC0.CTRLA = (TC0_CLKSEL_gm & TC_CLKSEL_DIV8_gc);
    TCC0.CTRLB |= (TC0_WGMODE_gm & TC_WGMODE_SS_gc) | (TC0_CCAEN_bm)| (TC0_CCBEN_bm);
    TCC0.PER = 256*64;


}

void setNL(uint8_t val) {
    PRINTF("nlset\n");
    TCC0.CCA = val*64;
    TCC0.CCB = val*64;
    
}

void initPWM(void) {
    PWMTIMER.CTRLA = (TC0_CLKSEL_gm & TC_CLKSEL_DIV8_gc);
    PWMTIMER.CTRLB |= (TC0_WGMODE_gm & TC_WGMODE_SS_gc);
    PWMTIMER.PER = 256*64;
}

/* sets the PWM output on the assigned PWM pin */
static void setPWM(uint8_t val) {

//     if (val > 130) {
//         val = 130;
//     }
    
    //val = 255-val;
    PRINTF("pwmset\n");
    if (val == 255 ) {
        PWMTIMER.CTRLB &= ~(PWMSUBTIMER);
        PWMPORT.DIRSET = PWMBIT;
        PWMPORT.OUTSET = PWMBIT;
    } else if (val == 0) {
        PWMTIMER.CTRLB &= ~(PWMSUBTIMER);
        PWMPORT.DIRSET = PWMBIT;
        PWMPORT.OUTCLR = PWMBIT;
    } else {
        PWMPORT.OUTCLR = PWMBIT;
        PRINTF("here\n");
        PWMPORT.DIRSET = PWMBIT;

        PWMTIMER.CTRLB |= (PWMSUBTIMER);
        
        PWMTIMER.PWMSUBCOUNTER = val*64;
    }
}

/* handles an XPL control.basic message */
static int handle_control() {
    char* message = (char*)uip_appdata;
    //PRINTF("msg: %s", message);
    char* curs;
    if (strcasestr(message, "device=pwm") != NULL){
        if (strcasestr(message, "type=variable") != NULL){
            curs = strcasestr(message, "current=");
            curs += 8;
            uint8_t val = str_to_int(curs, 3);
            PRINTF("got %u \n", val);
            setPWM(val);
            setNL(val);
        } else return 0;
    } else {
        PRINTF("unknown control message\n");
        return 0;
    }
    return 1;
}

/* handles an XPL sensor.request message*/
static int handle_sensor() {
    char* message = (char*)uip_appdata;
    //PRINTF("msg: %s", message);
    if (strcasestr(message, "request=current") != NULL){
        if (strcasestr(message, "device=temp") != NULL){
            PRINTF("temp");
            uint32_t t = 0;
            uint16_t i;
            for (i=0; i<100; i++){
                t += adc_sample_temperature();
                //_delay_us(100);
            }
            //t=t;
            
            uip_len = sprintf_P(message, sensorformat, "temperature","temp",t,"custom");

//             uip_ipaddr_t addr;
//             uip_ipaddr(&addr, 192,168,1,255);
            uip_udp_packet_sendto(uip_udp_conn,uip_appdata,uip_len,&daddr,UIP_HTONS(3865));
            //uip_udp_send(uip_len);
            PRINTF(".\n");
            return 1;
        } else return 0;
    } else {
        PRINTF("unknown sensor message\n");
        return 0;
    }
    return 1;
}

/* parses an incomming XPL packet */
static void parse_incomming() {
    printf("start\n");
    //start by writing an extra null at the end of the string
    char* endp = ((char*)uip_appdata + uip_datalen());
    *endp = '\0';
    //TODO what if this is past the end of the buffer?

    char* message = (char*)uip_appdata;
    //PRINTF("Received from %u.%u.%u.%u:%u: '%s'\n", uip_ipaddr_to_quad(&UDP_HDR->srcipaddr), UIP_HTONS(UDP_HDR->srcport), (char*)uip_appdata);

    char* targetp = NULL;
    char* target_txt = "target=";

    targetp = strcasestr((const char*)message, target_txt);
    if (targetp != 0) {
        //so we have "target=" in the message
        //targetp+= sizeof(target_txt) - 1;
        PRINTF("has target\n");
        //push this to the end of the string;
        targetp= strchr(targetp, '=');
        if(targetp==NULL) {
            PRINTF("No = in target line?");
            return;
        }
        targetp+=1;
        
        PRINTF("our name is: ");
        printf_P(&(xplname[0]));
        PRINTF(", message is for: ");
        char buf[20];
        uint8_t namechrs = strchrnul(targetp, '\n') - targetp ;
        if(namechrs > 19) namechrs = 19;
        memcpy(buf,targetp,namechrs);
        buf[namechrs] = '\0';
        printf(buf);
        PRINTF("\n");
        
        if(strcasestr_P(targetp, &(xplname[0]))
            || strcasestr(targetp, "*")) {
            //if we're called directly, or there's an asterisk
            //TODO: should we support something like "target=smgpoe-fan.*" ?

            PRINTF("we're the target\n");
            if (strcasestr(message, "control.basic") != NULL && handle_control()){

            } else if (strcasestr(message, "sensor.request") != NULL && handle_sensor()){

            } else {
                PRINTF("unknown XPL message, ignoring.\n");
            }
        }

    } else {
        PRINTF("XPL message with no target= line\n");
    }

}
/*
void initButtons(void) {
    //make the button lines inputs
}*/

uint8_t pollButtons(void) {

    return 0;
}

/*to be called when there's a tcpip event, be it a packet of a poll */
static void udphandler(const process_event_t ev, const process_data_t data)
{
    if (!uip_udpconnection()) {
        PRINTF("is TCP\n");
        return;
    }

    if (uip_poll()) {
        clock_time_t heartbeat_countdown = clock_time() - last_heartbeat;
        if(heartbeat_countdown > HBEAT_INTERVAL) {
            PRINTF("hbeat timer!\n");

            memcpy_P(udpdata, hbeatmessage, sizeof(hbeatmessage));
            uip_udp_packet_sendto(udpconn, udpdata,sizeof(hbeatmessage)-1, &daddr, UIP_HTONS(3865));
            last_heartbeat = clock_time();
            return;
        }
        
    }
    
    if (uip_newdata())
    {
        /* Set the last byte of the received data as 0 in order to print it. */
        int len = uip_datalen();
        ((char *)uip_appdata)[len] = 0;
        //PRINTF("RReceived from %u.%u.%u.%u:%u: '%s'\n", uip_ipaddr_to_quad(&UDP_HDR->srcipaddr), UIP_HTONS(UDP_HDR->srcport), (char*)uip_appdata);

        parse_incomming();

    }
}

/*---------------------------------------------------------------------------*/
/*
 * We declare the process.
 */
PROCESS(xPL_process, "xPL process");
//AUTOSTART_PROCESSES(&xPL_process);
/*---------------------------------------------------------------------------*/
/*
 * The definition of the process.
 */
PROCESS_THREAD(xPL_process, ev, data)
{

    PROCESS_BEGIN();
    initNL();
    setNL(1);
    initPWM();
    setPWM(1);
    PRINTF("xPL listener\n");

    last_heartbeat = clock_time() - HBEAT_INTERVAL;


    //do some legwork to figure out our local broadcast address
    //TODO: this will fail if we have one of the funny subnets with a netmask that has a partial octect
    static uip_ipaddr_t baddr;
    uip_getnetmask(&baddr);
    uip_gethostaddr(&daddr);
    int i = 0;
    printf("dest addr: ");
    for (i=0; i<4; i++) {
        //printf("add: %d", baddr.u8[i]);
        if (baddr.u8[i] != 255)
            daddr.u8[i] = 255;
        printf("%d.", daddr.u8[i]);
    }
    printf("\n");
    
    //uip_ipaddr_copy(&daddr, &uip_broadcast_addr);
    //uip_ipaddr(&daddr, 192, 168, 1,255);
    //uip_ipaddr(&daddr, 255,255,255,255);
    
    //uip_ipaddr_t addr;
    /* Create a UDP 'connection' for broadcast packets on port 3865 */
    udpconn = udp_broadcast_new(0, NULL);

    if (udpconn == NULL) {
        PRINTF("cannot alloc!\n");
    } 
    /* Bind the UDP 'connection' to the xpl port. That's the port we're listening on. */

    udp_bind(udpconn, UIP_HTONS(XPL_PORT));
    
    PRINTF("listening on UDP port %u\n", UIP_HTONS(udpconn->lport));

    while(1) {
        /* Wait until we have an event caused by tcpip interaction, or tcpip poll */
        PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
        //PRINTF("tcpipevent\n");
        /* Handle it */
        udphandler(ev, data);
    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/

