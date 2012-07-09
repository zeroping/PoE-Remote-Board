



#include "contiki.h"
#include "contiki-net.h"
#include <avr/pgmspace.h>


PROCESS_NAME(xPL_process);

#define XPL_PORT  3865

#define XPLNAME "smgpoe-lamp.3"
#define HOSTNAME "poelamp3"

extern char xplname[] PROGMEM;

#define XPL_HDR_HEAD "xpl-stat\n{\nhop=1\nsource="
#define XPL_TRIG_HDR_HEAD "xpl-trig\n{\nhop=1\nsource="
#define XPL_HDR_TAIL "\ntarget=*\n}\n"
#define XPL_HDR_W XPL_HDR_HEAD XPLNAME XPL_HDR_TAIL

#define HBEAT_MESSAGE "hbeat.basic\n{\ninterval=5\n}\n"

static char hbeatmessage [] PROGMEM = XPL_HDR_W HBEAT_MESSAGE;

#define SENSOR_MESSAGE "sensor.basic\n{\ndevice=%s\ntype=%s\ncurrent=%lu\nunits=%s\n}\n"


static char sensorformat [] PROGMEM = XPL_HDR_W SENSOR_MESSAGE;

#define BUTTON_MESSAGE "sensor.basic\n{\ndevice=%s\ntype=%s\ncurrent=%s\n}\n"

static char buttonformat [] PROGMEM = XPL_TRIG_HDR_HEAD XPLNAME XPL_HDR_TAIL BUTTON_MESSAGE;


