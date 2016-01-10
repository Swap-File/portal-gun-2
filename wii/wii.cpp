#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <errno.h>
#include <stdint.h>

uint32_t sampletime = 0;
int error = 0;
int missed_cycle = 0;
uint32_t fps_counter = 0;
int fps = 0;

int joyX = 0;
int joyY = 0;
int accelX = 0;
int accelY = 0;
int accelZ = 0;
int c = 0;
int z = 0;
		
void reinit(int fd){
	wiringPiI2CWriteReg8(fd, 0xF0 , 0x55 );
	wiringPiI2CWriteReg8(fd, 0xFB , 0x00);
	delayMicroseconds(500);
}

int main(int argc, char *argv[]) {
	
	
	fprintf(stderr, "Testing the nunchuck through I2C\n");
    wiringPiSetup();
    int fd = wiringPiI2CSetup(0x52);
    if (fd < 0) {
		fprintf(stderr, "Error setting up I2C: %d\n", errno);
        exit(0);
    }
	
	reinit(fd);
	
    int bytes[7];
    int i;
    while(1) {
		
		sampletime += 20;
		if (sampletime < millis()){
			sampletime = millis();
			//printf("Missed cycle(s), Skipping...\n");
			fprintf(stderr, "Wii Missed cycle(s), Skipping...\n");
			missed_cycle++;
		}else{
			delay(sampletime - millis()); //predictive delay
		}
		
		fps++;
		if (fps_counter < millis()){
			fprintf(stderr, "I2C FPS:%d  err:%d  mis:%d\n",fps,error,missed_cycle);
			fps = 0;
			fps_counter += 1000;
			if (fps_counter < millis()){
			fps_counter = millis()+1000;
			}			
		}
		
		int disconnected = 0;
		
		wiringPiI2CWrite(fd, 0x00);
		delayMicroseconds(500);
		for (i=0; i<7; i++) {
			bytes[i] = wiringPiI2CRead(fd);
			if (bytes[i] == 0xff) disconnected++;
				
		}
	
		if (disconnected == 7) reinit(fd);
		
		if (disconnected < 7 && bytes[6] ==0x00){
		 joyX = bytes[0];
		 joyY = bytes[1];
		 accelX = (bytes[2] << 2) | ((bytes[5] & 0xc0) >> 6);
		 accelY = (bytes[3] << 2) | ((bytes[5] & 0x30) >> 4);
		 accelZ = (bytes[4] << 2) | ((bytes[5] & 0x0c) >> 2);
		 c = (bytes[5] & 0x02) >> 1;
		 z = bytes[5] & 0x01;
		 disconnected = 0;
		}else {
			disconnected = 1;
			error++;
		}
			
		//printf("data: err=%d joyX=%d joyY=%d accelX=%d accelY=%d accelZ=%d c=%d z=%d\n", error, joyX, joyY, accelX, accelY, accelZ, c, z);
		printf("%d %d %d %d %d %d %d %d\n", joyX, joyY, accelX, accelY, accelZ, c, z, disconnected);
		fflush(stdout);
		
	}

    return 0;
}