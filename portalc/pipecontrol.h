#ifndef _PIPECONTROL_H 
#define _PIPECONTROL_H

void pipecontrol_setup(int ip);
void pipecontrol_cleanup(void);

void aplay(const char *filename);//filename full path string
void web_output(int mode1, int mode2);//filename full path string

void ahrs_command(int x, int y, int z, int number); //numerical command for what state to be in
void gst_command(int number);
void launch_gst_control(void);
void launch_ahrs_control(void);
//8 and 9 is closed
//1 open a closed blue portal 
//0 open a open blue portal

//6 open a open orange portal
//7 open a closed orange portal

int read_web_pipe(void);

#endif