#include "popen_noshell.h"
#include <err.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <sys/time.h>  
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <wiringPi.h>

#include <gst/gst.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

//this crashes if left running for hours! figure it out
char *alsasrc[] = {"gst-launch-1.0","alsasrc","device=hw:1","buffer-time=20000","!","queue","!","udpsink","","port=5000", NULL};

//gstreamer launch codes
//queues are NEEDED otherwise occasionally launch will fail
//queue takes up the slack
char videotestsrc[] = "videotestsrc ! queue ! eglglessink";
char videotestsrc_cubed[] = "videotestsrc ! queue ! glupload ! glfiltercube ! gldownload ! eglglessink";

char *raspivid_src[] = {"raspivid","-w","400","-h","240","-fps","30","-g","10","-n","-pf","baseline","-ex","auto","-t","0","-o","-", NULL};
char *raspivid_sink[] = {"gst-launch-1.0","fdsrc","!","h264parse","!","tee","name=t","!","queue","!","rtph264pay","config-interval=1","pt=96","!","udpsink","","port=9000","t.","!","queue","!","avdec_h264","!","videoflip","method=1","!","jpegenc","!","multifilesink","location=/var/www/html/tmp/snapshot.jpg",NULL};

char normal[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! queue ! eglglessink";

char libvisual_jess[] = "udpsrc port=5000 caps=audio/x-raw,format=S16LE,layout=interleaved,rate=44100,channels=1 ! libvisual_jess ! eglglessink";
char libvisual_bumpscope[] = "udpsrc port=5000 caps=audio/x-raw,format=S16LE,layout=interleaved,rate=44100,channels=1 ! libvisual_bumpscope ! eglglessink";
char libvisual_corona[] = "udpsrc port=5000 caps=audio/x-raw,format=S16LE,layout=interleaved,rate=44100,channels=1 ! libvisual_corona ! eglglessink";
char libvisual_infinite[] = "udpsrc port=5000 caps=audio/x-raw,format=S16LE,layout=interleaved,rate=44100,channels=1 ! libvisual_infinite ! eglglessink";
char libvisual_jakdaw[] = "udpsrc port=5000 caps=audio/x-raw,format=S16LE,layout=interleaved,rate=44100,channels=1 ! libvisual_jakdaw ! eglglessink";
//  char libvisual_lv_analyzer[] = "udpsrc port=5000 caps=audio/x-raw,format=S16LE,layout=interleaved,rate=44100,channels=1 ! libvisual_lv_analyzer ! eglglessink";
//  char libvisual_lv_scope[] = "udpsrc port=5000 caps=audio/x-raw,format=S16LE,layout=interleaved,rate=44100,channels=1 ! libvisual_lv_scope ! eglglessink";
char libvisual_oinksie[] = "udpsrc port=5000 caps=audio/x-raw,format=S16LE,layout=interleaved,rate=44100,channels=1 ! libvisual_oinksie ! eglglessink";

char glfiltercube[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! glfiltercube ! gldownload ! eglglessink";
char gleffects_mirror[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_mirror ! gldownload ! eglglessink";
char gleffects_squeeze[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_squeeze ! gldownload ! eglglessink";
char gleffects_stretch[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_stretch ! gldownload ! eglglessink";
char gleffects_tunnel[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_tunnel ! gldownload ! eglglessink";
char gleffects_fisheye[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_fisheye ! gldownload ! eglglessink";
char gleffects_twirl[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_twirl ! gldownload ! eglglessink";
char gleffects_bulge[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_bulge ! gldownload ! eglglessink";
char gleffects_square[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_square ! gldownload ! eglglessink";
char gleffects_heat[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_heat ! gldownload ! eglglessink";
char gleffects_sepia[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_sepia ! gldownload ! eglglessink";
char gleffects_xpro[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_xpro ! gldownload ! eglglessink";
char gleffects_lumaxpro[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_lumaxpro ! gldownload ! eglglessink";
char gleffects_xray[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_xray ! gldownload ! eglglessink";
char gleffects_sin[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_sin ! gldownload ! eglglessink";
//  char gleffects_glow[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_glow ! gldownload ! eglglessink";
//  char gleffects_sobel[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_sobel ! gldownload ! eglglessink";
//  char gleffects_blur[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_blur ! gldownload ! eglglessink";
//  char gleffects_laplacian[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! glupload ! gleffects_laplacian ! gldownload ! eglglessink";

char revtv[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! revtv ! eglglessink";
char agingtv[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! agingtv ! eglglessink";
char dicetv[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! dicetv ! eglglessink";
char warptv[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! warptv ! eglglessink";
char shagadelictv[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! shagadelictv ! eglglessink";
char vertigotv[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! vertigotv ! eglglessink";
//  char quarktv[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! quarktv ! eglglessink";
char optv[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! optv ! eglglessink";
//  char radioactv[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! radioactv ! eglglessink";
char streaktv[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! streaktv ! eglglessink";
char rippletv[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! rippletv ! eglglessink";
char edgetv[] = "udpsrc port=9000 caps=application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! queue ! edgetv ! eglglessink";

char movie1[] = "filesrc location=/home/pi/movies/1.mp4 ! qtdemux name=dmux ! queue ! avdec_h264 ! eglglessink dmux. ! aacparse !  avdec_aac ! audioconvert ! queue ! alsasink device=hw:0";
char movie2[] = "filesrc location=/home/pi/movies/2.mp4 ! qtdemux name=dmux ! queue ! avdec_h264 ! eglglessink dmux. ! aacparse !  avdec_aac ! audioconvert ! queue ! alsasink device=hw:0";
char movie3[] = "filesrc location=/home/pi/movies/3.mp4 ! qtdemux name=dmux ! queue ! avdec_h264 ! eglglessink dmux. ! aacparse !  avdec_aac ! audioconvert ! queue ! alsasink device=hw:0";
char movie4[] = "filesrc location=/home/pi/movies/4.mp4 ! qtdemux name=dmux ! queue ! avdec_h264 ! eglglessink dmux. ! aacparse !  avdec_aac ! audioconvert ! queue ! alsasink device=hw:0";
char movie5[] = "filesrc location=/home/pi/movies/5.mp4 ! qtdemux name=dmux ! queue ! avdec_h264 ! eglglessink dmux. ! aacparse !  avdec_aac ! audioconvert ! queue ! alsasink device=hw:0";
char movie6[] = "filesrc location=/home/pi/movies/6.mp4 ! qtdemux name=dmux ! queue ! avdec_h264 ! eglglessink dmux. ! aacparse !  avdec_aac ! audioconvert ! queue ! alsasink device=hw:0";
char movie7[] = "filesrc location=/home/pi/movies/7.mp4 ! qtdemux name=dmux ! queue ! avdec_h264 ! eglglessink dmux. ! aacparse !  avdec_aac ! audioconvert ! queue ! alsasink device=hw:0";
char movie8[] = "filesrc location=/home/pi/movies/8.mp4 ! qtdemux name=dmux ! queue ! avdec_h264 ! eglglessink dmux. ! aacparse !  avdec_aac ! audioconvert ! queue ! alsasink device=hw:0";
char movie9[] = "filesrc location=/home/pi/movies/9.mp4 ! qtdemux name=dmux ! queue ! avdec_h264 ! eglglessink dmux. ! aacparse !  avdec_aac ! audioconvert ! queue ! alsasink device=hw:0";
char movie10[] = "filesrc location=/home/pi/movies/10.mp4 ! qtdemux name=dmux ! queue ! avdec_h264 ! eglglessink dmux. ! aacparse !  avdec_aac ! audioconvert ! queue ! alsasink device=hw:0";
char movie11[] = "filesrc location=/home/pi/movies/11.mp4 ! qtdemux name=dmux ! queue ! avdec_h264 ! eglglessink dmux. ! aacparse !  avdec_aac ! audioconvert ! queue ! alsasink device=hw:0";
char movie12[] = "filesrc location=/home/pi/movies/12.mp4 ! qtdemux name=dmux ! queue ! avdec_h264 ! eglglessink dmux. ! aacparse !  avdec_aac ! audioconvert ! queue ! alsasink device=hw:0";


char * new_cmd;

//keep track of processes
struct popen_noshell_pass_to_pclose alsasrc_process;
struct popen_noshell_pass_to_pclose raspivid_src_process;
struct popen_noshell_pass_to_pclose raspivid_sink_process;
FILE *raspivid_fp;


int requested_state = 0;
int active_state = -1; //force change to state 0 on launch

int temp_fd1;
int temp_fd2;
//major error counter
int errors =0;

char my_ip [50];
char dest_ip [50];

GstElement *pipeline;

struct timeval t1, t2;
double elapsedTime;

void timer_stop(void){
	// stop timer
	gettimeofday(&t2, NULL);

	// compute and print the elapsed time in millisec
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
	printf("Lag to preform %d: %f ms\n\n",requested_state,elapsedTime);	
}
void get_ip(void){

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
	sprintf(my_ip,"host=%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

	if (strstr(my_ip,"192.168.1.22")){
		strcpy(dest_ip, "host=192.168.1.23");
	}
	else if (strstr(my_ip,"192.168.1.23")){
		strcpy(dest_ip, "host=192.168.1.22");

	}
	alsasrc[8] = my_ip;
	raspivid_sink[15] = dest_ip;

	return;
	
}

void stop_gst_pipeline(void){
	gst_element_set_state (pipeline, GST_STATE_NULL);
	gst_object_unref (pipeline);	
}

void kill_process(struct popen_noshell_pass_to_pclose *process){
	if (process->pid != 0){
		kill(process->pid,SIGINT);
		pclose_noshell(process);
		process->pid = 0;
	}
}

void kill_process_hard(struct popen_noshell_pass_to_pclose *process){
	if (process->pid != 0){
		kill(process->pid,SIGTERM);
		pclose_noshell(process);
		process->pid = 0;
	}
}

void INThandler(int dummy) {
	printf("\nCleaning up...\n");
	//stop_gst_pipeline();
	exit(1);
}

void launch(char** launch_cmd,struct popen_noshell_pass_to_pclose *launch_process ){					
	//scratch buffers
	char buffer[100];
	FILE *fp;
	memset(buffer, 0, sizeof(buffer));
	
	retry_launch:	
	
	fp = NULL;
	while(!fp){
		fp = popen_noshell( *launch_cmd, (  char**)launch_cmd, "r", launch_process, 1);
		if (!fp) {
			printf("Failure popen_noshell\n");
			errors++;
			delay(10);
		}
	}
	
	//read stdout until playing
	
	int max_delay = 0;
	while (strstr(buffer, "PLAYING") == NULL) {
		fgets(buffer, sizeof(buffer)-1, fp);
		//printf("%s\n",buffer);
		delay(10);
		max_delay += 10;
		if (max_delay > 2000) {
			printf("\nFatal Loading Delay, Retrying:  %d  \n" , launch_process->pid);
			kill_process_hard(launch_process);
			kill_process_hard(&alsasrc_process);
			launch(alsasrc,&alsasrc_process);
			delay(100);
			errors++;
			goto retry_launch;  //alternative is nested loops with a flag...
		}
	}
}


void start_raspivid(){

	
	//scratch buffers
	char buffer[100];
	memset(buffer, 0, sizeof(buffer));
	FILE *fp;
	
	//pipe to couple the processes
	int outgoing[2];
	pipe(outgoing);
	
	retry_raspivid:
	
	//start loading raspivid before unloading gstreamer
	
	temp_fd2 = 0;
	temp_fd1 = 0;
	while(!temp_fd1){
		temp_fd1 = popen_noshell_fork( *raspivid_sink, (char **)raspivid_sink,&raspivid_sink_process, 1,outgoing[1]);
		if (!temp_fd1) {
			printf("Failure to launch raspivid_sink_process\n");
			errors++;
			delay(10);
		}
	}
	while(!temp_fd2){
		temp_fd2 = popen_noshell_fork( *raspivid_src, (char **)raspivid_src,&raspivid_src_process, 1,temp_fd1);
		if (!temp_fd2) {
			printf("Failure to launch raspivid_src_process\n");
			errors++;
			delay(10);
		}
	}

	//saves about 100-200ms doing it out of order...
	stop_gst_pipeline();
	
	//open pipe for reading
	fp = fdopen(outgoing[0], "r");
	
	//parse stream and wait for playing
	int max_delay = 0;
	while (strstr(buffer, "PLAYING") == NULL) {
		fgets(buffer, sizeof(buffer)-1,fp);
		delay(10);
		max_delay += 10;
		if (max_delay > 1000) {
			printf("\nPipeline didn't reach PLAYING state, Retrying...\n");
			kill_process_hard(&raspivid_sink_process);
			kill_process_hard(&raspivid_src_process);
			kill_process_hard(&alsasrc_process);
			launch(alsasrc,&alsasrc_process);
			close(temp_fd1);
			close(temp_fd2);
			raspivid_sink_process.pid = 0;
			raspivid_src_process.pid = 0;
			errors++;
			goto retry_raspivid; //alternative is nested loops with a flag...
		}
	}
	
	fclose(fp);
	close(outgoing[1]);
	close(outgoing[0]);
	timer_stop();
	active_state = requested_state;
}


int main(int argc, char *argv[]) {

	/* Initialize GStreamer */
	gst_init (&argc, &argv);

	//read src and set variables
	get_ip();
	
	//Start Alsa Background Process
	launch(alsasrc,&alsasrc_process);
	
	//stats
	uint32_t sampletime = 0;
	int missed = 0;
	uint32_t fps_counter = 0;
	int fps = 0;
	
	//non blocking sdtin read
	fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);
	
	//signal handler
	signal(SIGINT, INThandler);
	
	while (1){
		
		sampletime += 20;
		if (sampletime < millis()){
			sampletime = millis();
			//printf("Missed cycle(s), Skipping...\n");
			missed++;
		}else{
			delay(sampletime - millis()); //predictive delay
		}		
		
		int count = 1;
		char buffer[100];
		//stdin is line buffered so we can cheat a little bit
		while (count > 0){ // dump entire buffer
			count = read(STDIN_FILENO, buffer, sizeof(buffer)-1);
			if (count > 1){ //ignore blank lines
				buffer[count-1] = '\0';
				//keep most recent line
				int temp_state = 0;
				int result = sscanf(buffer,"%d", &temp_state);
				if (result != 1){
					fprintf(stderr, "POPEN_Process: Unrecognized input with %d items.\n", result);
				}else{
					requested_state = temp_state;
				}
			}
		}

		
		if (active_state != requested_state){
			
			// start timer
			gettimeofday(&t1, NULL);
			
			printf("\nStarting Request: %d\n",requested_state);
						
			if (requested_state == 2){
				//special case
				start_raspivid();
			}else{
				
				//figure out the correct app
				switch (requested_state){
					//the basics
				case 0:	new_cmd = videotestsrc;	break;
				case 1: new_cmd = videotestsrc_cubed; break;
				case 3: new_cmd = normal; break; 					
					//libvisual 10 - 18
				case 10: new_cmd = libvisual_jess; break;	  //good
				case 11: new_cmd = libvisual_bumpscope; break;	 //works but stupid sideways
				case 12: new_cmd = libvisual_corona; break;	//works but stupid sideways
				case 13: new_cmd = libvisual_infinite; break;	//good
				case 14: new_cmd = libvisual_jakdaw; break;	//good
				case 15: new_cmd = libvisual_oinksie; break;	//good		
					//tv effects	
				case 20: new_cmd = revtv; break;//good
				case 21: new_cmd = agingtv; break;//steampunk
				case 22: new_cmd = dicetv; break;//works
				case 23: new_cmd = warptv; break;//works
				case 24: new_cmd = shagadelictv; break;//works			
				case 25: new_cmd = vertigotv; break;//works
				case 26: new_cmd = optv; break;//looks stupid, swirl
				case 27: new_cmd = streaktv; break;//needs stable camera, might be ok
				case 28: new_cmd = rippletv; break;//works
				case 29: new_cmd = edgetv; break;//works					
					//gl effects	
				case 30: new_cmd = glfiltercube; break;
				case 31: new_cmd = gleffects_mirror; break;
				case 32: new_cmd = gleffects_squeeze; break;
				case 33: new_cmd = gleffects_stretch; break;
				case 34: new_cmd = gleffects_tunnel; break;			
				case 35: new_cmd = gleffects_fisheye; break;
				case 36: new_cmd = gleffects_twirl; break;
				case 37: new_cmd = gleffects_bulge; break;	
				case 38: new_cmd = gleffects_square; break;			
				case 39: new_cmd = gleffects_heat; break;
				case 40: new_cmd = gleffects_sepia; break;
				case 41: new_cmd = gleffects_xpro; break;	
				case 42: new_cmd = gleffects_lumaxpro; break;
				case 43: new_cmd = gleffects_xray; break;	
				case 44: new_cmd = gleffects_sin; break;	
					
				case 50: new_cmd = movie1; break;	
				case 51: new_cmd = movie2; break;	
				case 52: new_cmd = movie3; break;	
				case 53: new_cmd = movie4; break;	
				case 54: new_cmd = movie5; break;	
				case 55: new_cmd = movie6; break;	
				case 56: new_cmd = movie7; break;	
				case 57: new_cmd = movie8; break;	
				case 58: new_cmd = movie9; break;	
				case 59: new_cmd = movie10; break;	
				case 60: new_cmd = movie11; break;	
				case 61: new_cmd = movie12; break;	
					
				default:
					//skip bad requests by claiming we already did it!
					requested_state = active_state; 
				}
				
				if (active_state != requested_state){
					if (active_state == 2){
						//KILL RASPVID HERE! DO IT NOW
						kill(raspivid_src_process.pid,SIGINT);
						kill(raspivid_sink_process.pid,SIGINT);
						close(temp_fd1);
						close(temp_fd2);
						pclose_noshell2(&raspivid_src_process);
						pclose_noshell2(&raspivid_sink_process);
						raspivid_sink_process.pid = 0;
						raspivid_src_process.pid = 0;
					}
					
					//kill old pieline
					stop_gst_pipeline();
					
					//make new pipeline
					pipeline = gst_parse_launch (new_cmd, NULL);

					//new start pipeline
					gst_element_set_state (pipeline, GST_STATE_PLAYING);
					
					timer_stop();
					
					//mark request as completed
					active_state = requested_state;
				}
			}			
		}
		
		//fps calculations ran every second
		fps++;
		if (fps_counter < millis()){
			printf("POPEN FPS:%d  mis:%d errors:%d\n",fps,missed,errors);
			fps = 0;
			fps_counter += 1000;
			if (fps_counter < millis()){
				fps_counter = millis()+1000;
			}			
		}		
	}

}
