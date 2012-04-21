



#include "contiki.h"
#include "contiki-net.h"
#include <avr/pgmspace.h>


PROCESS_NAME(xPL_process);

#define XPL_PORT  3865

#define XPLNAME "smgpoe-lamp.2"
#define HOSTNAME "poelamp2"

extern char xplname[] PROGMEM;

#define XPL_HDR_HEAD "xpl-stat\n{\nhop=1\nsource="
#define XPL_HDR_TAIL "\ntarget=*\n}\n"
#define XPL_HDR XPL_HDR_HEAD XPLNAME XPL_HDR_TAIL

#define HBEAT_MESSAGE "hbeat.basic\n{\ninterval=5\n}\n"

static char hbeatmessage [] PROGMEM = XPL_HDR HBEAT_MESSAGE;

#define SENSOR_MESSAGE "sensor.basic\n{\ndevice=%s\ntype=%s\ncurrent=%lu\nunits=%s\n}\n"

static char sensorformat [] PROGMEM = XPL_HDR SENSOR_MESSAGE;

