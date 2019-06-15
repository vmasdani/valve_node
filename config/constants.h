#ifndef _VALVE_IOT_CONSTANTS_
#define _VALVE_IOT_CONSTANTS_

// #define _host "broker.hivemq.com"
#define _host "localhost"
#define _port "1883"
#define _topic "control"
#define _topic_sched "schedule"
#define _topic_poweroff "poweroff"
#define _relay_pin 7 // 7 in wiringPi, 6 in orange pi.
#define _relay_on 0 // LOW logic triggers the relay on
#define _relay_off 1 // HIGH logic triggers the relay off

#endif
