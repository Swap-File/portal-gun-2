#ifndef _NUNCHUK_H
#define _NUNCHUK_H
#include <stdint.h>
void arduino_setup(struct arduino_struct *arduino);
int arduino_update(const struct this_gun_struct& this_gun);
uint8_t crc8(const uint8_t* addr, uint8_t len);
int SerialUpdate(int fd);
int onPacket(const uint8_t* buffer, uint8_t size);

static const uint8_t dscrc_table[] = {
      0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
    157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
     35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
    190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
     70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
    219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
    101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
    248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
    140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
     17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
    175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
     50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
    202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
     87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
    233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
    116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};



struct arduino_struct {
	
	bool first_cycle = true; //if true, preload the filters with data
	
	int16_t yaw = 0;
	int16_t pitch = 0;
	int16_t roll = 0;
	
	int16_t aaRealx = 0; 
	int16_t aaRealy = 0; 
	int16_t aaRealz = 0; 
	
	int16_t aaWorldx = 0; 
	int16_t aaWorldy = 0; 
	int16_t aaWorldz = 0; 
	
	uint32_t connected_last = 0;
	uint32_t connected = 0;
	
	bool orange_button_previous = false;
	bool blue_button_previous = false;
	
	bool orange_button = false;
	bool blue_button = false;
	
	uint32_t OrangePressTime = 0;
	uint32_t BluePressTime = 0;
	
	uint8_t cpuload = 0; //0 to 100
	
	uint16_t temp = 0;
	float temperature_pretty = 0;

	uint16_t battery[256]; //array of samples
	uint8_t battery_index = 0; //this rolls over
	uint32_t battery_total = 0; //sum of the entire battery array
	uint16_t battery_level = 0; //the last calculated average level in ADC format
	float battery_level_pretty = 0; //battery level in volts
	
	uint8_t packets_in_per_second = 0;
	uint8_t packets_out_per_second = 0;
	uint8_t framing_error = 0;
	uint8_t crc_error = 0;
	uint8_t packet_counter = 0;
	
	bool supress_blue = false;
	bool supress_orange = false;
};  

#endif