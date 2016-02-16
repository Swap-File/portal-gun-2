#include "portal.h"
#include "gstvideo/gstvideo.h"
#include "ledcontrol.h"
#include "udpcontrol.h"
#include "pipecontrol.h"
#include "arduino.h"
#include "statemachine.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <wiringPi.h>

void INThandler(int dummy) {
	printf("\nCleaning up...\n");
	ledcontrol_wipe();
	pipecontrol_cleanup();
	exit(1);
}

int main(void){
	//magic numbers of the video lengths in milliseconds
	const uint32_t video_length[12] = {24467,16767,12067,82000,30434,22900,19067,70000,94000,53167,184000,140000};
	uint32_t video_end_time = 0;
	
	struct this_gun_struct this_gun;
	struct other_gun_struct other_gun;
	struct arduino_struct arduino;
	
	//catch broken pipes to respawn threads if they crash
	signal(SIGPIPE, SIG_IGN);
	//catch ctrl+c when exiting
	signal(SIGINT, INThandler);
	
	//stats
	uint32_t sampletime = 0;
	int missed = 0;
	uint32_t fps_counter = 0;
	int fps = 0;
	uint32_t udp_send_time = 0;
	
	//setup libaries
	int ip = udpcontrol_setup();
	pipecontrol_setup(ip);
	ledcontrol_setup();
	arduino_setup(&arduino);
	
	bool freq_division = true; //toggles every other cycle
	
	while(1){
		sampletime += 10;
		if (sampletime < millis()){
			sampletime = millis();
			printf("MAIN has missed cycle(s), Skipping...\n");
			missed++;
		}else{
			delay(sampletime - millis()); //predictive delay
		}
		
		this_gun.clock = millis();  //stop time for duration of frame
		freq_division = !freq_division;
		this_gun.shared_state_previous = this_gun.shared_state;
		this_gun.private_state_previous = this_gun.private_state;
		other_gun.state_previous = other_gun.state;
		
		int button_event = arduino_update(this_gun);
		
		if (button_event == BUTTON_NONE) button_event = read_web_pipe(this_gun);
	
		//read other gun's data, only if no button events are happening this cycle
		while (button_event == BUTTON_NONE){
			int result = udp_receive_state(&other_gun.state,&other_gun.clock);
			if (result <= 0) break;  //read until buffer empty
			else other_gun.last_seen = this_gun.clock;  //update time data was seen
			if (millis() - this_gun.clock > 5) break; //flood protect
		}
		
		//process state changes
		local_state_engine(button_event,this_gun,other_gun);
				
		//gstreamer state stuff, blank it if shared state and private state are 0
		int gst_state = GST_BLANK;
		//camera preload
		if(this_gun.shared_state <= -1) gst_state = GST_RPICAMSRC;
		//project shared preload
		else if(this_gun.shared_state >= 1) gst_state = this_gun.shared_effect;		
		//project private preload
		else if(this_gun.private_state != 0) gst_state = this_gun.private_effect;	
				
		//special handling of videos
		if (gst_state >= GST_MOVIE_FIRST && gst_state <= GST_MOVIE_LAST){
			//calculate video end time at video start
			if ((this_gun.private_state == 4 && this_gun.private_state_previous != 4) ||\
    			(this_gun.private_state == -4 && this_gun.private_state_previous != -4)){
				video_end_time = this_gun.clock + video_length[gst_state - GST_MOVIE_FIRST];		
			}
			//auto state change at video ending
			if (this_gun.clock + 1000 > video_end_time){
				if (this_gun.private_state == 4)       this_gun.private_state = 3;
				else if (this_gun.private_state == -4) this_gun.private_state = -3;
			}
			//block preloading of video
			if (this_gun.private_state != 4 && this_gun.private_state != -4) gst_state = GST_BLANK;
		}
		
		//ahrs effects
		int ahrs_state = AHRS_CLOSED; 
		// for networked modes
		if (this_gun.private_state == 0){
			if (this_gun.shared_state == 3) ahrs_state = AHRS_CLOSED_ORANGE;
			else if (this_gun.shared_state == 4) ahrs_state = AHRS_OPEN_ORANGE;		
			else if (this_gun.shared_state == 5) ahrs_state = AHRS_CLOSED_ORANGE; //blink shut on effect change
		}
		// for self modes
		if (this_gun.shared_state == 0){
			if (this_gun.private_state == 3) ahrs_state = AHRS_CLOSED_ORANGE;
			else if (this_gun.private_state == -3) ahrs_state = AHRS_CLOSED_BLUE;
			else if (this_gun.private_state <= -4) ahrs_state = AHRS_OPEN_BLUE;
			else if (this_gun.private_state >= 4) ahrs_state = AHRS_OPEN_ORANGE;
		}
		
		ahrs_command(2,2,2,ahrs_state);
		audio_effects(this_gun);
		gst_command(gst_state);	
		
		//hic svnt dracones
		if(freq_division) this_gun.brightness = led_update(this_gun,other_gun);

		//send data to other gun
		if (this_gun.clock - udp_send_time > 100){
			udp_send_state(this_gun.shared_state,this_gun.clock);
			udp_send_time = this_gun.clock;
			web_output(this_gun,arduino);
		}
		
		//fps counter code
		fps++;
		if (fps_counter < millis()){		
			printf("MAIN FPS:%d missed: %d\n",fps,missed);
			fps = 0;
			fps_counter += 1000;
			//readjust counter if we missed a cycle
			if (fps_counter < millis())	fps_counter = millis() + 1000;
		}	
	}
	return 0;
}