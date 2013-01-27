



#include "contiki.h"
#include "contiki-net.h"
#include <avr/pgmspace.h>


PROCESS_NAME(xPL_process);
PROCESS_NAME(clock_tick_process);

#define XPL_PORT  3865

#define XPLNAME "smgpoe-lamp.5"
#define HOSTNAME "poelamp5"

extern const char xplname[] PROGMEM;

#define XPL_TRIG_STR "xpl-trig"
#define XPL_STAT_STR "xpl-stat"
#define XPL_CMND_STR "xpl-cmnd"

#define XPL_STAT_HDR_HEAD "xpl-stat\n{\nhop=1\nsource="
#define XPL_TRIG_HDR_HEAD "xpl-trig\n{\nhop=1\nsource="
#define XPL_HDR_TAIL "\ntarget=*\n}\n"
#define XPL_HDR_W XPL_STAT_HDR_HEAD XPLNAME XPL_HDR_TAIL

#define HBEAT_MESSAGE "hbeat.basic\n{\ninterval=5\n}\n"

static const char hbeatmessage [] PROGMEM = XPL_HDR_W HBEAT_MESSAGE;

#define SENSOR_MESSAGE "sensor.basic\n{\ndevice=%s\ntype=%s\ncurrent=%lu\nunits=%s\n}\n"

#define SENSOR_MESSAGE_LONG "sensor.basic\n{\ndevice=%s\ntype=%s\ncurrent=%g\nunits=%s\n}\n"


static const char sensorformat [] PROGMEM = XPL_HDR_W SENSOR_MESSAGE;
static const char sensorformatlong [] PROGMEM = XPL_HDR_W SENSOR_MESSAGE_LONG;

#define BUTTON_MESSAGE "sensor.basic\n{\ndevice=%s\ntype=%s\ncurrent=%s\n}\n"

static const char buttonformat [] PROGMEM = XPL_TRIG_HDR_HEAD XPLNAME XPL_HDR_TAIL BUTTON_MESSAGE;

#define MOTION_MESSAGE "sensor.basic\n{\ndevice=%s\ntype=%s\ncurrent=%s\n}\n"

static const char motionformat [] PROGMEM = XPL_TRIG_HDR_HEAD XPLNAME XPL_HDR_TAIL MOTION_MESSAGE;

#define CONFIGLIST_MESSAGE "config.list\n{\nreconf=newconf\nreconf=motionsens\nreconf=motiontime\n}\n"

static const char configlistformat [] PROGMEM = XPL_STAT_HDR_HEAD XPLNAME XPL_HDR_TAIL CONFIGLIST_MESSAGE;

#define CONFIGCURRENT_MESSAGE "config.list\n{\nnewconf=%s\nmotionsens=%u\nmotiontime=%u\n}\n"

static const char configcurrentformat [] PROGMEM = XPL_STAT_HDR_HEAD XPLNAME XPL_HDR_TAIL CONFIGCURRENT_MESSAGE;

struct persistentconfig{
    uint16_t motionsens;
    uint16_t motiontime;
    char instanceid[32];
} ;


void initPWM(void);
void setPWM ( uint8_t val );

