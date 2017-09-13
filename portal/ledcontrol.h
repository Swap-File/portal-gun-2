#ifndef _LEDCONTROL_H
#define _LEDCONTROL_H

uint8_t led_update(const this_gun_struct& this_gun,const other_gun_struct& other_gun);
void ledcontrol_setup(void);
float ledcontrol_update(int color_temp,int width_temp,int width_speed_temp,int overlay_temp, int total_offset);
void ledcontrol_wipe(void);

#endif
