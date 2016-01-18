#ifndef _PORTAL_H
#define _PORTAL_H

#include "ledcontrol.h"
#include "udpcontrol.h"
#include "pipecontrol.h"
#include "wiicontrol.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/time.h>  
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define EFFECT_RESOLUTION 400
#define BREATHING_RATE 2000

#define STATION_EXPIRE 1000 //expire a station in 1 second

#define BUTTON_BOTH_LONG_BLUE 0
#define BUTTON_BOTH_LONG_ORANGE 1
#define BUTTON_ORANGE_LONG 2
#define BUTTON_ORANGE_SHORT 3
#define BUTTON_BLUE_SHORT 4
#define BUTTON_BLUE_LONG 5
#define BUTTON_NONE 6

#define WEB_ORANGE_WIFI 100
#define WEB_BLUE_WIFI 104
#define WEB_BLUE_SELF 103
#define WEB_ORANGE_SELF 101
#define WEB_CLOSE 102

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int get_ip(void);
void INThandler(int dummy);
void local_state_engine(int button);
void network_state_engine(int button);
void led_update(int time_this_cycle, uint32_t other_gun_clock_offset);
#endif