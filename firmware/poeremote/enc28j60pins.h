
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


#elif (0)
//new board
#define ENC28J60_CONTROL_RESET_PORT    PORTE
#define ENC28J60_CONTROL_RESET  0
#define ENC28J60_CONTROL_CS_PORT   PORTD
#define ENC28J60_CONTROL_CS     2
#define ENC28J60_CONTROL_INT_PORT   PORTD
#define ENC28J60_CONTROL_INT     4

#elif (1)
//new NEW board (v2.2)
#define ENC28J60_CONTROL_RESET_PORT    PORTE
#define ENC28J60_CONTROL_RESET  0
#define ENC28J60_CONTROL_CS_PORT   PORTD
#define ENC28J60_CONTROL_CS     4
#define ENC28J60_CONTROL_INT_PORT   PORTD
#define ENC28J60_CONTROL_INT     3

#endif

//static spi_xmega_slave_t enc28j60_spi[1] = {{&PORTD,&SPID.DATA, &SPID.CTRL, &SPID.STATUS, &PORTD, (1<<2), SPI_CLK2X_bm,0}};
static spi_xmega_slave_t enc28j60_spi[1] = {{&PORTD,&SPID.DATA, &SPID.CTRL, &SPID.STATUS, &PORTD, (1<<4), SPI_CLK2X_bm,0}};    




#define SPIPORT SPID
#define SPIPINPORT PORTD

#endif