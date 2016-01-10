#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>


int joyX = 0;
int joyY = 0;
int accelX = 0;
int accelY = 0;
int accelZ = 0;
int c = 0;
int z = 0;
int connected = 0;
FILE * wiifp;

void wiicontrol_setup(void){	
	printf("WII_CONTROL: SPAWNING\n");
	wiifp = popen("sudo /home/pi/wii/wii", "r");
	fcntl(fileno(wiifp), F_SETFL, fcntl(fileno(wiifp), F_GETFL, 0) | O_NONBLOCK);
	printf("WII_CONTROL: READY\n");
}

void wiicontrol_update(void){
	
	int count = 1;
	char buffer[100];
	//stdin is line buffered so we can cheat a little bit
	while (count > 0){ // dump entire buffer
		count = read(fileno(wiifp), buffer, sizeof(buffer)-1);
		if (count > 1){ //ignore blank lines
			buffer[count-1] = '\0';
			//keep most recent line
			int temp[8];
			int result = sscanf(buffer,"%d %d %d %d %d %d %d %d", &temp[0],&temp[1],&temp[2],&temp[3],&temp[4],&temp[5],&temp[6],&temp[7]);

			if (result != 8){
				fprintf(stderr, "WII_Control: Unrecognized input with %d items.\n", result);
			}else{
				joyX = temp[0];
				joyY = temp[1];
				accelX = temp[2];
				accelY = temp[3];
				accelZ = temp[4];
				connected =  temp[7];
				c = temp[5];				
				z = temp[6];
				break;
			}
		}
	}
};

int wiicontrol_c(void){
	
	return c;
}

