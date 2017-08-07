#include "gstvideo.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <wiringPi.h>

int main(int argc, char *argv[]) {
	//gstreamer launch codes
	char rpicamsrc22[] = "rpicamsrc keyframe-interval=3 preview=0 ! video/x-h264,width=640,height=420,framerate=40/1,profile=high ! tee name=t ! queue max-size-time=50000000 leaky=upstream ! rtph264pay config-interval=-1 pt=96 ! udpsink host=192.168.1.22 port=9000 sync=false t. ! queue max-size-time=50000000 leaky=upstream ! h264parse ! omxh264dec ! videorate ! videoscale ! video/x-raw,width=400,height=240,framerate=4/1 ! videoflip method=3 ! jpegenc !  multifilesink location=/var/www/html/tmp/snapshot.jpg";
	char rpicamsrc23[] = "rpicamsrc keyframe-interval=3 preview=0 ! video/x-h264,width=640,height=420,framerate=40/1,profile=high ! tee name=t ! queue max-size-time=50000000 leaky=upstream ! rtph264pay config-interval=-1 pt=96 ! udpsink host=192.168.1.23 port=9000 sync=false t. ! queue max-size-time=50000000 leaky=upstream ! h264parse ! omxh264dec ! videorate ! videoscale ! video/x-raw,width=400,height=240,framerate=4/1 ! videoflip method=3 ! jpegenc !  multifilesink location=/var/www/html/tmp/snapshot.jpg";

	char blank[] = "";

	char videotestsrc[] =       "videotestsrc is-live=true ! video/x-raw,width=640,height=420 ! queue ! glimagesink";
	char videotestsrc_cubed[] = "videotestsrc is-live=true ! video/x-raw,width=640,height=420 ! queue ! glupload ! glfiltercube ! glimagesink";

	char libvisual_jess[] = 	"alsasrc device=hw:1 buffer-time=40000 ! queue max-size-time=50000000 leaky=upstream ! libvisual_jess ! video/x-raw,width=320,height=210,framerate=15/1 ! queue max-size-time=100000000 leaky=upstream ! glupload ! glcolorconvert ! glcolorscale ! 'video/x-raw(memory:GLMemory),width=640,height=420' ! glimagesink";
	char libvisual_infinite[] = "alsasrc device=hw:1 buffer-time=40000 ! queue max-size-time=50000000 leaky=upstream ! libvisual_infinite ! video/x-raw,width=320,height=210,framerate=15/1 ! queue max-size-time=100000000 leaky=upstream ! glupload ! glcolorconvert ! glcolorscale ! 'video/x-raw(memory:GLMemory),width=640,height=420' ! glimagesink";
	char libvisual_jakdaw[] = 	"alsasrc device=hw:1 buffer-time=40000 ! queue max-size-time=50000000 leaky=upstream ! libvisual_jakdaw ! video/x-raw,width=320,height=210,framerate=15/1 ! queue max-size-time=100000000 leaky=upstream ! glupload ! glcolorconvert ! glcolorscale ! 'video/x-raw(memory:GLMemory),width=640,height=420' ! glimagesink";
	char libvisual_oinksie[] = 	"alsasrc device=hw:1 buffer-time=40000 ! queue max-size-time=50000000 leaky=upstream ! libvisual_oinksie ! video/x-raw,width=320,height=210,framerate=15/1 ! queue max-size-time=100000000 leaky=upstream ! glupload ! glcolorconvert ! glcolorscale ! 'video/x-raw(memory:GLMemory),width=640,height=420' ! glimagesink";
	char goom[] = 				"alsasrc device=hw:1 buffer-time=40000 ! queue max-size-time=50000000 leaky=upstream ! goom ! video/x-raw,width=320,height=210,framerate=15/1 ! queue max-size-time=100000000 leaky=upstream ! glupload ! glcolorconvert ! glcolorscale ! 'video/x-raw(memory:GLMemory),width=640,height=420' ! glimagesink";
	char goom2k1[] = 			"alsasrc device=hw:1 buffer-time=40000 ! queue max-size-time=50000000 leaky=upstream ! goom2k1 ! video/x-raw,width=320,height=210,framerate=15/1 ! queue max-size-time=100000000 leaky=upstream ! glupload ! glcolorconvert ! glcolorscale ! 'video/x-raw(memory:GLMemory),width=640,height=420' ! glimagesink";
	
	char normal[] =            "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! glimagesink sync=false";
	char glfiltercube[] =      "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! glupload ! glfiltercube ! glimagesink sync=false";
	char gleffects_mirror[] =  "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! glupload ! gleffects_mirror ! glimagesink sync=false";
	char gleffects_squeeze[] = "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! glupload ! gleffects_squeeze ! glimagesink sync=false";
	char gleffects_stretch[] = "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! glupload ! gleffects_stretch ! glimagesink sync=false";
	char gleffects_tunnel[] =  "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! glupload ! gleffects_tunnel ! glimagesink sync=false";
	char gleffects_twirl[] =   "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! glupload ! gleffects_twirl ! glimagesink sync=false";
	char gleffects_bulge[] =   "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! glupload ! gleffects_bulge ! glimagesink sync=false";
	char gleffects_heat[] =    "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! glupload ! gleffects_heat ! glimagesink sync=false";
	char radioactv[] =         "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! videorate ! video/x-raw,framerate=20/1 ! videoconvert ! queue ! radioactv ! glimagesink sync=false";
	char revtv[] =             "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! videorate ! video/x-raw,framerate=20/1 ! videoconvert ! queue ! revtv ! glimagesink sync=false";
	char agingtv[] =           "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! videorate ! video/x-raw,framerate=20/1 ! videoconvert ! queue ! agingtv ! glimagesink sync=false";
	char dicetv[] =            "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! videorate ! video/x-raw,framerate=20/1 ! videoconvert ! queue ! dicetv ! glimagesink sync=false";
	char warptv[] =            "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! videorate ! video/x-raw,framerate=20/1 ! videoconvert ! queue ! warptv ! glimagesink sync=false";
	char shagadelictv[] =      "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! videorate ! video/x-raw,framerate=20/1 ! videoconvert ! queue ! shagadelictv ! glimagesink sync=false";
	char vertigotv[] =         "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! videorate ! video/x-raw,framerate=20/1 ! videoconvert ! queue ! vertigotv ! glimagesink sync=false";
	char rippletv[] =          "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! videorate ! video/x-raw,framerate=20/1 ! videoconvert ! queue ! rippletv ! glimagesink sync=false";
	char edgetv[] =            "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! videorate ! video/x-raw,framerate=20/1 ! videoconvert ! queue ! edgetv ! glimagesink sync=false";
	char streaktv[] =          "udpsrc port=9000 caps='application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264' ! rtph264depay ! h264parse ! omxh264dec ! videorate ! video/x-raw,framerate=20/1 ! videoconvert ! queue ! streaktv ! glimagesink sync=false";
	//redo all movies
	//char movie1[] =  "filesrc location=/home/pi/movies/1.mp4 ";
	//char movie2[] =  "filesrc location=/home/pi/movies/2.mp4 ";
	//char movie3[] =  "filesrc location=/home/pi/movies/3.mp4 ";
	//char movie4[] =  "filesrc location=/home/pi/movies/4.mp4 ";
	//char movie5[] =  "filesrc location=/home/pi/movies/5.mp4 ";
	//char movie6[] =  "filesrc location=/home/pi/movies/6.mp4 ";
	//char movie7[] =  "filesrc location=/home/pi/movies/7.mp4 ";
	//char movie8[] =  "filesrc location=/home/pi/movies/8.mp4 ";
	//char movie9[] =  "filesrc location=/home/pi/movies/9.mp4 ";
	//char movie10[] = "filesrc location=/home/pi/movies/10.mp4";
	//char movie11[] = "filesrc location=/home/pi/movies/11.mp4";
	//char movie12[] = "filesrc location=/home/pi/movies/12.mp4";
	char movie1[] =  "omxplayer --audio_fifo 1 -o hdmi /home/pi/movies/1.mp4";
	char movie2[] =  "omxplayer --audio_fifo 1 -o hdmi /home/pi/movies/2.mp4";
	char movie3[] =  "omxplayer --audio_fifo 1 -o hdmi /home/pi/movies/3.mp4";
	char movie4[] =  "omxplayer --audio_fifo 1 -o hdmi /home/pi/movies/4.mp4";
	char movie5[] =  "omxplayer --audio_fifo 1 -o hdmi /home/pi/movies/5.mp4";
	char movie6[] =  "omxplayer --audio_fifo 1 -o hdmi /home/pi/movies/6.mp4";
	char movie7[] =  "omxplayer --audio_fifo 1 -o hdmi /home/pi/movies/7.mp4";
	char movie8[] =  "omxplayer --audio_fifo 1 -o hdmi /home/pi/movies/8.mp4";
	char movie9[] =  "omxplayer --audio_fifo 1 -o hdmi /home/pi/movies/9.mp4";
	char movie10[] = "omxplayer --audio_fifo 1 -o hdmi /home/pi/movies/10.mp4";
	char movie11[] = "omxplayer --audio_fifo 1 -o hdmi /home/pi/movies/11.mp4";
	char movie12[] = "omxplayer --audio_fifo 1 -o hdmi /home/pi/movies/12.mp4";

	char * new_cmd;
	char * rpicamsrc;
	
	int requested_state = 0;
	int active_state = -1; //force change to state 0 on launch

	

	int changes = 0;
	
	if (argc != 2) {
		fprintf(stderr, "Need IP As Argument (22 or 23)\n");
		exit(1);
	}
	


	//read src and set variables
	int ip = atoi(argv[1]);
	
	if (ip == 23) rpicamsrc = rpicamsrc22;
	else if (ip ==22) rpicamsrc = rpicamsrc23;
	else {
		fprintf(stderr, "Need IP As Argument (22 or 23)\n");
		exit(1);
	}

	//stats
	uint32_t time_start = 0;
	int missed = 0;
	uint32_t time_fps = 0;
	int fps = 0;
	uint32_t time_delay = 0;
	
	//init remote control
	FILE *bash_fp;
	system("pkill gst-launch");
	bash_fp = popen("bash", "w");
	fcntl(fileno(bash_fp), F_SETFL, fcntl(fileno(bash_fp), F_GETFL, 0) | O_NONBLOCK);
	
	//non blocking sdtin read
	fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);
	
	while (1){
		
		
		time_start += 20;
		uint32_t predicted_delay = time_start - millis(); //calc predicted delay
		if (predicted_delay > 20) predicted_delay = 0; //check for overflow
		if (predicted_delay != 0){
			delay(predicted_delay);
			time_delay += predicted_delay;			
		}else{
			time_start = millis(); //reset timer to now
			printf("GST  Skipping Idle...\n");
			missed++;
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
					fprintf(stderr, "GST  Unrecognized input with %d items.\n", result);
				}else{
					requested_state = temp_state;
				}
			}
		}

		if (active_state != requested_state){
			
			// start timer
			struct timeval t1, t2;
			double elapsedTime;
			gettimeofday(&t1, NULL);
			
			printf("GST  Starting Request: %d\n",requested_state);
			
			//figure out the correct app
			switch (requested_state){
				//the basics
			case GST_BLANK: new_cmd = blank; break;
			case GST_VIDEOTESTSRC:	new_cmd = videotestsrc;	break;
			case GST_VIDEOTESTSRC_CUBED: new_cmd = videotestsrc_cubed; break;
			case GST_RPICAMSRC: new_cmd = rpicamsrc; break;
			case GST_NORMAL: new_cmd = normal; break; 	
				
				//libvisual 10 - 18
			case GST_LIBVISUAL_JESS: new_cmd = libvisual_jess; break;	  //good
			case GST_LIBVISUAL_INFINITE: new_cmd = libvisual_infinite; break;	//good
			case GST_LIBVISUAL_JAKDAW: new_cmd = libvisual_jakdaw; break;	//good
			case GST_LIBVISUAL_OINKSIE: new_cmd = libvisual_oinksie; break;	//good		
			case GST_GOOM: new_cmd = goom; break;	//good		
			case GST_GOOM2K1: new_cmd = goom2k1; break;	//good		
				//tv effects	
			case GST_STREAKTV: new_cmd = streaktv; break;//fixed
			case GST_RADIOACTV: new_cmd = radioactv; break;//fixed
			case GST_REVTV: new_cmd = revtv; break;//good
			case GST_AGINGTV: new_cmd = agingtv; break;//steampunk
			case GST_DICETV: new_cmd = dicetv; break;//works
			case GST_WARPTV: new_cmd = warptv; break;//works
			case GST_SHAGADELICTV: new_cmd = shagadelictv; break;//works			
			case GST_VERTIGOTV: new_cmd = vertigotv; break;//works
			case GST_RIPPLETV: new_cmd = rippletv; break;//works
			case GST_EDGETV: new_cmd = edgetv; break;//works					
				//gl effects	
			case GST_GLCUBE: new_cmd = glfiltercube; break;
			case GST_GLMIRROR: new_cmd = gleffects_mirror; break;
			case GST_GLSQUEEZE: new_cmd = gleffects_squeeze; break;
			case GST_GLSTRETCH: new_cmd = gleffects_stretch; break;
			case GST_GLTUNNEL: new_cmd = gleffects_tunnel; break;
			case GST_GLTWIRL: new_cmd = gleffects_twirl; break;
			case GST_GLBULGE: new_cmd = gleffects_bulge; break;	
			case GST_GLHEAT: new_cmd = gleffects_heat; break;

			case GST_MOVIE1: new_cmd = movie1; break;	
			case GST_MOVIE2: new_cmd = movie2; break;	
			case GST_MOVIE3: new_cmd = movie3; break;	
			case GST_MOVIE4: new_cmd = movie4; break;	
			case GST_MOVIE5: new_cmd = movie5; break;	
			case GST_MOVIE6: new_cmd = movie6; break;	
			case GST_MOVIE7: new_cmd = movie7; break;	
			case GST_MOVIE8: new_cmd = movie8; break;	
			case GST_MOVIE9: new_cmd = movie9; break;	
			case GST_MOVIE10: new_cmd = movie10; break;	
			case GST_MOVIE11: new_cmd = movie11; break;	
			case GST_MOVIE12: new_cmd = movie12; break;	
				
			default:
				//skip bad requests by claiming we already did it!
				requested_state = active_state; 
			}
			
			if (active_state != requested_state){

				
				
				
				if (active_state <= GST_MOVIE_LAST && active_state >= GST_MOVIE_FIRST){
					//kill old pieline
					system("sudo pkill omxplayer");
				}
				else{
					//kill old pieline
					system("sudo pkill gst-launch");
				}
				if (requested_state <= GST_MOVIE_LAST && requested_state >= GST_MOVIE_FIRST){
					fprintf(bash_fp, "%s &\n",new_cmd);
				}
				else{
					fprintf(bash_fp, "sudo nice --10 gst-launch-1.0 --gst-disable-registry-update --no-fault %s &\n",new_cmd);
				}
				//make new pipeline
			
				fflush(bash_fp);

				
				//mark request as completed
				active_state = requested_state;
				
				// stop timer
				gettimeofday(&t2, NULL);

				// compute and print the elapsed time in millisec
				elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
				elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
				printf("GST  Lag to enter mode %d: %f ms\n",requested_state,elapsedTime);	
				changes++;
			}
		}
		
		//fps calculations ran every second
		fps++;
		if (time_fps < millis()){
			printf("GST  FPS:%d  mis:%d idle:%d%% changes:%d\n",fps,missed,time_delay/10,changes);
			fps = 0;
			time_delay = 0;
			time_fps += 1000;
			if (time_fps < millis()){
				time_fps = millis()+1000;
			}			
		}		
	}

}
