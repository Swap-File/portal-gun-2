#include "portal.h"
#include "ledcontrol.h" 

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

#define LED_STRIP_LENGTH 20
#define BREATHING_PERIOD 2 
#define EFFECT_RESOLUTION 400
#define BREATHING_RATE 2000

const uint8_t ending[2] = {0x00,0x00};

int spi_handle; 

uint8_t blank[2*LED_STRIP_LENGTH*3]; 
uint8_t raw_buffer[LED_STRIP_LENGTH*3];
uint8_t effect_buffer[LED_STRIP_LENGTH*3]; 
int timearray[LED_STRIP_LENGTH];

int color1 = 0; 
int color2 = -1; 
int previous_color1 = 0;
uint8_t overlay = 0x80;
bool overlay_primer = true;
bool overlay_enabled = false;
int overlay_timer;

int timeoffset=0;
int offset_target_time = 0 ;

int led_index = 0;
int led_width_actual = 0;
int led_width_requested = 0	;
int color_update_index = 0;

int total_offset_previous = 0;
int width_speed = 200; //.2 seconds
int cooldown_time = 0; 

float effect_array[EFFECT_RESOLUTION];
int ticks_since_overlay_enable = 128; //disabled overlay on bootup
//int shift_speed_last_update;
int width_speed_last_update = 0;
uint8_t brightnesslookup[128][EFFECT_RESOLUTION];

uint8_t led_update(const this_gun_struct& this_gun,const other_gun_struct& other_gun){
	
	int color1 = -1;  //default color of white for shutdown state
	//set color from state data		
	if (this_gun.shared_state > 0 || this_gun.private_state > 0)		color1 = 20;
	else if(this_gun.shared_state < 0 || this_gun.private_state < 0)	color1 = 240;
	
	int width_request = 20;
	//set width
	if(this_gun.shared_state == 1 || this_gun.private_state == -1 || this_gun.private_state == 1 || this_gun.shared_state == -3 ){
		width_request = 10;
	}else if(this_gun.shared_state == -1)	width_request = 1;	
	else if(this_gun.shared_state == -2)	width_request = 5;	
	
	int width_speed = 200;
	//set width speed
	if (this_gun.shared_state <= -4 || this_gun.shared_state >= 4 || this_gun.private_state <= -4 || this_gun.private_state>= 4 ){
		width_speed = 0;
	}
	
	int shutdown_effect = 0;
	//shutdown_effect
	if (this_gun.shared_state == 0 && this_gun.private_state == 0) shutdown_effect = 1;
	
	uint32_t total_time_offset;
	if (this_gun.connected) {
		total_time_offset = (this_gun.clock >> 1) + (other_gun.clock >> 1);  //average the two values
	}else{
		total_time_offset = this_gun.clock;
	}
	total_time_offset = int((float)(total_time_offset % BREATHING_RATE) * ((float)EFFECT_RESOLUTION)/((float)BREATHING_RATE));
	
	return 255 * ledcontrol_update(color1,width_request,width_speed,shutdown_effect,total_time_offset);
}

//red and blue swapped
void Wheel(int WheelPos, uint8_t *b, uint8_t *r, uint8_t *g){
	
	if (WheelPos >= 0){
		switch(WheelPos / 128)
		{
		case 0:
			*r = (127 - WheelPos % 128) ;   //Red down
			*g = (WheelPos % 128);      // Green up
			*b = 0;                  //blue off
			break; 
		case 1:
			*g = (127 - WheelPos % 128);  //green down
			*b =( WheelPos % 128) ;      //blue up
			*r = 0;                  //red off
			break; 
		case 2:
			*b = (127 - WheelPos % 128);  //blue down 
			*r = (WheelPos % 128 );      //red up
			*g = 0;                  //green off
			break; 
		case 3:
			*r = 42;*g = 42;*b = 42;
			break; 
		case 4:
			*r = 127;*g = 127;*b = 127;
			break; 
		}
	}else{
		*r = 0;*g = 0;*b = 0;
	}
	return;
}

void ledcontrol_setup(void) {
	
	for (int i = 0; i < 2*LED_STRIP_LENGTH*3; i++) blank[i] = 0x80;
	
	overlay_timer =  millis();
	printf("LED_Control : Building Lookup Table...\n");
	for ( int i = 0; i < EFFECT_RESOLUTION; i++ ) { 
		//add pi/2 to put max value (1) at start of range
		effect_array[i] =  ((exp(sin( M_PI/2  +(float(i)/(EFFECT_RESOLUTION/BREATHING_PERIOD)*M_PI))) )/ (M_E));
		//printf( "Y: %f\n", effect_array[i]);
	}
	

	for ( int x = 0; x < 128; x++ ) { 
		//printf( "X: %d\n", x);
		for ( int y = 0; y < EFFECT_RESOLUTION; y++ ) { 
			brightnesslookup[x][y] = int(float(x * effect_array[y])) | 0x80;
			//printf( "Y: %d\n", brightnesslookup[x][y]);
		}
	}
	
	printf("LED_Control : Starting SPI...\n");
	/* Open SPI device */
	if( wiringPiSPISetup (1, 16000000) == -1)
	{
		printf("Could not start SPI\n");
		return;
	}
	
	spi_handle = wiringPiSPIGetFd(1);
	
	width_speed_last_update = millis();
}

float ledcontrol_update(int color_temp,int width_temp,int width_speed_temp,int overlay_temp, int  total_offset ) {

	if (total_offset_previous > 200 && total_offset < 200 && led_width_actual == 20){
		//printf("RESET!\n");
		led_index = 0;
	}
	
	total_offset_previous = (total_offset_previous + 4) % 400;  //expected approximate change per cycle
	
	if (abs(total_offset - total_offset_previous) > 8){ //this gives it a bit more leeway  50 FPS, 2 seconds per offset, offset cycle of 400. 400/100 = 4
		//find shortest route 
		if((( total_offset - total_offset_previous + 400) % 400) < 200){  // add to total_offset_previous to reach total_offset
			total_offset_previous = ((total_offset_previous + 8)  % 400 ) ;
			//printf("this: %d previous %d adding to catchup!\n", total_offset, total_offset_previous);
		}else {// subtract from total_offset_previous to reach total_offset
			total_offset_previous = (total_offset_previous - 8 + 400) % 400;
			//printf("this: %d previous %d subbing to catchup!\n", total_offset, total_offset_previous);
		}		   
		total_offset = total_offset_previous; // use old value
	}
	//at 50 FPS, and 2 second effect time, and 20 LEDs, every 2 seconds we do 3 rotations
	//if we are at full width, reset the index to line it up

	total_offset_previous = total_offset;
	
	int time_this_cycle = millis();
	
	if (overlay_temp == 0){
		overlay_primer = true;
		overlay_enabled=false;
		overlay = 0x80;
	}else{
		if( overlay_primer == true && overlay_enabled== false){
			overlay = 0xFF | 0x80;
			overlay_enabled= true;
			overlay_primer = false;
			overlay_timer = time_this_cycle;
		}
	}
	
	if (overlay_primer == true){
		
		if ((color_temp >= -1) && (color_temp < 1024)){
			color1 = color_temp;
		}
		
		if ((width_temp <= 20) && (width_temp >= 0)){
			led_width_requested = width_temp;
		}
		
		if (width_speed_temp >=0){
			width_speed = width_speed_temp;
		}
	}

	
	//on a color change, or coming back from zero width, go full bright on fill complete
	if( previous_color1 != color1 || ((led_width_requested !=0 && led_width_actual == 0 ) && overlay_enabled == false)){
		//adjust time offset, aiming to hit max brightness as color fill completes
		//offset to half of resolution for 50 FPS
		timeoffset =   (300 -  total_offset ) ;
		cooldown_time = time_this_cycle;
		color_update_index = 0;
		previous_color1 = color1;
	}
	
	//tweak breathing rate to attempt to keep in sync across network
	//only tweaks by 1 each cycle to be unnoticeable
	//only tweak when overlay not enabled

	if (time_this_cycle - cooldown_time > 1000){ 
		for ( int i = 0; i < LED_STRIP_LENGTH; i++ ){
			if(timearray[i] < 0){  // add to total_offset_previous to reach total_offset
				timearray[i] = timearray[i] + 1;
			}else if(timearray[i] > 0) {// subtract from total_offset_previous to reach total_offset
				timearray[i] = timearray[i] - 1;
			}	
		}
		
		if(timeoffset < 0){  // add to total_offset_previous to reach total_offset
			timeoffset = timeoffset + 1;
			//printf("Subbing to timearray! %d \n",timeoffset);
		}else if(timeoffset > 0){// subtract from total_offset_previous to reach total_offset
			timeoffset = timeoffset - 1;
			//printf("Adding to timearray! %d \n",timeoffset);
		}	

	}

	if (overlay_enabled == true){
		
		ticks_since_overlay_enable = time_this_cycle - overlay_timer;
		
		//printf("curtime %d\n", ticks_since_overlay_enable);
		
		if (ticks_since_overlay_enable > 127){
			//overlay is now done, disable overlay
			//blank the buffer, and set all variables to 0
			overlay_enabled = false;
			led_width_requested = 0;
			led_width_actual = 0 ;
			color_update_index = 20;
			color1 = -1;
			previous_color1 = color1;
			for ( int i = 0; i < LED_STRIP_LENGTH; i++ ){
				raw_buffer[3*i+0] = 0x00;
				raw_buffer[3*i+1] = 0x00;
				raw_buffer[3*i+2] = 0x00;
				timearray[i] = timeoffset;
			}
			overlay = 0x80;
			
		}else{	
			//during the overlay ramp up linearly, gives white output
			overlay = ticks_since_overlay_enable | 0x80;
		}
	}
	
	//Update the color1 and time of lit LEDs
	if (color_update_index < led_width_actual){
		timearray[color_update_index] = timeoffset;
		Wheel(color1,&raw_buffer[3*color_update_index+0],&raw_buffer[3*color_update_index+1],&raw_buffer[3*color_update_index+2]);
		color_update_index = color_update_index + 1;
	}
	
	//width stuff
	if (time_this_cycle - width_speed_last_update > width_speed){
		//Narrow the lit section by blanking the index LED and incrementing the index.
		//Don't change time data for color2 LEDs
		if (led_width_requested < led_width_actual && led_width_requested >= 0){
			//printf("smaller\n" );
			
			int location = 3*(led_width_actual -1);
			Wheel(color2, &raw_buffer[location+0], &raw_buffer[location+1], &raw_buffer[location+2] );

			//timearray[led_width_actual] = timeoffset;
			
			led_width_actual = led_width_actual -1;
		}
		//Widen the lit section by copying the index LED color1 and time data and decrementing the index.
		else if( led_width_requested > led_width_actual  && led_width_requested <= 20){
			//printf("bigger\n" );
			
			int location = 3*(led_width_actual);

			if (led_width_actual == 0  ){
				//starting an empty array
				Wheel(color1, &raw_buffer[0],&raw_buffer[1] , &raw_buffer[2] );
				timearray[0] = timeoffset;
			}else{
				raw_buffer[location+0] = raw_buffer[location-3];
				raw_buffer[location+1] = raw_buffer[location-2];
				raw_buffer[location+2] = raw_buffer[location-1];
				timearray[led_width_actual] = timearray[led_width_actual - 1];
			}
			led_width_actual = led_width_actual + 1;
			
			//supress color update code
			color_update_index = color_update_index +1;
		}
		width_speed_last_update = time_this_cycle;
	}
	
	//printf("curtime %d\n", ((total_offset + timearray[0]) % EFFECT_RESOLUTION));
	for ( int i = 0; i < LED_STRIP_LENGTH; i++ ) {
		int curtime = 0;
		//dont apply brightness correction to first LED
		if (i != led_width_actual-1){
			curtime = (total_offset + timearray[i] + EFFECT_RESOLUTION) % EFFECT_RESOLUTION;
		}
		int current_location = (i + led_index) % LED_STRIP_LENGTH;

		effect_buffer[current_location*3 + 0] = brightnesslookup[raw_buffer[i*3 + 0]][curtime] | overlay;
		effect_buffer[current_location*3 + 1] = brightnesslookup[raw_buffer[i*3 + 1]][curtime] | overlay;
		effect_buffer[current_location*3 + 2] = brightnesslookup[raw_buffer[i*3 + 2]][curtime] | overlay;
	}
	
	write(spi_handle,effect_buffer,sizeof(effect_buffer));
	write(spi_handle,effect_buffer,sizeof(effect_buffer));
	write(spi_handle,ending,sizeof(ending));

	//Shift array one LED forward and update index
	led_index = (led_index + 1) % LED_STRIP_LENGTH;

	return effect_array[(total_offset + timeoffset) % EFFECT_RESOLUTION];
}

void ledcontrol_wipe(void){
	write(spi_handle,blank,sizeof(blank));
	write(spi_handle,ending,sizeof(ending));
}