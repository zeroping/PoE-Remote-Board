
#ifndef _ENC28J60_PINS_H
#define _ENC28J60_PINS_H


#define PRINTF(format, args...) ((void)0)

#define _ENC28J60_PINS 1

#if (0)
//old board
#define ENC28J60_CONTROL_CS_PORT   PORTD
#define ENC28J60_CONTROL_CS     4

// #define ENC28J60_CONTROL_RESET_PORT    PORTD
// #define ENC28J60_CONTROL_RESET  0

#define ENC28J60_CONTROL_INT_PORT   PORTE
#define ENC28J60_CONTROL_INT     0


#elif (1)
//new board
//#define ENC28J60_CONTROL_RESET_PORT    PORTE
//#define ENC28J60_CONTROL_RESET  0
#define ENC28J60_CONTROL_CS_PORT   PORTE
#define ENC28J60_CONTROL_CS     0
#define ENC28J60_CONTROL_INT_PORT   PORTD
#define ENC28J60_CONTROL_INT     4


#endif

#define SPIPORT SPID
#define SPIPINPORT PORTD

#endif