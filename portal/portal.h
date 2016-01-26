#ifndef _PORTAL_H
#define _PORTAL_H

#include <stdint.h>

struct this_gun_struct {
	uint8_t brightness = 0;
	int shared_state = 0;  //state reported to other gun
	int shared_state_previous = 0;
	int private_state = 0; //internal state for single player modes
	int private_state_previous = 0;
	bool initiator = false; //Did this gun start the connection request?
	uint32_t clock = 0;
};  

struct other_gun_struct {
	int state = 0; //state read from other gun
	int state_previous = 0;
	bool connected = false; 
	uint32_t last_seen = 0;
	uint32_t clock = 0;
};  

#define GUN_EXPIRE 1000 //expire a gun in 1 second
#define LONG_PRESS_TIME  500
#define BUTTON_ACK_BLINK 100

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

void INThandler(int dummy);

#endif