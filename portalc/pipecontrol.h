#ifndef _PIPECONTROL_H 
#define _PIPECONTROL_H

void pipecontrol_setup(void);
void pipecontrol_cleanup(void);


void aplay(const char *filename);//filename full path string

void ahrs_command(int number); //numerical command for what state to be in

void gst_command(int number);

//8 and 9 is closed
//1 open a closed blue portal 
//0 open a open blue portal

//6 open a open orange portal
//7 open a closed orange portal




#endif