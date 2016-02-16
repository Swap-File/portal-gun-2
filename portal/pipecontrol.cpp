#include "portal.h"
#include "pipecontrol.h"
#include "arduino.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>  

int ip;

int web_control;
int gstreamer_crashes = 0;
int ahrs_crashes = 0;

FILE *bash_fp;
FILE *ahrs_fp;
FILE *gst_fp;

void pipecontrol_cleanup(void){
	printf("KILLING OLD PROCESSES\n");
	system("pkill gst");
	system("pkill mjpeg");
	system("pkill raspivid");
	system("pkill ahrs");
}

void pipecontrol_setup(int new_ip){
	ip = new_ip;
	pipecontrol_cleanup();
	
	printf("BASH_CONTROL: SPAWNING PROCESS\n");
	bash_fp = popen("bash", "w");
	fcntl(fileno(bash_fp), F_SETFL, fcntl(fileno(bash_fp), F_GETFL, 0) | O_NONBLOCK);
	printf("BASH_CONTROL: READY\n");
	
	launch_ahrs_control();
	
	launch_gst_control();
	
	printf("WEB_PIPE: MKFIFO\n");
	mkfifo ("/tmp/FIFO_PIPE", 0777 );
	//OPEN PIPE WITH READ ONLY
	if ((web_control = open ("/tmp/FIFO_PIPE",  ( O_RDONLY | O_NONBLOCK ) ))<0)
	{
		perror("WEB_PIPE: Could not open named pipe for reading.");
		exit(-1);
	}
	fcntl(web_control, F_SETFL, fcntl(web_control, F_GETFL, 0) | O_NONBLOCK);
	
	fprintf(bash_fp, "sudo chown www-data /tmp/FIFO_PIPE\n");
	fflush(bash_fp);
	
	printf("WEB_PIPE has been opened.\n");
	
	system("LD_LIBRARY_PATH=/usr/local/lib mjpg_streamer -i 'input_file.so -f /var/www/html/tmp -n snapshot.jpg' -o 'output_http.so -w /usr/local/www' &");
}

void ahrs_command(int x, int y, int z, int number){
	
	errno = 0;
	int completed = 0;
	
	while (completed == 0){
		fprintf(ahrs_fp, "%d %d %d %d\n",x,y,z,number);
		fflush(ahrs_fp);
		if (errno == EPIPE) {
			printf("BROKEN PIPE TO AHRS_CONTROL!\n");
			fclose(ahrs_fp);
			launch_ahrs_control();
			errno = 0;
			ahrs_crashes++;
		}else{
			completed = 1;
		}
	}
	

}		

void launch_ahrs_control(void){
	printf("AHRS_CONTROL: SPAWNING PROCESS\n");
	ahrs_fp = popen("/home/pi/portal/ahrs-visualizer/ahrs-visualizer", "w");
	fcntl(fileno(ahrs_fp), F_SETFL, fcntl(fileno(ahrs_fp), F_GETFL, 0) | O_NONBLOCK);
	printf("AHRS_CONTROL: READY\n");
}

void gst_command(int number){
	
	errno = 0;
	int completed = 0;
	
	while (completed == 0){
		
		fprintf(gst_fp, "%d\n",number);
		fflush(gst_fp);
		if (errno == EPIPE) {
			printf("BROKEN PIPE %d TO GST_CONTROL!\n",gstreamer_crashes);
			fclose(gst_fp);
			launch_gst_control();
			errno = 0;
			gstreamer_crashes++;
		}else{
			completed = 1;
		}
	}
}

void launch_gst_control(void){
	printf("GST_CONTROL: SPAWNING PROCESS\n");
	if (ip == 22) gst_fp = popen("/home/pi/portal/gstvideo/gstvideo 22", "w");
	else if (ip == 23) gst_fp = popen("/home/pi/portal/gstvideo/gstvideo 23", "w");
	fcntl(fileno(gst_fp), F_SETFL, fcntl(fileno(gst_fp), F_GETFL, 0) | O_NONBLOCK);
	printf("GST_CONTROL: READY\n");
}


void aplay(const char *filename){
	printf("aplay %s\n",filename);
	fprintf(bash_fp, "aplay %s &\n",filename);
	fflush(bash_fp);
}

void web_output(const this_gun_struct& this_gun,const arduino_struct& arduino ){
	fprintf(bash_fp, "echo '%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %.2f %.2f %d %d %d %d %d' > /var/www/html/tmp/portal.txt &\n" ,\
	this_gun.private_state,this_gun.shared_state,this_gun.connected ,this_gun.ir_pwm,this_gun.private_playlist[0],\
	this_gun.private_playlist[1],this_gun.private_playlist[2],this_gun.private_playlist[3],this_gun.private_playlist[4],\
	this_gun.private_playlist[5],this_gun.private_playlist[6],this_gun.private_playlist[7],this_gun.private_playlist[8],\
	this_gun.private_playlist[9],this_gun.private_effect,this_gun.private_playlist_index,this_gun.shared_playlist[0],\
	this_gun.shared_playlist[1],this_gun.shared_playlist[2],this_gun.shared_playlist[3],this_gun.shared_playlist[4],\
	this_gun.shared_playlist[5],this_gun.shared_playlist[6],this_gun.shared_playlist[7],this_gun.shared_playlist[8],\
	this_gun.shared_playlist[9],this_gun.shared_effect,this_gun.shared_playlist_index,arduino.battery_level_pretty,\
	arduino.temperature_pretty,arduino.packets_in_per_second,arduino.packets_out_per_second,arduino.framing_error,\
	arduino.crc_error,arduino.cpuload);
	fflush(bash_fp);
}


int read_web_pipe(this_gun_struct& this_gun){
	int web_button = BUTTON_NONE;
	int count = 1;
	char buffer[100];
	//stdin is line buffered so we can cheat a little bit
	while (count > 0){ // dump entire buffer
		count = read(web_control, buffer, sizeof(buffer)-1);
		if (count > 1){ //ignore blank lines
			buffer[count-1] = '\0';
			//keep most recent line
			int tv[11];
			printf(" \n'%s'\n",buffer);
			int results = sscanf(buffer,"%d %d %d %d %d %d %d %d %d %d %d", &tv[0],&tv[1],&tv[2],&tv[3],&tv[4],&tv[5],&tv[6],&tv[7],&tv[8],&tv[9],&tv[10]);
			//button stuff
			if (tv[0] == 1 && results == 2) {
				switch (tv[1]){
				case WEB_ORANGE_WIFI:	web_button = BUTTON_ORANGE_SHORT;  		break;
				case WEB_BLUE_WIFI:		web_button = BUTTON_BLUE_SHORT;    		break;
				case WEB_BLUE_SELF:		web_button = BUTTON_BOTH_LONG_BLUE; 	break; 
				case WEB_ORANGE_SELF: 	web_button = BUTTON_BOTH_LONG_ORANGE; 	break; 
				case WEB_CLOSE: 		web_button = BUTTON_BLUE_LONG; 			break; 
				default: 				web_button = BUTTON_NONE;
				}
			}
			//ir stuff
			if (tv[0] == 2 && results == 2) {
				if (tv[1] >= 0 && tv[1] <= 255) this_gun.ir_pwm = tv[1];
			}
			//self playlist setting
			if (tv[0] == 3 && results == 11) {
				for (int i = 0; i < 10; i++) this_gun.private_playlist[i] = tv[i+1];
				this_gun.private_playlist_index = 0;
			}
			//shared playlist setting
			if (tv[0] == 4 && results == 11) {
				for (int i = 0; i < 10; i++) this_gun.shared_playlist[i] = tv[i+1];
				this_gun.shared_playlist_index = 0;
			}
			//playlist index change self
			if (tv[0] == 5 && results == 2) {
				this_gun.private_playlist_index = tv[1];
			}
			//playlist index change self
			if (tv[0] == 6 && results == 2) {
				this_gun.shared_playlist_index = tv[1];
			}
			
		}
	}

	return web_button;
}


void audio_effects(const this_gun_struct& this_gun){
	
	//LOCAL STATES
	if ((this_gun.shared_state_previous != 0 || this_gun.private_state_previous != 0) && (this_gun.shared_state == 0 && this_gun.private_state == 0)){
		aplay("/home/pi/portalgun/portal_close1.wav");
	}
	//on entering state 1
	if ((this_gun.shared_state_previous != this_gun.shared_state) && (this_gun.shared_state == 1)){
		aplay("/home/pi/physcannon/physcannon_charge1.wav");
	}
	//on entering state -1
	if ((this_gun.shared_state_previous != -1)&& (this_gun.shared_state == -1)){
		aplay("/home/pi/physcannon/physcannon_charge1.wav");			
	}
	//on entering state 2
	if ((this_gun.shared_state_previous !=2 ) &&  (this_gun.shared_state == 2)){
		aplay("/home/pi/physcannon/physcannon_charge2.wav");		
	}
	//on entering state -2 from -1
	if ((this_gun.shared_state_previous !=-2 ) &&  (this_gun.shared_state == -2)){
		aplay("/home/pi/physcannon/physcannon_charge2.wav");
	}	
	if ((this_gun.shared_state_previous !=-3 ) &&  (this_gun.shared_state == -3)){
		aplay("/home/pi/physcannon/physcannon_charge3.wav");
	}	
	//on entering state 3 from 2
	if ((this_gun.shared_state_previous == 2 ) && ( this_gun.shared_state ==3)){	
		aplay("/home/pi/portalgun/portalgun_shoot_blue1.wav");
	}
	//on quick swap to rec
	if ((this_gun.shared_state_previous < 4 )&& (this_gun.shared_state == 4)){
		aplay("/home/pi/portalgun/portal_open2.wav");
	}
	//on quick swap to transmit
	if ((this_gun.shared_state_previous >= 4 )&& (this_gun.shared_state <= -4)){
		aplay("/home/pi/portalgun/portal_fizzle2.wav");
	}	

	//shared effect change close portal end sfx
	if (this_gun.shared_state_previous == 4 && this_gun.shared_state == 5){
		aplay("/home/pi/portalgun/portal_close1.wav");
	}	
	//shared effect change open portal end sfx
	if (this_gun.shared_state_previous == 5 && this_gun.shared_state == 4){
		aplay("/home/pi/portalgun/portal_open1.wav");
	}	
	
	//SELF STATES
	if ((this_gun.private_state_previous != this_gun.private_state) && (this_gun.private_state == 1 || this_gun.private_state == -1)){
		aplay("/home/pi/physcannon/physcannon_charge1.wav");
	}
	//on entering state 2 or -2
	if ((this_gun.private_state_previous != this_gun.private_state) && (this_gun.private_state == 2 || this_gun.private_state == -2)){
		aplay("/home/pi/physcannon/physcannon_charge2.wav");				
	}
	
	//on entering state 3 or -3 from 0
	if ((this_gun.private_state_previous < 3 && this_gun.private_state_previous > -3  ) && (this_gun.private_state == 3 || this_gun.private_state == -3)){
		aplay("/home/pi/portalgun/portalgun_shoot_blue1.wav");
	}


	
	//on quick swap 
	if (( this_gun.private_state_previous >= 3 && this_gun.private_state == -3) || (this_gun.private_state_previous <= -3 && this_gun.private_state == 3)){
		aplay("/home/pi/portalgun/portal_open2.wav");		
	}
	
	//private end sfx
	if ((this_gun.private_state_previous > 3 && this_gun.private_state == 3) || (this_gun.private_state_previous < -3 && this_gun.private_state == -3)){
		aplay("/home/pi/portalgun/portal_close1.wav");
	}
	
	//private start sfx
	if ((this_gun.private_state_previous != -5 && this_gun.private_state == -5) || (this_gun.private_state_previous != 5 && this_gun.private_state == 5)){
		aplay("/home/pi/portalgun/portal_open1.wav");
	}
	
	//rip from private to shared mode sfx
	if ((this_gun.private_state_previous <= -3 || this_gun.private_state_previous>=3) && this_gun.shared_state == 4){
		aplay("/home/pi/portalgun/portal_open2.wav");		
	}
	
	return;
}