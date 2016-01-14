#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "pipecontrol.h"
#include <sys/types.h>
#include <sys/stat.h>

int camera_PID;
int audio_send_PID;
int web_control;

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
	
	printf("BASH_CONTROL: SPAWNING PROCESS\n");
	bash_fp = popen("bash", "w");
	//fcntl(fileno(bash_fp), F_SETFL, O_NONBLOCK);
	printf("BASH_CONTROL: READY\n");
	
	printf("AHRS_CONTROL: SPAWNING PROCESS\n");
	ahrs_fp = popen("/home/pi/ahrs-visualizer/ahrs-visualizer", "w");
	//fcntl(fileno(ahrs_fp), F_SETFL, O_NONBLOCK);
	printf("AHRS_CONTROL: READY\n");
	
	printf("GST_CONTROL: SPAWNING PROCESS\n");
	gst_fp = popen("/home/pi/popen/popen", "w");
	fcntl(fileno(gst_fp), F_SETFL, O_NONBLOCK);
	printf("GST_CONTROL: READY\n");
	
	printf("WEB_PIPE: MKFIFO\n");
	mkfifo ("/tmp/FIFO_PIPE", 0777 );
	//OPEN PIPE WITH READ ONLY
	if ((web_control = open ("/tmp/FIFO_PIPE",  ( O_RDONLY | O_NONBLOCK ) ))<0)
	{
		perror("WEB_PIPE: Could not open named pipe for reading.");
		exit(-1);
	}
	fcntl(web_control, F_SETFL, O_NONBLOCK);
	
	fprintf(bash_fp, "sudo chown www-data /tmp/FIFO_PIPE\n");
	fflush(bash_fp);
	
	printf("WEB_PIPE has been opened.\n");
	
	system("LD_LIBRARY_PATH=/usr/local/lib mjpg_streamer -i 'input_file.so -f /var/www/html/tmp -n snapshot.jpg' -o 'output_http.so -w /usr/local/www' &");
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

void web_output(int mode1, int mode2){
	//printf("aplay %d\n",filename);
	fprintf(bash_fp, "echo '%d %d' > /var/www/html/tmp/portal.txt &\n",mode1,mode2);
	fflush(bash_fp);
}

int read_web_pipe(void){
	int temp = -1;
	
		int count = 1;
		char buffer[100];
		//stdin is line buffered so we can cheat a little bit
		while (count > 0){ // dump entire buffer
			count = read(web_control, buffer, sizeof(buffer)-1);
			if (count > 1){ //ignore blank lines
				buffer[count-1] = '\0';
				//keep most recent line
				int temp_state = 0;
				int result = sscanf(buffer,"%d", &temp_state);
				if (result != 1){
					fprintf(stderr, "WEB_PIPE: Unrecognized input with %d items.\n", result);
				}else{
					temp = temp_state;
				}
			}
		}
	
	
	
	return temp;
}


