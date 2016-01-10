#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "pipecontrol.h"


int camera_PID;
int audio_send_PID;

FILE *bash_fp;
FILE *ahrs_fp;
FILE *gst_fp;

void pipecontrol_cleanup(void){
	//printf("KILLING OLD PROCESSES\n");
	system("pkill gst");
	system("pkill raspivid");
	system("pkill ahrs");
	system("pkill popen");
}

void pipecontrol_setup(void){
	pipecontrol_cleanup();
	
	printf("BASH_CONTROL: SPAWNING\n");
	bash_fp = popen("bash", "w");
	//fcntl(fileno(bash_fp), F_SETFL, O_NONBLOCK);
	printf("BASH_CONTROL: READY\n");
	
	printf("AHRS_CONTROL: SPAWNING\n");
	ahrs_fp = popen("/home/pi/ahrs-visualizer/ahrs-visualizer", "w");
	//fcntl(fileno(ahrs_fp), F_SETFL, O_NONBLOCK);
	printf("AHRS_CONTROL: READY\n");
	
	printf("GST_CONTROL: SPAWNING\n");
	gst_fp = popen("/home/pi/popen/popen", "w");
	fcntl(fileno(gst_fp), F_SETFL, O_NONBLOCK);
	printf("GST_CONTROL: READY\n");
		
}

void ahrs_command(int x, int y, int z, int number){
	fprintf(ahrs_fp, "%d %d %d %d\n",x,y,z,number);
	fflush(ahrs_fp);
}		

void gst_command(int number){
	fprintf(gst_fp, "%d\n",number);
	fflush(gst_fp);
}		

void aplay(const char *filename){
	printf("aplay %s\n",filename);
	fprintf(bash_fp, "aplay %s &\n",filename);
	fflush(bash_fp);
}