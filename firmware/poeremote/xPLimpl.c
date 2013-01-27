
/**
 * \file
 *         A network application that listens on UDP port 50000 and echoes.
 * \author
 *         Clément Burin des Roziers <clement.burin-des-roziers@inrialpes.fr>
 */
#include <stdlib.h>
#include "xPLimpl.h"

#include "contiki.h"
#include "contiki-net.h"

#include <stddef.h>
#include "dev/leds.h"
#include "dev/light/light.h"
#include "dev/buttons/buttons.h"
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
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




#if 0

#define PWMPORT PORTC
#define PWMBIT (1<<2)
#define PWMTIMER TCC0
#define PWMSUBTIMER TC0_CCCEN_bm
#define PWMSUBCOUNTER CCC

#elif 1

//new wall switch board

#define PWMPORT PORTE
#define PWMBIT (1<<1)
#define PWMTIMER TCE0
#define PWMSUBTIMER TC0_CCBEN_bm
#define PWMSUBCOUNTER CCB

//used to divide the PWM frequency an arbitrary amount
#define PWMDIVISOR 1
//used to extend the PWM period to cut down the maximum power. 256 = no change, 512 = 50% max
#define PWMEXTEND 340


#endif


#define MIN(a,b)      ((a<b)?(a):(b)) 
#define MAX(a,b)      ((a>b)?(a):(b)) 
#define ABS(x)        ((x>0)?(x):(-x)) 

uint8_t buttonsLast= 0;


uint8_t motionLast = 0;
uint8_t motionval = 0;
uint8_t motiontime = 0;


volatile uint8_t nlstartval = 0;
volatile uint8_t nlendval = 0;
volatile uint16_t nlsteps = 0;

volatile uint8_t pmstartval = 0;
volatile uint8_t pmendval = 0;
volatile uint16_t pmsteps = 0;

struct persistentconfig perconf = {0x14,1000,900,1000,"smgpoe-lamp.4"};

char const xplname[] PROGMEM = XPLNAME;

static struct uip_udp_conn *udpconn;
static uint8_t udpdata[UDP_DATA_LEN] = "rx=";
static clock_time_t last_heartbeat = 0;

uip_ipaddr_t daddr;

/* take a character in buf and make an int out of it, so long as it int is no larger than max */
static uint8_t str_to_int ( char* buf, uint8_t max )
{
    uint8_t out = 0;
    while ( max )
    {
        if ( ( *buf >= '0' ) && ( *buf<='9' ) )
        {
            out  = ( out * 10 ) + ( *buf - '0' );
        }
        else
        {
            return out;
        }

        buf+=1;
        max--;
    }
    return out;
};

static uint16_t str_to_int_16 ( char* buf, uint16_t max )
{
    uint16_t out = 0;
    while ( max )
    {
        if ( ( *buf >= '0' ) && ( *buf<='9' ) )
        {
            out  = ( out * 10 ) + ( *buf - '0' );
        }
        else
        {
            return out;
        }
        
        buf+=1;
        max--;
    }
    return out;
};

void initNL ( void )
{
    PORTC.DIR |= 0x03;
    TCC0.CTRLA = ( TC0_CLKSEL_gm & TC_CLKSEL_DIV8_gc );
    TCC0.CTRLB |= ( TC0_WGMODE_gm & TC_WGMODE_SS_gc ) | ( TC0_CCAEN_bm ) | ( TC0_CCBEN_bm );
    TCC0.PER = 256*32;
}

void setNL ( uint8_t val )
{
    val = MIN(255,val);
    TCC0.CCA = val*32;
    TCC0.CCB = val*32;
}

void gotoNL(uint8_t val) {
    if(perconf.fadetime <10){
        setNL(val);
    } else {
        nlstartval = (nlendval) + (((int16_t)((int16_t)nlstartval-(int16_t)nlendval) * (int16_t)nlsteps)/(perconf.fadetime / 10) ) ;
        nlendval = val;
        nlsteps = perconf.fadetime / 10;
    }
}

void initPWM ( void )
{
    PWMTIMER.CTRLA = ( TC0_CLKSEL_gm & TC_CLKSEL_DIV2_gc );
    //PWMTIMER.CTRLA = ( TC0_CLKSEL_gm & TC_CLKSEL_DIV1_gc );
    PWMTIMER.CTRLB |= ( TC0_WGMODE_gm & TC_WGMODE_SS_gc );
//    PWMTIMER.PER = 256*4;
    //if we want to ensure that we never run at full power, we can do this to make max power 256/340 = 75%
    PWMTIMER.PER = PWMEXTEND*PWMDIVISOR;
}


/* sets the PWM output on the assigned PWM pin */
void setPWM ( uint8_t val )
{
    val = MIN(255,val);
//     PRINTF ( "pwmset %u \n", val );
    if ( val == 0 )
    {
//         PRINTF ( "pwm off\n");
        PWMTIMER.CTRLB &= ~ ( PWMSUBTIMER );
        PWMPORT.DIRSET = PWMBIT;
        PWMPORT.OUTSET = PWMBIT;
    }
//     else if ( val == 255 )
//     {
//         PRINTF ( "pwm on\n");
//         PWMTIMER.CTRLB &= ~ ( PWMSUBTIMER );
//         PWMPORT.DIRSET = PWMBIT;
//         PWMPORT.OUTCLR = PWMBIT;
//     }
    else
    {
//         PRINTF ( "pwm var\n");
        PWMPORT.OUTCLR = PWMBIT;
        PWMPORT.DIRSET = PWMBIT;

        PWMTIMER.CTRLB |= ( PWMSUBTIMER );

        PWMTIMER.PWMSUBCOUNTER = (PWMEXTEND-val)*PWMDIVISOR;
    }
}

void gotoPWM(uint8_t val) {
    if(perconf.fadetime <10){
        setPWM(val);
    } else {
        pmstartval = (pmendval) + (((int16_t)((int16_t)pmstartval-(int16_t)pmendval) * (int16_t)pmsteps)/(perconf.fadetime / 10) ) ;
        pmendval = val;
        pmsteps = perconf.fadetime / 10;
    }
}




/**
 * @brief Reads the persistent storage data into memory if it's valid
 **/
void persistent_init( void )
{
    if(eeprom_read_byte(0) == perconf.magic) {
        PRINTF("perconf is valid\n");
        eeprom_read_block(&perconf, 0, sizeof(perconf));
    } else {
        PRINTF("perconf is invalid: %x\n", eeprom_read_byte(0));
    }
    PRINTF("Our instance ID is \"%s\"\n", perconf.instanceid);
}




/* handles an XPL control.basic message */
static int handle_control()
{
    char* message = ( char* ) uip_appdata;
    //PRINTF("msg: %s", message);
    char* curs;
    if ( strcasestr ( message, "device=pwm" ) != NULL )
    {
        if ( strcasestr ( message, "type=variable" ) != NULL )
        {
            curs = strcasestr ( message, "current=" );
            curs += 8;
            uint8_t val = str_to_int ( curs, 3 );
          
            gotoPWM ( val );
        }
        else return 0;
    }
    else if ( strcasestr ( message, "device=lamp" ) != NULL )
    {
        if ( strcasestr ( message, "type=variable" ) != NULL )
        {
            curs = strcasestr ( message, "current=" );
            curs += 8;
            uint8_t val = str_to_int ( curs, 3 );
            PRINTF ( "got %u \n", val );
            gotoPWM ( val );
        }
        else return 0;
    }
    else if ( strcasestr ( message, "device=nightlight" ) != NULL )
    {
        if ( strcasestr ( message, "type=variable" ) != NULL )
        {
            curs = strcasestr ( message, "current=" );
            curs += 8;
            uint8_t val = str_to_int ( curs, 3 );
            val = MIN(255,val);
            PRINTF ( "got %u \n", val );
//             setNL ( val );
            gotoNL(val);
        }
        else return 0;
    }
    else
    {
        PRINTF ( "unknown control message\n" );
        return 0;
    }
    return 1;
}

/* handles an XPL sensor.request message*/
static int handle_sensor()
{
    char* message = ( char* ) uip_appdata;
    //PRINTF("msg: %s", message);
    if ( strcasestr ( message, "request=current" ) != NULL )
    {
        if ( strcasestr ( message, "device=temp" ) != NULL )
        {
            uint32_t t = 0;
            uint16_t i;
            for ( i=0; i<100; i++ )
            {
                t += adc_sample_temperature();
                //_delay_us(100);
            }

            uip_len = sprintf_P ( message, sensorformat, perconf.instanceid, "temperature","temp",t,"custom" );

            uip_udp_packet_sendto ( uip_udp_conn,uip_appdata,uip_len,&daddr,UIP_HTONS ( 3865 ) );
            //uip_udp_send(uip_len);
            PRINTF ( ".\n" );
            return 1;
        } else if ( strcasestr ( message, "device=light" ) != NULL )
        {

            uip_len = sprintf_P ( message, sensorformatlong, "light","light",lightSample(),"lux" );
            uip_udp_packet_sendto ( uip_udp_conn,uip_appdata,uip_len,&daddr,UIP_HTONS ( 3865 ) );
            return 1;
        }
        
        else return 0;
    }
    else
    {
        PRINTF ( "unknown sensor message\n" );
        return 0;
    }
    return 1;
}

void commit_persist ( void ) {
    PRINTF("persistent config\n");
    PRINTF("instanceid = %s\n",(perconf.instanceid)+ strlen_P(XPLNAMESTART));
    PRINTF("motiontime = %u\n",perconf.motiontime);
    PRINTF("motionsens = %u\n",perconf.motionsens);
    PRINTF("fade-rate = %u\n",perconf.fadetime);
    
    eeprom_update_block(&perconf, 0, sizeof(perconf));
}
    
    


uint8_t config_list() {
    char* message = ( char* ) uip_appdata;
    if ( strncasecmp( message, XPL_CMND_STR, strlen(XPL_CMND_STR)) != 0 ) {
        printf("not a cmnd\n");
        return 0;
    }
    if ( strcasestr ( message, "command=request" ) != NULL )
    {
        printf("a request");
        char* message = ( char* ) uip_appdata;
        uip_len = sprintf_P ( message, configlistformat );
        uip_udp_packet_sendto ( uip_udp_conn,uip_appdata,uip_len,&daddr,UIP_HTONS ( 3865 ) );
        return 1;
    }
    return 0;
}

uint8_t config_response() {
    char* message = ( char* ) uip_appdata;
    if ( strncasecmp( message, XPL_CMND_STR, strlen(XPL_CMND_STR)) != 0 ) {
        printf("not a cmnd\n");
        return 0;
    }
    printf("a response\n");
    char* curs = strcasestr ( message, "fade-rate=" );
    if(curs != NULL) {
        curs += 10;
        uint16_t val = str_to_int_16 ( curs, 5 );
        perconf.fadetime=val;
    }
    curs = strcasestr ( message, "motiontime=" );
    if(curs != NULL) {
        curs += 11;
        uint16_t val = str_to_int_16 ( curs, 5 );
        perconf.motiontime=val;
    }
    curs = strcasestr ( message, "motionsens=" );
    if(curs != NULL) {
        curs += 11;
        uint16_t val = str_to_int_16 ( curs, 5 );
        perconf.motionsens=val;
    }
    curs = strcasestr ( message, "newconf=" );
    if(curs != NULL) {
        curs += 8;
        char* end = strchrnul ( curs, '\n' );
        uint8_t namelen = end-curs;
        memcpy(perconf.instanceid + strlen_P(XPLNAMESTART), curs, namelen);
        perconf.instanceid[namelen + strlen_P(XPLNAMESTART) ] = '\0';
    }
    commit_persist();
    return 1;
}

uint8_t config_current() {
    char* message = ( char* ) uip_appdata;
    if ( strncasecmp( message, XPL_CMND_STR, strlen(XPL_CMND_STR)) != 0 ) {
        printf("not a cmnd\n");
        return 0;
    }
    if ( strcasestr ( message, "command=request" ) != NULL )
    {
        printf("req current");
        char* message = ( char* ) uip_appdata;

        uip_len = sprintf_P ( message, configcurrentformat, perconf.instanceid, perconf.instanceid, perconf.motionsens, perconf.motiontime, perconf.fadetime);
        uip_udp_packet_sendto ( uip_udp_conn,uip_appdata,uip_len,&daddr,UIP_HTONS ( 3865 ) );
        return 1;
    }
    return 0;
}



/* parses an incomming XPL packet */
static void parse_incomming()
{
    //start by writing an extra null at the end of the string
    if (uip_datalen() == UIP_BUFSIZE) {
        //we need to add a /0 to the end, but we can't, so we give up
        return;
    }
    char* endp = ( ( char* ) uip_appdata + uip_datalen() );
    *endp = '\0';

    char* message = ( char* ) uip_appdata;
    //PRINTF("Received from %u.%u.%u.%u:%u: '%s'\n", uip_ipaddr_to_quad(&UDP_HDR->srcipaddr), UIP_HTONS(UDP_HDR->srcport), (char*)uip_appdata);

    char* targetp = NULL;
    char* target_txt = "target=";

    targetp = strcasestr ( ( const char* ) message, target_txt );
    if ( targetp != 0 )
    {
        //so we have "target=" in the message
        //push this to the end of the string;
        targetp= strchr ( targetp, '=' );
        if ( targetp==NULL )
        {
            PRINTF ( "No = in target line?" );
            return;
        }
        targetp+=1;

//         PRINTF ( "our name is: " );
//         printf_P ( & ( xplname[0] ) );
//         PRINTF ( ", message is for: " );
        char buf[20];
        uint8_t namechrs = strchrnul ( targetp, '\n' ) - targetp ;
        if ( namechrs > 19 ) namechrs = 19;
        memcpy ( buf,targetp,namechrs );
        buf[namechrs] = '\0';
//         printf ( buf );
//         PRINTF ( "\n" );

        if ( strcasestr ( targetp, perconf.instanceid )
                || strcasestr ( targetp, "*" ) )
        {
            //if we're called directly, or there's an asterisk
            //TODO: should we support something like "target=smgpoe-fan.*" ?

            PRINTF ( "xPL: message for us\n" );
            if ( strcasestr ( message, "control.basic" ) != NULL && handle_control() )
            {

            }
//             else if ( strcasestr ( message, "sensor.request" ) != NULL && handle_sensor() )
//             {
//                 
//             }
            else if ( strcasestr ( message, "config.list" ) != NULL && config_list() ) { }
            else if ( strcasestr ( message, "config.response" ) != NULL && config_response() ) { }
            else if ( strcasestr ( message, "config.current" ) != NULL && config_current() ) { }

            else
            {
                //PRINTF ( "xPL unknown XPL message, ignoring.\n" );
            }
        }

    }
    else
    {
        PRINTF ( "XPL message with no target= line\n" );
    }

}

uint8_t motionPoll ( void )
{  
    if ( motionval != motionLast )
    {
        motionLast = motionval;
        PRINTF ( "mo: %d %d\n", motionval );
        //memcpy_P(udpdata, hbeatmessage, sizeof(hbeatmessage));
        uint16_t mylen = 0;
        if ( motionval)
        {
            mylen = sprintf_P ( (char*)udpdata, motionformat, perconf.instanceid, "motion","input","HIGH" );
        }
        else
        {
            mylen = sprintf_P ( (char*)udpdata, motionformat, perconf.instanceid, "motion","input","LOW" );
        }

        uip_udp_packet_sendto ( udpconn, udpdata, mylen, &daddr, UIP_HTONS ( 3865 ) );
        return 1;
    }
    return 0;
}

//checks the button input lines, and returns 1 if something has changed.
uint8_t pollButtons ( void )
{
    uint8_t curr = buttonsRead();
    if ( curr != buttonsLast )
    {
        PRINTF ( "sw: %d %d\n", curr, buttonsLast );
        //memcpy_P(udpdata, hbeatmessage, sizeof(hbeatmessage));
        uint16_t mylen = 0;
        if ( curr & ~buttonsLast & 0x01 )
        {
            mylen = sprintf_P ( (char*)udpdata, buttonformat, perconf.instanceid, "button1","input","HIGH" );
            buttonsLast |= 0x01;
        }
        else if ( curr & ~buttonsLast & 0x02 )
        {
            mylen = sprintf_P ( (char*)udpdata, buttonformat, perconf.instanceid, "button2","input","HIGH" );
            buttonsLast |= 0x02;
        }
        else if ( ~curr & buttonsLast & 0x01 )
        {
            mylen = sprintf_P ( (char*)udpdata, buttonformat, perconf.instanceid, "button1","input","LOW" );
            buttonsLast &= ~0x01;
        }
        else if ( ~curr & buttonsLast & 0x02 )
        {
            mylen = sprintf_P ( (char*)udpdata, buttonformat, perconf.instanceid, "button2","input","LOW" );
            buttonsLast &= ~0x02;
        }
        uip_udp_packet_sendto ( udpconn, udpdata, mylen, &daddr, UIP_HTONS ( 3865 ) );
        return 1;
    }
    return 0;
}


/*to be called when there's a tcpip event, be it a packet of a poll */
static void udphandler ( const process_event_t ev, const process_data_t data )
{
    if ( !uip_udpconnection() )
    {
        PRINTF ( "is TCP\n" );
        return;
    }

    if ( uip_poll() )
    {
        clock_time_t heartbeat_countdown = clock_time() - last_heartbeat;
//         if (pollButtons()) {
//             buttonsSend();
//         }
//

        if ( heartbeat_countdown > HBEAT_INTERVAL )
        {
            PRINTF ( "hbeat timer!\n" );

            memcpy_P ( udpdata, hbeatmessage, sizeof ( hbeatmessage ) );
            uip_udp_packet_sendto ( udpconn, udpdata,sizeof ( hbeatmessage )-1, &daddr, UIP_HTONS ( 3865 ) );
            last_heartbeat = clock_time();
            return;
        }
        else  if ( pollButtons() )
        {
            return;
        }
        else if ( motionPoll() )
        {
            return;
        }


    }


    if ( uip_newdata() )
    {
        /* Set the last byte of the received data as 0 in order to print it. */
        int len = uip_datalen();
        ( ( char * ) uip_appdata ) [len] = 0;
        //PRINTF("RReceived from %u.%u.%u.%u:%u: '%s'\n", uip_ipaddr_to_quad(&UDP_HDR->srcipaddr), UIP_HTONS(UDP_HDR->srcport), (char*)uip_appdata);

        parse_incomming();

    }
}

/*---------------------------------------------------------------------------*/
/*
 * We declare the process.
 */
PROCESS ( xPL_process, "xPL process" );
//AUTOSTART_PROCESSES(&xPL_process);
/*---------------------------------------------------------------------------*/
/*
 * The definition of the process.
 */
PROCESS_THREAD ( xPL_process, ev, data )
{

    PROCESS_BEGIN();
    initNL();
    setNL ( 1 );
    initPWM();
    setPWM ( 0 );
    buttonsInit();
    persistent_init();

    PRINTF ( "xPL listener\n" );

    last_heartbeat = clock_time() - HBEAT_INTERVAL;


    //do some legwork to figure out our local broadcast address
    //TODO: this will fail if we have one of the funny subnets with a netmask that has a partial octect
    static uip_ipaddr_t baddr;
    uip_getnetmask ( &baddr );
    uip_gethostaddr ( &daddr );
    int i = 0;
    printf ( "dest addr: " );
    for ( i=0; i<4; i++ )
    {
        //printf("add: %d", baddr.u8[i]);
        if ( baddr.u8[i] != 255 )
            daddr.u8[i] = 255;
        printf ( "%d.", daddr.u8[i] );
    }
    printf ( "\n" );

    //uip_ipaddr_copy(&daddr, &uip_broadcast_addr);
    //uip_ipaddr(&daddr, 192, 168, 1,255);
    //uip_ipaddr(&daddr, 255,255,255,255);

    //uip_ipaddr_t addr;
    /* Create a UDP 'connection' for broadcast packets on port 3865 */
    udpconn = udp_broadcast_new ( 0, NULL );

    if ( udpconn == NULL )
    {
        PRINTF ( "cannot alloc!\n" );
    }
    /* Bind the UDP 'connection' to the xpl port. That's the port we're listening on. */

    udp_bind ( udpconn, UIP_HTONS ( XPL_PORT ) );

    PRINTF ( "listening on UDP port %u\n", UIP_HTONS ( udpconn->lport ) );

    while ( 1 )
    {
        /* Wait until we have an event caused by tcpip interaction, or tcpip poll */
        PROCESS_WAIT_EVENT_UNTIL ( ev == tcpip_event );
        //PRINTF("tcpipevent\n");
        /* Handle it */
        udphandler ( ev, data );
    }


    PROCESS_END();
}
/*---------------------------------------------------------------------------*/




/*---------------------------------------------------------------------------*/
PROCESS ( clock_tick_process, "clock tick" );



/*---------------------------------------------------------------------------*/
PROCESS_THREAD ( clock_tick_process, ev, data )
{
    static struct etimer timer;

    //    uint16_t i;
    PROCESS_BEGIN();

    adc_init();


    while ( 1 )
    {
        //wait here for the timer to expire

        int16_t sample = adc_sample_differential ( 0 );

        uint8_t motionmax = perconf.motiontime/100;
        uint8_t motionrat = 10;

        if ( ( abs ( sample ) >perconf.motionsens ) && ( motiontime < motionmax ) )
        {
            motiontime += motionrat;
            if(motiontime>perconf.motiontime/100)
                motionval = 1;
        }
        else if ( motiontime > 0 )
        {
            motiontime -= 1;
        }
        else if (motiontime == 0) {
            motionval = 0;
        }
//          printf("motiontime %d\t sample: %d\n",  motiontime, sample);
        etimer_set ( &timer, CLOCK_CONF_SECOND/10 );
        PROCESS_WAIT_EVENT_UNTIL ( ev == PROCESS_EVENT_TIMER );

    }

    PROCESS_END();
}

PROCESS ( fade_process, "fade" );
PROCESS_THREAD ( fade_process, ev, data )
{
    static struct etimer timer2;
    
    PROCESS_BEGIN();
    while ( 1 )
    {
        if(nlsteps > 0) {
            nlsteps -= 1;
            
            //as nlsteps goes from perconf.fadetime to 0
            // we'll take set from old val (nlcurval?) to nlval
            int16_t totalsteps = perconf.fadetime/10;
            uint8_t toval = (nlendval) + (((int16_t)((int16_t)nlstartval-(int16_t)nlendval) * (int16_t)nlsteps)/(totalsteps) ) ;
            //             printf("s:%u e:%u s=%u/%u to%u\n", nlstartval, nlendval, nlsteps, totalsteps,toval);
            setNL(toval);
        }
        if(pmsteps > 0) {
            pmsteps -= 1;
            
            //as nlsteps goes from perconf.fadetime to 0
            // we'll take set from old val (nlcurval?) to nlval
            int16_t totalsteps = perconf.fadetime/10;
            uint8_t toval = (pmendval) + (((int16_t)((int16_t)pmstartval-(int16_t)pmendval) * (int16_t)pmsteps)/(totalsteps) ) ;
            //             printf("s:%u e:%u s=%u/%u to%u\n", nlstartval, nlendval, nlsteps, totalsteps,toval);
            setPWM(toval);
        }
        etimer_set ( &timer2, CLOCK_CONF_SECOND/100 );
        PROCESS_WAIT_EVENT_UNTIL ( ev == PROCESS_EVENT_TIMER );
    }
    PROCESS_END();
}

/*---------------------------------------------------------------------------*/


