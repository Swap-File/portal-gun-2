#include "portal.h"

int self_state_previous = 0;
int local_state_previous = 0;
int remote_state_previous = 0;
int remote_state = 0;
int self_state = 0;
int local_state = 0;
int connection_initiator = 0;
int other_gun_connected = 0;
int video_playing = 0;

const uint32_t video_length[12] = {24467,16767,12067,82000,30434,22900,19067,70000,94000,53167,184000,140000} ;
uint32_t video_start_time =0 ;
uint32_t active_video_lentgh = 0;

int self_playlist[10]={10,50,13,51,14,51,15,-1};
int self_playlist_index = 0;

int local_playlist[10]={4,20,29,-1};
int local_playlist_index = 0;

void INThandler(int dummy) {
	printf("\nCleaning up...\n");
	pipecontrol_cleanup();
	exit(1);
}

int get_ip(void){
	char my_ip[16];
	int fd;
	struct ifreq ifr;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;
	/* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);
	/* display result */
	sprintf(my_ip,"%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	if (strstr(my_ip,"192.168.1.22"))		return 22;
	else if (strstr(my_ip,"192.168.1.23"))	return 23;
	
	printf("Unknown IP\n");
	exit(1);
	return 0;
}

int main(void){
	int ip = get_ip();
	
	
	
	int other_gun_last_seen = 0;

	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, INThandler);
	
	//stats
	uint32_t sampletime = 0;
	int missed = 0;
	uint32_t fps_counter = 0;
	int fps = 0;
	uint32_t udp_send_time = 0;
	
	pipecontrol_setup(ip);
	ledcontrol_setup();
	udpcontrol_setup(ip);
	wiicontrol_setup();
	int gst_backend = 0;
	int next_effect = 50;
	bool every_other_cycle = true;
	
	while(1){
		
		int button_pressed = BUTTON_NONE;
		
		local_state_previous = local_state;
		self_state_previous = self_state;
		remote_state_previous = remote_state;
		
		sampletime += 10;
		if (sampletime < millis()){
			sampletime = millis();
			//printf("Missed cycle(s), Skipping...\n");
			missed++;
			
		}else{
			delay(sampletime - millis()); //predictive delay
		}
		uint32_t time_this_cycle = millis();
		
		wiicontrol_update();
		
		int result = read_web_pipe();
		if (result != -1){
			printf("PIPE INCOMING! %d",result);
			switch (result){
			case WEB_ORANGE_WIFI:	button_pressed = BUTTON_ORANGE_SHORT;  break;
			case WEB_BLUE_WIFI:		button_pressed = BUTTON_BLUE_SHORT;    break;
			case WEB_BLUE_SELF:		button_pressed = BUTTON_BOTH_LONG_BLUE; break; 
			case WEB_ORANGE_SELF: 	button_pressed = BUTTON_BOTH_LONG_ORANGE; break; 
			case WEB_CLOSE: 		button_pressed = BUTTON_BLUE_LONG; break; 
			default: 
				gst_backend = result;
			}
		}

		//read other gun's data
		int temp_state;
		uint32_t other_gun_clock_offset = 0;
		while (1){
			int result = udp_receive_state(&temp_state,&other_gun_clock_offset);
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
			remote_state = 0;
		}
		else {
			if(other_gun_connected == 0){
			other_gun_connected = 1;
			printf("Gun Connected \n");
			}
			remote_state = temp_state;
		}
		
		
		//process one state change per cycle only!
		local_state_engine(button_pressed);
		if (button_pressed == BUTTON_NONE) network_state_engine(remote_state);

		
		
		
		//start camera if in the right mode for it.
		if(local_state <= -3) gst_backend = 3;
			
		//start camera if in the right mode for it.
		if(local_state >= 3) gst_backend = 4;
		
		//preload next effect and start in background if its video
		if ((self_state_previous != -3 && self_state == -3) || (self_state_previous != 3  && self_state == 3) ){
			//movies
			if (gst_backend >= 50){
				gst_backend = 0;
			}
			next_effect = self_playlist[self_playlist_index++];
			if (self_playlist[self_playlist_index] < 0) self_playlist_index = 0;
			 
			if (next_effect >= 10 && next_effect < 20){
				gst_backend = next_effect;
				printf("\n\nPreloading vis\n");
			}else{
				gst_backend = 0;
				printf("\n\nNot preloading movie\n");
			}
		}
		
		
		//start new video
		if ((self_state_previous != -4 && self_state == -4) || (self_state_previous != 4  && self_state == 4) ){
			video_playing = 1;
			
			if( self_state == 4)	   self_state=5;
			else if( self_state == -4) self_state=-5;
			
			printf("\n\n Starting Video or viz\n");
			gst_backend = next_effect; //run video
			if (gst_backend >= 50 && gst_backend <= 61){
				video_start_time = time_this_cycle;
				active_video_lentgh = video_length[gst_backend - 50];			
			}
		}
		
		//video auto close if video ends
		if (gst_backend >= 50){
			if ( time_this_cycle - video_start_time >  active_video_lentgh + 1500){
				if (self_state == 5)       self_state = 3;
				else if (self_state == -5) self_state = -3;
				gst_backend = 0;
			}
		}
		
		//video kill switch
		if ((self_state != 4 && self_state != -4 && self_state != 5 && self_state != -5 && gst_backend >= 50 )){
			gst_backend =0;			
		}

		
		
		
		//ahrs effects
		int ahrs_number = 9;
		// for networked modes
		if (self_state == 0){
			if (local_state ==3) {
				ahrs_number = 7;
			}else if (local_state >= 4) {
				ahrs_number = 6;
			}		
		}
		// for self modes
		if (local_state == 0){
			if (self_state ==3 || self_state ==4){
				ahrs_number = 7;
			}else if (self_state == -3 || self_state == -4){
				ahrs_number =  1;
			}else if (self_state < -4){
				ahrs_number = 0;
			}else if (self_state > 4){
				ahrs_number = 6;
			}		
		}
		ahrs_command(2,2,2,ahrs_number);
		
		//float buttonbrightness = ledcontrol_update(color1,width_request,width_speed,shutdown_effect, total_time_offset);
		
		gst_command(gst_backend);	
				
		//this keeps the subprocesses running at 50hz since main thread is 100hz	
		if(every_other_cycle) led_update(time_this_cycle,other_gun_clock_offset);
		
		every_other_cycle = !every_other_cycle;
		
		//send data to other gun
		if (time_this_cycle - udp_send_time > 100){
			udp_send_state(&local_state,&time_this_cycle);
			udp_send_time = time_this_cycle;
			web_output(gst_backend,local_state);
		}
		
		//fps counter code
		fps++;
		if (fps_counter < millis()){
			//if (wiicontrol_c()) gst_backend++;
			
			//if(gst_backend == 4) gst_backend = 10;
			//if(gst_backend == 16) gst_backend = 20;
			//if(gst_backend == 40) gst_backend = 0;

			
			printf("SPI FPS:%d missed: %d\n",fps,missed);
			fps = 0;
			fps_counter += 1000;
			
			if (fps_counter < millis()){
				fps_counter = millis()+1000;
			}			
		}	
	}
	return 0;
}



void local_state_engine(int button){
	

	if( button == BUTTON_ORANGE_LONG || button == BUTTON_BLUE_LONG){
		local_state = 0; //reset local state
		connection_initiator = 0; //reset initiator
		self_state = 0; //reset self state
		
	}
	
	else if (button == BUTTON_ORANGE_SHORT){

		if(local_state == 0 && self_state == 0){
			local_state=1;
		}else if(local_state == 1){
			local_state=2;
			connection_initiator=1; 
		}else if(local_state == 2 && connection_initiator == 1){
			local_state=3;
		}else if(local_state == 4){
			local_state=5;
		}else if(local_state == 5){
			local_state=4;
		}else if(local_state == -4){ //swap places
			local_state=4;
		}else if(local_state == 2 && connection_initiator == 0){  //answer an incoming call immediately and open portal on button press
			local_state=4;  
		}else if(self_state > 0 && self_state < 4){
			self_state++;
		}			
	}
	
	else if (button == BUTTON_BLUE_SHORT){

		if (local_state ==0 && self_state == 0){
			local_state = -1;
		}else if (local_state ==-1){
			local_state =-2;
			connection_initiator = 1;
		}else if (local_state ==-2 && connection_initiator == 1){
			local_state = -3;
		}else if (local_state ==-2 && connection_initiator == 0){
			local_state = -4;
		}else if (local_state ==-3 && connection_initiator == 0){
			local_state = -4; //connection established
		}else if(self_state < 0 && self_state > -4){
			self_state--;
		}
	}
	
	else if (button == BUTTON_BOTH_LONG_ORANGE){

		if ((local_state == 0 && self_state == 0) || local_state == 1||local_state == 2||local_state == 3  ){
			self_state = local_state +1;
			local_state_previous = local_state = 0;  //avoid transition changes
		}else if(self_state ==1 ||  self_state ==2 || self_state ==3){
			self_state++;
		}else if(self_state == -3 || self_state == -4 || self_state == -5 || self_state == 4  || self_state == 5){
			self_state=3;
		}			
	}
	
	else if (button == BUTTON_BOTH_LONG_BLUE){

		if ((local_state == 0 && self_state == 0)|| local_state == -1 || local_state == -2 ||local_state == -3){
			self_state =local_state-1;  //do this outside of MAX macro
			self_state = MAX(self_state,-3); //blue needs to clamp to -3 for visual consistency since we dont have a blue portal in local state -3 mode
			
			local_state_previous = local_state = 0;  //avoid transition changes
		}else if(self_state ==-1 || self_state ==-2 || self_state ==-3){
			self_state--;
		}else if(self_state == 3 || self_state == 4  || self_state == 5 || self_state == -4  || self_state == -5){
			self_state= -3;
		}
	}
	
}

void network_state_engine(int remote_state){
	if (self_state == 0){
		if ((remote_state_previous >= 2 || remote_state_previous <= -2)  && remote_state == 0){
			local_state = 0;
		}
		
		if (remote_state_previous != -2 && remote_state == -2 && connection_initiator == 0){
			local_state = 2;
		}
		
		if (remote_state_previous != 2 && remote_state == 2 && connection_initiator == 0){
			local_state = -2;
		}
		
		if (remote_state_previous != 3 && remote_state == 3 && connection_initiator == 0){
			local_state = -3;
		}
		
		if (remote_state_previous != -3 && remote_state == -3 && connection_initiator == 0){
			local_state = 2;
		}
		
		//if (remote_state_previous != -3 && remote_state == -3 && connection_initiator == 1){
		//	local_state = 4;
		//	connection_initiator = 0;
		//}
		
		if (remote_state_previous != -4 && remote_state == -4 && connection_initiator == 1){
			local_state = 4;
			connection_initiator = 0;
		}
		
		if (remote_state_previous != 4 && remote_state == 4 && connection_initiator == 1){
			local_state = -4;
			connection_initiator = 0;
		}
		
		if (remote_state_previous == -4 && remote_state >= 4 ){
			local_state = -4;
			connection_initiator = 0;
		}
	}else{
		//code to pull out of self state
		if ((remote_state_previous != remote_state )&& (remote_state <= -2)){
			//cmus_remote_play("/home/pi/portalgun/portal_open2.wav");		
			
			if (self_state <= -3 || self_state>=3){
				local_state = 4;
			}else {
				local_state = 2;
			}
			self_state = 0;
		}
	}
}

void led_update(int time_this_cycle, uint32_t other_gun_clock_offset){
	int color1 = -1;
	//set color from state data		
	if (local_state > 0 || self_state > 0)		color1 = 20;
	else if(local_state < 0 || self_state < 0)	color1 = 240;
	
	
	int width_request = 20;
	//set width	
	if(local_state == 1 || self_state == -1 || self_state == 1 || self_state == -1 ){
		width_request = 10;
	}else if(local_state == -1 )		width_request = 1;	
	else if(local_state == -2 )		width_request = 5;	
	else if(local_state == -3 )		width_request = 10;	
	
	
	int width_speed = 200;
	//set width speed
	if (local_state <= -4 ||  local_state>= 4 || self_state <= -4 ||  self_state>= 4 ){
		width_speed = 0;
	}
	
	int shutdown_effect = 0;
	//shutdown_effect
	if (local_state == 0 && self_state == 0) shutdown_effect = 1;
	
	const int effect_resolution = 400;
	const int breathing_rate = 2000;
	int total_time_offset = 0;
	
	if (other_gun_connected) {
		total_time_offset=  int((float)(((time_this_cycle + other_gun_clock_offset) / (2))% (breathing_rate)) * ((float)effect_resolution)/((float)(breathing_rate)));
	}else{
		total_time_offset=  int((float)((time_this_cycle)% (breathing_rate)) * ((float)effect_resolution)/((float)(breathing_rate)));
	}
	
	ledcontrol_update(color1,width_request,width_speed,shutdown_effect, total_time_offset);
	
}