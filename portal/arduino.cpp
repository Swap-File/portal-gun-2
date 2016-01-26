#include "cobs.h"
#include "portal.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <wiringSerial.h>
#include <wiringPi.h>
#include "arduino.h"

struct arduino_struct {
	
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
	
	uint8_t cpuload = 0;
	
	float temp = 0.0;
	float battery = 0.0;

	uint8_t packets_in_per_second = 0;
	uint8_t packets_out_per_second = 0;
	uint8_t framing_error = 0;
	uint8_t crc_error = 0;
	uint8_t packet_counter = 0;
	
	bool supress_blue = false;
	bool supress_orange = false;
};  

struct arduino_struct arduino;
//serial com data
#define INCOMING_BUFFER_SIZE 128
uint8_t incoming_buffer[INCOMING_BUFFER_SIZE];
uint8_t incoming_index = 0;
uint8_t incoming_decoded_buffer[INCOMING_BUFFER_SIZE];

int framing_error = 0;
int crc_error =0;

int fd;


int arduino_update(uint8_t brightness, bool connected,uint8_t ir_pwm){
	
	//default lights
	uint8_t orange_pwm = (connected) ? brightness : 127;
	uint8_t blue_pwm = (connected) ? brightness : 127;
	
	//blink orange momentarily if pressed
	if (arduino.orange_button){
		if ((millis() - arduino.OrangePressTime < BUTTON_ACK_BLINK) || ((millis() - arduino.OrangePressTime < (BUTTON_ACK_BLINK + LONG_PRESS_TIME)) && (millis() - arduino.OrangePressTime > LONG_PRESS_TIME))){
			orange_pwm = 0; 
		}
		blue_pwm = 0;
	}
	
	//blink blue momentarily if pressed
	if (arduino.blue_button){
		if ((millis() - arduino.BluePressTime < BUTTON_ACK_BLINK) || ((millis() - arduino.BluePressTime < (BUTTON_ACK_BLINK + LONG_PRESS_TIME)) && (millis() - arduino.BluePressTime > LONG_PRESS_TIME))){
			blue_pwm = 0; 
		}		
		orange_pwm = 0;
	}
	
	//don't blink second button for double presses
	if (arduino.blue_button && arduino.orange_button){
		if (arduino.BluePressTime < arduino.OrangePressTime){
			orange_pwm = (connected) ? brightness : 127;
		}else{
			blue_pwm = (connected) ? brightness : 127;
		}
	}
	
	uint8_t raw_buffer[4];
	raw_buffer[0] = blue_pwm;
	raw_buffer[1] = orange_pwm;
	raw_buffer[2] = ir_pwm;
	raw_buffer[3] = crc8(raw_buffer, 3);

	uint8_t encoded_buffer[6];
	uint8_t encoded_size = COBSencode(raw_buffer, 4, encoded_buffer);
	
	encoded_buffer[encoded_size]=0x00;
	write(fd,encoded_buffer,encoded_size+1);
	
	
	return SerialUpdate(fd);
}

int onPacket(const uint8_t* buffer, uint8_t size)
{
	//check for framing errors
	if (size != 30){
		framing_error++;
	}
	else{
		//check for crc errors
		uint8_t crc = crc8(buffer, size - 1);
		if (crc != buffer[size - 1]){
			crc_error++;
		}
		else{
			arduino.orange_button_previous = arduino.orange_button;
			arduino.blue_button_previous = arduino.blue_button;
			
			arduino.yaw = buffer[0] << 8 | buffer[1];
			arduino.pitch = buffer[2] << 8 | buffer[3];
			arduino.roll = buffer[4] << 8 | buffer[5];
			
			arduino.aaRealx = buffer[6] << 8 | buffer[7]; 
			arduino.aaRealy = buffer[8] << 8 | buffer[9]; 
			arduino.aaRealz = buffer[10] << 8 | buffer[11]; 
			
			
			arduino.aaWorldx = buffer[12] << 8 | buffer[13]; 
			arduino.aaWorldy = buffer[14] << 8 | buffer[15]; 
			arduino.aaWorldz = buffer[16] << 8 | buffer[17]; 
			

			if ((buffer[18] & 0x01) == 0x01) arduino.blue_button = false;
			else {
				arduino.blue_button = true;	
				if (!arduino.blue_button_previous) arduino.BluePressTime = millis();
			} 
			
			if ((buffer[18] & 0x02) == 0x02) arduino.orange_button = false;
			else{
				arduino.orange_button = true;
				if (!arduino.orange_button_previous) arduino.OrangePressTime = millis();					
			}
			
			
			arduino.cpuload = buffer[19];
			
			int16_t temp_temp = buffer[20] << 8 | buffer[21]; 
			arduino.temp = temp_temp;
			
			int16_t temp_battery = buffer[26] << 8 | buffer[27]; 
			arduino.battery = temp_battery;
			
			arduino.packets_in_per_second = buffer[22];
			arduino.packets_out_per_second = buffer[23];
			arduino.framing_error = buffer[24];
			arduino.crc_error = buffer[25];
			arduino.packet_counter = buffer[29];
			//printf("ypr %d, %d, %d\n", yaw,pitch,roll);	
			
			//printf("areal %d, %d, %d\n", aaRealx,aaRealy,aaRealz);
			
			//printf("aaWorld %d, %d, %d\n", aaWorldx,aaWorldy,aaWorldz);
			
			//printf("Temp: %d  Load: %d  Fingers: %d battery: %d\n",temp ,cpuload ,inputs,battery);
			
			//printf("PPSIN: %d  PPSOUT: %d  FRAMING: %d CRC: %d\n",packets_in_per_second ,packets_out_per_second,framing_error ,crc_error);
			
			
			//scan for button presses
			
			if (!arduino.blue_button && arduino.blue_button_previous){
				if (arduino.supress_blue) arduino.supress_blue = false;
				else {
					if (!arduino.blue_button && arduino.blue_button_previous && arduino.orange_button){
						arduino.supress_orange = true;
						return BUTTON_BOTH_LONG_ORANGE;
					}
					if (millis() - arduino.BluePressTime < LONG_PRESS_TIME)	return BUTTON_BLUE_SHORT;
					else return BUTTON_BLUE_LONG;
				}
			}
			
			if (!arduino.orange_button && arduino.orange_button_previous){
				if (arduino.supress_orange) arduino.supress_orange = false;
				else {
					if (arduino.blue_button){
						arduino.supress_blue = true;
						return BUTTON_BOTH_LONG_BLUE;
					}
					if (millis() - arduino.OrangePressTime < LONG_PRESS_TIME) return BUTTON_ORANGE_SHORT;
					else return BUTTON_ORANGE_LONG;
				}
			}
			return BUTTON_NONE;
		}
	}
	return BUTTON_NONE;
}

void arduino_setup(void){	
	char device[] = "/dev/ttyAMA0";
	fd = serialOpen(device,115200);
	if (fd < 0) {
		printf("Error setting up serial: %d\n", errno);
		exit(0);
	}
	serialFlush(fd);
}

int SerialUpdate(int fd){
	int return_value = BUTTON_NONE;
	while (serialDataAvail(fd)){

		//read in a byte
		incoming_buffer[incoming_index] = serialGetchar(fd);

		//check for end of packet
		if (incoming_buffer[incoming_index] == 0x00){

			//try to decode
			uint8_t decoded_length = COBSdecode(incoming_buffer, incoming_index, incoming_decoded_buffer);

			//check length of decoded data (cleans up a series of 0x00 bytes)
			if (decoded_length > 0)	return_value = onPacket(incoming_decoded_buffer, decoded_length);

			//reset index
			incoming_index = 0;
		}
		else{
			//read data in until we hit overflow, then hold at last position
			incoming_index++;
			if (incoming_index == INCOMING_BUFFER_SIZE) incoming_index = 0;
		}
	}
	return return_value;
}

uint8_t crc8(const uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;
	while (len--) crc = dscrc_table[(crc ^ *addr++)];
	return crc;
}