#include "ledcontrol.h"
#include "udpcontrol.h"
#include "pipecontrol.h"
#include "wiicontrol.h"
 
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <errno.h>
#include <signal.h>

#define EFFECT_RESOLUTION 400
#define BREATHING_RATE 2000

#define STATION_EXPIRE 1000 //expire a station in 1 second

void INThandler(int dummy) {
	printf("\nCleaning up...\n");
	//ahrs_quit();
	pipecontrol_cleanup();
	
	exit(1);
}


int main(void){
	

	int other_gun_state;
	uint32_t other_gun_clock_offset;
	int other_gun_last_seen = 0;
	int other_gun_connected = 0;

	
	signal(SIGINT, INThandler);
	
	//stats
	uint32_t sampletime = 0;
	int missed = 0;
	uint32_t fps_counter = 0;
	int fps = 0;
	uint32_t udp_send_time = 0;
	
	pipecontrol_setup();
	
	ledcontrol_setup();
	udpcontrol_setup();
	wiicontrol_setup();
	int gst_backend = 0;
	
	while(1){
		
		
		sampletime += 10;
		if (sampletime < millis()){
			sampletime = millis();
			printf("Missed cycle(s), Skipping...\n");
			missed++;
			
		}else{
			delay(sampletime - millis()); //predictive delay
		}

		wiicontrol_update();
		
		if (wiicontrol_c()) gst_backend = 2;
		else gst_backend = 1;
		
		gst_command(gst_backend);	

		uint32_t time_this_cycle = millis();
			
		//read other gun's data
		while (1){
			int result = udp_receive_state(&other_gun_state,&other_gun_clock_offset);
			if (result <= 0) break;  //read until buffer empty
			else other_gun_last_seen = time_this_cycle;  //update time data was seen
			if (millis() - time_this_cycle > 5) break; //flood protect
		}
		
		//check for expiration of other gun
		if (time_this_cycle - other_gun_last_seen > STATION_EXPIRE) {
			if (other_gun_connected != 0){
			other_gun_connected = 0;
			printf("Gun Expired\n");	
			}
		}
		else if (other_gun_connected == 0){
			other_gun_connected = 1;
			printf("Gun Connected %d\n",other_gun_state);
		}
		
		

		
		const int effect_resolution = 400;
		const int breathing_rate = 2000;
		int total_time_offset = 0;
		
		if (other_gun_connected) {
		total_time_offset=  int((float)(((time_this_cycle + other_gun_clock_offset) / (2))% (breathing_rate)) * ((float)effect_resolution)/((float)(breathing_rate)));
		}else{
		total_time_offset=  int((float)((time_this_cycle)% (breathing_rate)) * ((float)effect_resolution)/((float)(breathing_rate)));
		}
	
		int local_state = 0;
		
		//float buttonbrightness = ledcontrol_update(color1,width_request,width_speed,shutdown_effect, total_time_offset);
		//ahrs_update(2,2,0);
		ledcontrol_update(20,20,200,0,total_time_offset);
		
		
		//send data to other gun
		if (time_this_cycle - udp_send_time > 100){
			udp_send_state(&local_state,&time_this_cycle);
			udp_send_time = time_this_cycle;
		}
		
		//fps counter code
		fps++;
		if (fps_counter < millis()){
			printf("SPI FPS:%d \n",fps);
			fps = 0;
			fps_counter += 1000;
			if (fps_counter < millis()){
				fps_counter = millis()+1000;
			}			
		}	
	}
	return 0;
}
