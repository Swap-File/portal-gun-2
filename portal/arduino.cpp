#include "cobs.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <wiringSerial.h>
#include "arduino.h"


//serial com data
#define INCOMING_BUFFER_SIZE 128
uint8_t incoming_buffer[INCOMING_BUFFER_SIZE];
uint8_t incoming_index = 0;
uint8_t incoming_decoded_buffer[INCOMING_BUFFER_SIZE];
int framing_error = 0;
int crc_error =0;

int fd;

void arduino_update(void){

int pwm1 = 127;
		int pwm2 = 127;
		
		SerialUpdate(fd);
		
		
		uint8_t raw_buffer[3];

		raw_buffer[0] = pwm1;
		raw_buffer[1] = pwm2;
		raw_buffer[2] = crc8(raw_buffer, 2);

		uint8_t encoded_buffer[6];
		uint8_t encoded_size = COBSencode(raw_buffer, 3, encoded_buffer);
		
		encoded_buffer[encoded_size]=0x00;
		write(fd,encoded_buffer,encoded_size+1);
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

uint8_t crc8(const uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;
	while (len--) crc = dscrc_table[(crc ^ *addr++)];
	return crc;
}
 
void onPacket(const uint8_t* buffer, uint8_t size)
{
	//check for framing errors
	if (size != 29){
		framing_error++;
	}
	else{
		//check for crc errors
		uint8_t crc = crc8(buffer, size - 1);
		if (crc != buffer[size - 1]){
			crc_error++;
			printf("bad \n");
		}
		else{
			
		int16_t yaw = buffer[0] << 8 | buffer[1];
		int16_t pitch = buffer[2] << 8 | buffer[3];
		int16_t roll = buffer[4] << 8 | buffer[5];
		
		int16_t aaRealx = buffer[6] << 8 | buffer[7]; 
		int16_t aaRealy = buffer[8] << 8 | buffer[9]; 
		int16_t aaRealz = buffer[10] << 8 | buffer[11]; 
		
	
		int16_t aaWorldx = buffer[12] << 8 | buffer[13]; 
		int16_t aaWorldy = buffer[14] << 8 | buffer[15]; 
		int16_t aaWorldz = buffer[16] << 8 | buffer[17]; 
		

		uint8_t inputs = buffer[18];
		
		uint8_t cpuload = buffer[19];
		int16_t temp = buffer[20] << 8 | buffer[21]; 
		int16_t battery = buffer[26] << 8 | buffer[27]; 

	
		
		uint8_t packets_in_per_second = buffer[22];
		uint8_t packets_out_per_second = buffer[23];
		uint8_t framing_error = buffer[24];
		uint8_t crc_error = buffer[25];
		
			//printf("ypr %d, %d, %d\n", yaw,pitch,roll);	
		
			//printf("areal %d, %d, %d\n", aaRealx,aaRealy,aaRealz);
	
				//printf("aaWorld %d, %d, %d\n", aaWorldx,aaWorldy,aaWorldz);
		
		//printf("Temp: %d  Load: %d  Fingers: %d battery: %d\n",temp ,cpuload ,inputs,battery);
		
		//printf("PPSIN: %d  PPSOUT: %d  FRAMING: %d CRC: %d\n",packets_in_per_second ,packets_out_per_second,framing_error ,crc_error);
		
		}
	}
}


void SerialUpdate(int fd){
	while (serialDataAvail(fd)){

		//read in a byte
		incoming_buffer[incoming_index] = serialGetchar(fd);

		//check for end of packet
		if (incoming_buffer[incoming_index] == 0x00){

			//try to decode
			uint8_t decoded_length = COBSdecode(incoming_buffer, incoming_index, incoming_decoded_buffer);

			//check length of decoded data (cleans up a series of 0x00 bytes)
			if (decoded_length > 0)	onPacket(incoming_decoded_buffer, decoded_length);

			//reset index
			incoming_index = 0;
		}
		else{
			//read data in until we hit overflow, then hold at last position
			incoming_index++;
			if (incoming_index == INCOMING_BUFFER_SIZE) incoming_index = 0;
		}
	}
}
