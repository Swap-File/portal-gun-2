#include "portal.h"

//structs bundle all the data that needs to be passed to state engines
struct this_gun_struct {
	int shared_state = 0;  //state reported to other gun
	int shared_state_previous = 0;
	int private_state = 0; //internal state for single player modes
	int private_state_previous = 0;
	bool initiator = false; //Did this gun start the connection request?
	uint32_t clock = 0;
} this_gun_struct;  

struct other_gun_struct {
	int state = 0; //state read from other gun
	int state_previous = 0;
	bool connected = false; 
	uint32_t last_seen = 0;
	uint32_t clock = 0;
} other_gun_struct;  

void INThandler(int dummy) {
	printf("\nCleaning up...\n");
	ledcontrol_wipe();
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
	
	const uint32_t video_length[12] = {24467,16767,12067,82000,30434,22900,19067,70000,94000,53167,184000,140000} ;
	uint32_t video_start_time =0 ;
	uint32_t active_video_lentgh = 0;

	//reset playlists on close? (make option?)
	int private_playlist[10]={10,50,13,51,14,51,15,-1};
	int private_playlist_index = 0;

	int shared_playlist[10]={4,20,29,-1};
	int shared_playlist_index = 0;


	struct other_gun_struct other_gun;
	struct this_gun_struct this_gun;

	int ip = get_ip();

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
	
	pipecontrol_setup(ip);
	ledcontrol_setup();
	udpcontrol_setup(ip);
	wiicontrol_setup();
	
	int gst_backend = 0;
	int next_effect = 50;
	bool every_other_cycle = true; //toggles every other cycle
	
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
		every_other_cycle = !every_other_cycle;
		this_gun.shared_state_previous = this_gun.shared_state;
		this_gun.private_state_previous = this_gun.private_state;
		other_gun.state_previous = other_gun.state;
		
		wiicontrol_update();
		
		int button_event = BUTTON_NONE;
		int result = read_web_pipe();
		if (result != -1){
			printf("PIPE INCOMING! %d",result);
			switch (result){
			case WEB_ORANGE_WIFI:	button_event = BUTTON_ORANGE_SHORT;  	break;
			case WEB_BLUE_WIFI:		button_event = BUTTON_BLUE_SHORT;    	break;
			case WEB_BLUE_SELF:		button_event = BUTTON_BOTH_LONG_BLUE; 	break; 
			case WEB_ORANGE_SELF: 	button_event = BUTTON_BOTH_LONG_ORANGE; break; 
			case WEB_CLOSE: 		button_event = BUTTON_BLUE_LONG; 		break; 
			default: 
				gst_backend = result;
			}
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
			if (other_gun.connected != false){
				other_gun.connected = false;
				printf("\nGun Expired\n");	
			}
			other_gun.state = 0;
		}
		else {
			if(other_gun.connected == false){
				other_gun.connected = true;
				printf("\nGun Connected\n");
			}
		}
		
		//process state changes
		local_state_engine(button_event,&this_gun,&other_gun);
		
		//shared state stuff
		
		//start camera (preload in state 3, shutter is closed)
		if(this_gun.shared_state <= -3) gst_backend = 3;
		
		//start live feed playback (preload in state 3, shutter is closed)
		if(this_gun.shared_state >= 3) gst_backend = 4;
		
		
		//private state stuff
		
		//preload next effect and start in background if its video
		if ((this_gun.private_state_previous != -3 && this_gun.private_state == -3) || (this_gun.private_state_previous != 3  && this_gun.private_state == 3) ){
			//movies
			if (gst_backend >= 50){
				gst_backend = 0;
			}
			next_effect = private_playlist[private_playlist_index++];
			if (private_playlist[private_playlist_index] < 0) private_playlist_index = 0;
			
			if (next_effect >= 10 && next_effect < 20){
				gst_backend = next_effect;
				printf("\n\nPreloading vis\n");
			}else{
				gst_backend = 0;
				printf("\n\nNot preloading movie\n");
			}
		}
		
		//start new video
		if ((this_gun.private_state_previous != -4 && this_gun.private_state == -4) || (this_gun.private_state_previous != 4  && this_gun.private_state == 4) ){
			
			if( this_gun.private_state == 4)	   this_gun.private_state=5;
			else if( this_gun.private_state == -4) this_gun.private_state=-5;
			
			printf("\n\n Starting Video or viz\n");
			gst_backend = next_effect; //run video
			if (gst_backend >= 50 && gst_backend <= 61){
				video_start_time = this_gun.clock;
				active_video_lentgh = video_length[gst_backend - 50];			
			}
		}
		
		//video auto close if video ends
		if (gst_backend >= 50){
			if ( this_gun.clock - video_start_time >  active_video_lentgh + 1500){
				if (this_gun.private_state == 5)       this_gun.private_state = 3;
				else if (this_gun.private_state == -5) this_gun.private_state = -3;
				gst_backend = 0;
			}
		}
		
		//video kill switch
		if ((this_gun.private_state != 4 && this_gun.private_state != -4 && this_gun.private_state != 5 && this_gun.private_state != -5 && gst_backend >= 50 )){
			gst_backend = 0;			
		}
		
		//ahrs effects
		int ahrs_number = 9;
		// for networked modes
		if (this_gun.private_state == 0){
			if (this_gun.shared_state ==3) {
				ahrs_number = 7;
			}else if (this_gun.shared_state >= 4) {
				ahrs_number = 6;
			}		
		}
		// for self modes
		if (this_gun.shared_state == 0){
			if (this_gun.private_state == 3 || this_gun.private_state == 4){
				ahrs_number = 7;
			}else if (this_gun.private_state == -3 || this_gun.private_state == -4){
				ahrs_number =  1;
			}else if (this_gun.private_state < -4){
				ahrs_number = 0;
			}else if (this_gun.private_state > 4){
				ahrs_number = 6;
			}		
		}
		ahrs_command(2,2,2,ahrs_number);
		
		//float buttonbrightness = ledcontrol_update(color1,width_request,width_speed,shutdown_effect, total_time_offset);
		
		gst_command(gst_backend);	
		
		//this keeps the subprocesses running at 50hz since main thread is 100hz	
		if(every_other_cycle) led_update(&this_gun,&other_gun);
		
		//send data to other gun
		if (this_gun.clock - udp_send_time > 100){
			udp_send_state(&this_gun.shared_state,&this_gun.clock);
			udp_send_time = this_gun.clock;
			web_output(gst_backend,this_gun.shared_state);
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
				fps_counter = millis() + 1000;
			}			
		}	
	}
	return 0;
}

void local_state_engine(int button,struct this_gun_struct *this_gun,struct other_gun_struct *other_gun){
	
	//button event transitions
	if( button == BUTTON_ORANGE_LONG || button == BUTTON_BLUE_LONG){
		this_gun->shared_state = 0; //reset local state
		this_gun->initiator = false; //reset initiator
		this_gun->private_state = 0; //reset self state
	}
	else if (button == BUTTON_ORANGE_SHORT){

		if(this_gun->shared_state == 0 && this_gun->private_state == 0){
			this_gun->shared_state = 1;
		}else if(this_gun->shared_state == 1){
			this_gun->shared_state = 2;
			this_gun->initiator = true; 
		}else if(this_gun->shared_state == 2 && this_gun->initiator == true){
			this_gun->shared_state = 3;
		}else if(this_gun->shared_state == 4){
			this_gun->shared_state = 5;
		}else if(this_gun->shared_state == 5){
			this_gun->shared_state = 4;
		}else if(this_gun->shared_state == -4){ //swap places
			this_gun->shared_state = 4;
		}else if(this_gun->shared_state == 2 && this_gun->initiator == false){  //answer an incoming call immediately and open portal on button press
			this_gun->shared_state = 4;  
		}else if(this_gun->private_state > 0 && this_gun->private_state < 4){
			this_gun->private_state++;
		}			
	}
	else if (button == BUTTON_BLUE_SHORT){

		if (this_gun->shared_state ==0 && this_gun->private_state == 0){
			this_gun->shared_state = -1;
		}else if (this_gun->shared_state == -1){
			this_gun->shared_state = -2;
			this_gun->initiator = true;
		}else if (this_gun->shared_state == -2 && this_gun->initiator == true){
			this_gun->shared_state = -3;
		}else if (this_gun->shared_state == -2 && this_gun->initiator == false){
			this_gun->shared_state = -4;
		}else if (this_gun->shared_state == -3 && this_gun->initiator == false){
			this_gun->shared_state = -4; //connection established
		}else if(this_gun->private_state < 0 && this_gun->private_state > -4){
			this_gun->private_state--;
		}
	}
	else if (button == BUTTON_BOTH_LONG_ORANGE){

		if ((this_gun->shared_state == 0 && this_gun->private_state == 0) || this_gun->shared_state == 1 || this_gun->shared_state == 2 || this_gun->shared_state == 3){
			this_gun->private_state = this_gun->shared_state + 1;
			this_gun->shared_state_previous = this_gun->shared_state = 0;  //avoid transition changes
		}else if(this_gun->private_state == 1 ||  this_gun->private_state == 2 || this_gun->private_state == 3){
			this_gun->private_state++;
		}else if(this_gun->private_state == -3 || this_gun->private_state == -4 || this_gun->private_state == -5 || this_gun->private_state == 4 || this_gun->private_state == 5){
			this_gun->private_state=3;
		}			
	}
	else if (button == BUTTON_BOTH_LONG_BLUE){

		if ((this_gun->shared_state == 0 && this_gun->private_state == 0) || this_gun->shared_state == -1 || this_gun->shared_state == -2 || this_gun->shared_state == -3){
			this_gun->private_state =this_gun->shared_state-1;  //do this outside of MAX macro
			this_gun->private_state = MAX(this_gun->private_state,-3); //blue needs to clamp to -3 for visual consistency since we dont have a blue portal in local state -3 mode
			
			this_gun->shared_state_previous = this_gun->shared_state = 0;  //avoid transition changes
		}else if(this_gun->private_state ==-1 || this_gun->private_state == -2 || this_gun->private_state == -3){
			this_gun->private_state--;
		}else if(this_gun->private_state == 3 || this_gun->private_state == 4  || this_gun->private_state == 5 || this_gun->private_state == -4 || this_gun->private_state == -5){
			this_gun->private_state= -3;
		}
	}
	
	//other gun transitions
	if (this_gun->private_state == 0){
		if ((other_gun->state_previous >= 2 || other_gun->state_previous <= -2)  && other_gun->state == 0){
			this_gun->shared_state = 0;
		}
		
		if (other_gun->state_previous != -2 && other_gun->state == -2 && this_gun->initiator == false){
			this_gun->shared_state = 2;
		}
		
		if (other_gun->state_previous != 2 && other_gun->state == 2 && this_gun->initiator == false){
			this_gun->shared_state = -2;
		}
		
		if (other_gun->state_previous != 3 && other_gun->state == 3 && this_gun->initiator == false){
			this_gun->shared_state = -3;
		}
		
		if (other_gun->state_previous != -3 && other_gun->state == -3 && this_gun->initiator == false){
			this_gun->shared_state = 2;
		}
				
		if (other_gun->state_previous != -4 && other_gun->state == -4 && this_gun->initiator == true){
			this_gun->shared_state = 4;
			this_gun->initiator = false;
		}
		
		if (other_gun->state_previous != 4 && other_gun->state == 4 && this_gun->initiator == true){
			this_gun->shared_state = -4;
			this_gun->initiator = false;
		}
		
		if (other_gun->state_previous == -4 && other_gun->state >= 4 ){
			this_gun->shared_state = -4;
			this_gun->initiator = false;
		}
	}else{
		//code to pull out of self state
		if ((other_gun->state_previous != other_gun->state )&& (other_gun->state <= -2)){
			//cmus_remote_play("/home/pi/portalgun/portal_open2->wav");		
			
			if (this_gun->private_state <= -3 || this_gun->private_state>=3){
				this_gun->shared_state = 4;
			}else {
				this_gun->shared_state = 2;
			}
			this_gun->private_state = 0;
		}
	}
	
}



void led_update(struct this_gun_struct *this_gun,struct other_gun_struct *other_gun){
	
	int color1 = -1;  //default color of white for shutdown state
	//set color from state data		
	if (this_gun->shared_state > 0 || this_gun->private_state > 0)		color1 = 20;
	else if(this_gun->shared_state < 0 || this_gun->private_state < 0)	color1 = 240;
	
	int width_request = 20;
	//set width
	if(this_gun->shared_state == 1 || this_gun->private_state == -1 || this_gun->private_state == 1 || this_gun->shared_state == -3 ){
		width_request = 10;
	}else if(this_gun->shared_state == -1)	width_request = 1;	
	else if(this_gun->shared_state == -2)	width_request = 5;	
	
	int width_speed = 200;
	//set width speed
	if (this_gun->shared_state <= -4 || 	this_gun->shared_state >= 4 || this_gun->private_state <= -4 || this_gun->private_state>= 4 ){
		width_speed = 0;
	}
	
	int shutdown_effect = 0;
	//shutdown_effect
	if (this_gun->shared_state == 0 && this_gun->private_state == 0) shutdown_effect = 1;
	
	#define effect_resolution 400
	#define breathing_rate 2000
	
	int total_time_offset;
	if (other_gun->connected) {
		total_time_offset = (this_gun->clock >> 1) + (other_gun->clock >> 1);  //average the two values
	}else{
		total_time_offset = this_gun->clock;
	}
	total_time_offset = int((float)(total_time_offset % breathing_rate) * ((float)effect_resolution)/((float)breathing_rate));
	
	ledcontrol_update(color1,width_request,width_speed,shutdown_effect,total_time_offset);
}