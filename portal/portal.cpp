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
	const uint32_t video_length[12] = {24467,16767,12067,82000,30434,22900,19067,70000,94000,53167,184000,140000} ;
	uint32_t video_start_time =0 ;
	uint32_t active_video_lentgh = 0;
	int gst_backend = 0;
	
	struct other_gun_struct other_gun;
	struct this_gun_struct this_gun;
	
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
	
	int ip = udpcontrol_setup();
	pipecontrol_setup(ip);
	ledcontrol_setup();
	arduino_setup(&arduino);
	

	int next_effect = 0;
	bool freq_division = true; //toggles every other cycle
	
	while(1){
		sampletime += 10;
		if (sampletime < millis()){
			sampletime = millis();
			//printf("Missed cycle(s), Skipping...\n");
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
		button_event = BUTTON_NONE;
		
		if (button_event == BUTTON_NONE){
			button_event = read_web_pipe(this_gun);
		}

		//reset playlists on reset
		if (button_event == BUTTON_BLUE_LONG || button_event == BUTTON_ORANGE_LONG){
			this_gun.shared_playlist_index = 0;
			this_gun.private_playlist_index = 0;			
		}
		
		//read other gun's data, only if no button events are happening this cycle
		while (button_event == BUTTON_NONE){
			int result = udp_receive_state(&other_gun.state,&other_gun.clock);
			if (result <= 0) break;  //read until buffer empty
			else other_gun.last_seen = this_gun.clock;  //update time data was seen
			if (millis() - this_gun.clock > 5) break; //flood protect
		}

		//check for expiration of other gun
		if (this_gun.clock - other_gun.last_seen > GUN_EXPIRE) {
			if (this_gun.connected != false){
				this_gun.connected = false;
				printf("\nGun Expired\n");	
			}
			other_gun.state = 0;
		}
		else {
			if(this_gun.connected == false){
				this_gun.connected = true;
				printf("\nGun Connected\n");
			}
		}
		
		//process state changes
		local_state_engine(button_event,this_gun,other_gun);
		
		//shared state stuff
		
		//figure out when to optimally increment indexes
		
		//start camera (preload in state 3, shutter is closed)
		if(this_gun.shared_state <= -2) gst_backend = GST_RPICAMSRC;
		
		//cache next effect
		if(this_gun.shared_state == 2 && this_gun.shared_state_previous == 1) next_effect = this_gun.shared_playlist[this_gun.shared_playlist_index];
	
		//preload live projector feed in state 3, shutter is closed, also change to next effect in state 5 entry
		if(this_gun.shared_state >= 2) gst_backend = next_effect;
		
		if (this_gun.shared_state == 5 && this_gun.shared_state_previous == 4){    
			if (this_gun.shared_playlist[++this_gun.shared_playlist_index] < 0) this_gun.shared_playlist_index = 0;
		}
					
		//private state stuff
		
		//preload next effect and start in background if its video
		if ((this_gun.private_state_previous != -3 && this_gun.private_state == -3) || (this_gun.private_state_previous != 3  && this_gun.private_state == 3) ){
			//movies
			if (gst_backend >= GST_MOVIE_FIRST && gst_backend <= GST_MOVIE_LAST){
				gst_backend = GST_BLANK;
			}
			next_effect = this_gun.private_playlist[this_gun.private_playlist_index++];
			if (this_gun.private_playlist[this_gun.private_playlist_index] < 0) this_gun.private_playlist_index = 0;
			
			if (next_effect >= GST_LIBVISUAL_FIRST && next_effect < GST_LIBVISUAL_LAST){
				gst_backend = next_effect;
				printf("\n\nPreloading vis\n");
			}else{
				gst_backend = GST_BLANK;
				printf("\n\nNot preloading movie\n");
			}
		}
		
		//start new video
		if ((this_gun.private_state_previous != -4 && this_gun.private_state == -4) || (this_gun.private_state_previous != 4  && this_gun.private_state == 4) ){
			
			if( this_gun.private_state == 4)	   this_gun.private_state=5;
			else if( this_gun.private_state == -4) this_gun.private_state=-5;
			
			printf("\n\n Starting Video or viz\n");
			gst_backend = next_effect; //run video
			if (gst_backend >= GST_MOVIE_FIRST && gst_backend <= GST_MOVIE_LAST){
				video_start_time = this_gun.clock;
				active_video_lentgh = video_length[gst_backend - GST_MOVIE_FIRST];			
			}
		}
		
		//video auto close if video ends
		if (gst_backend >= 50){
			if ( this_gun.clock - video_start_time >  active_video_lentgh + 1500){
				if (this_gun.private_state == 5)       this_gun.private_state = 3;
				else if (this_gun.private_state == -5) this_gun.private_state = -3;
				gst_backend = GST_BLANK;
			}
		}
		
		//video kill switch
		//master clear
		if(this_gun.shared_state < 3 && this_gun.shared_state > -3 && \
			this_gun.private_state < 3 && this_gun.private_state > -3) gst_backend = GST_BLANK;
		
		//ahrs effects
		int ahrs_number = AHRS_CLOSED; 
		// for networked modes
		if (this_gun.private_state == 0){
			if (this_gun.shared_state == 3) ahrs_number = AHRS_CLOSED_ORANGE;
			else if (this_gun.shared_state == 4) ahrs_number = AHRS_OPEN_ORANGE;		
			else if (this_gun.shared_state == 5) ahrs_number = AHRS_CLOSED_ORANGE; //blink shut on effect change
		}
		// for self modes
		if (this_gun.shared_state == 0){
			if (this_gun.private_state == 3 || this_gun.private_state == 4)	ahrs_number = AHRS_CLOSED_ORANGE;
			else if (this_gun.private_state == -3 || this_gun.private_state == -4) ahrs_number = AHRS_CLOSED_BLUE;
			else if (this_gun.private_state < -4) ahrs_number = AHRS_OPEN_BLUE;
			else if (this_gun.private_state > 4) ahrs_number = AHRS_OPEN_ORANGE;
		}
		
		ahrs_command(2,2,2,ahrs_number);
		
		audio_effects(this_gun);
				
		gst_command(gst_backend);	
		
		//hic svnt dracones
		if(freq_division) this_gun.brightness = led_update(this_gun,other_gun);

		//send data to other gun
		if (this_gun.clock - udp_send_time > 100){
			udp_send_state(this_gun.shared_state,this_gun.clock);
			udp_send_time = this_gun.clock;
			web_output(this_gun,arduino,gst_backend);
		}
		
		//fps counter code
		fps++;
		if (fps_counter < millis()){		
			printf("SPI FPS:%d missed: %d\n",fps,missed);
			fps = 0;
			fps_counter += 1000;
			
			if (fps_counter < millis()){
				fps_counter = millis() + 1000;
			}			
		}	
	}
	return 0;
}