#include "portal.h"
#include <stdio.h>

void local_state_engine(int button, this_gun_struct& this_gun, other_gun_struct& other_gun){	

	//check for expiration of other gun
	if (this_gun.clock - other_gun.last_seen > GUN_EXPIRE) {
		if (this_gun.connected != false){
			this_gun.connected = false;
			printf("\nGun Expired\n");	
		}
		other_gun.state = 0;
	}
	else {
		if(this_gun.connected == false){
			this_gun.connected = true;
			printf("\nGun Connected\n");
		}
	}
		
	//button event transitions
	if( button == BUTTON_ORANGE_LONG || button == BUTTON_BLUE_LONG){
		this_gun.shared_state = 0; //reset local state
		this_gun.initiator = false; //reset initiator
		this_gun.private_state = 0; //reset self state
	}
	else if (button == BUTTON_ORANGE_SHORT){
		if(this_gun.shared_state == 0 && this_gun.private_state == 0){
			this_gun.shared_state = 1;
		}else if(this_gun.shared_state == 1){
			this_gun.shared_state = 2;
			this_gun.initiator = true; 
		}else if(this_gun.shared_state == 2 && this_gun.initiator == true){
			this_gun.shared_state = 3;
		}else if(this_gun.shared_state == 4){
			this_gun.shared_state = 5;
		}else if(this_gun.shared_state == 5){
			this_gun.shared_state = 4;  
		}else if(this_gun.shared_state == -4){ //swap places
			this_gun.shared_state = 4;
		}else if(this_gun.shared_state == 2 && this_gun.initiator == false){  //answer an incoming call immediately and open portal on button press
			this_gun.shared_state = 4;  
		}else if(this_gun.private_state > 0 && this_gun.private_state < 3){
			this_gun.private_state++;
		}			
	}
	else if (button == BUTTON_BLUE_SHORT){
		if (this_gun.shared_state ==0 && this_gun.private_state == 0){
			this_gun.shared_state = -1;
		}else if (this_gun.shared_state == -1){
			this_gun.shared_state = -2;
			this_gun.initiator = true;
		}else if (this_gun.shared_state == -2 && this_gun.initiator == true){
			this_gun.shared_state = -3;
		}else if (this_gun.shared_state == -2 && this_gun.initiator == false){
			this_gun.shared_state = -4;
		}else if (this_gun.shared_state == -3 && this_gun.initiator == false){
			this_gun.shared_state = -4; //connection established
		}else if(this_gun.private_state < 0 && this_gun.private_state > -3){
			this_gun.private_state--;
		}
	}
	else if (button == BUTTON_BOTH_LONG_ORANGE){
		if ((this_gun.shared_state == 0 && this_gun.private_state == 0) || this_gun.shared_state == 1 || this_gun.shared_state == 2 || this_gun.shared_state == 3){
			this_gun.private_state = this_gun.shared_state + 1;
			this_gun.shared_state_previous = this_gun.shared_state = 0;  //avoid transition changes
		}else if(this_gun.private_state == 1 ||  this_gun.private_state == 2 || this_gun.private_state == 3){
			this_gun.private_state++;
		}else if(this_gun.private_state == -3 || this_gun.private_state == -4 || this_gun.private_state == -5 || this_gun.private_state == 4 || this_gun.private_state == 5){
			this_gun.private_state=3;
		}			
	}
	else if (button == BUTTON_BOTH_LONG_BLUE){
		if ((this_gun.shared_state == 0 && this_gun.private_state == 0) || this_gun.shared_state == -1 || this_gun.shared_state == -2 || this_gun.shared_state == -3){
			this_gun.private_state = this_gun.shared_state - 1;  //do this outside of MAX macro
			this_gun.private_state = MAX(this_gun.private_state,-3); //blue needs to clamp to -3 for visual consistency since we dont have a blue portal in local state -3 mode
			this_gun.shared_state_previous = this_gun.shared_state = 0;  //avoid transition changes
		}else if(this_gun.private_state ==-1 || this_gun.private_state == -2 || this_gun.private_state == -3){
			this_gun.private_state--;
		}else if(this_gun.private_state == 3 || this_gun.private_state == 4  || this_gun.private_state == -4){
			this_gun.private_state= -3;
		}
	}
	
	//other gun transitions
	if (this_gun.private_state == 0){
		if ((other_gun.state_previous >= 2 || other_gun.state_previous <= -2)  && other_gun.state == 0){
			this_gun.shared_state = 0;
		}
		if (other_gun.state_previous != -2 && other_gun.state == -2 && this_gun.initiator == false){
			this_gun.shared_state = 2;
		}
		if (other_gun.state_previous != 2 && other_gun.state == 2 && this_gun.initiator == false){
			this_gun.shared_state = -2;
		}
		if (other_gun.state_previous != 3 && other_gun.state == 3 && this_gun.initiator == false){
			this_gun.shared_state = -3;
		}
		if (other_gun.state_previous != -3 && other_gun.state == -3 && this_gun.initiator == false){
			this_gun.shared_state = 2;
		}	
		if (other_gun.state_previous != -4 && other_gun.state == -4 && this_gun.initiator == true){
			this_gun.shared_state = 4;
			this_gun.initiator = false;
		}
		if (other_gun.state_previous != 4 && other_gun.state == 4 && this_gun.initiator == true){
			this_gun.shared_state = -4;
			this_gun.initiator = false;
		}
		if (other_gun.state_previous == -4 && other_gun.state >= 4 ){
			this_gun.shared_state = -4;
			this_gun.initiator = false;
		}	
	}else{
		//code to pull out of self state
		if ((other_gun.state_previous != other_gun.state)&& (other_gun.state <= -2)){
			if (this_gun.private_state <= -3 || this_gun.private_state >= 3){
				this_gun.shared_state = 4;
			}else{
				this_gun.shared_state = 2;
			}
			this_gun.private_state = 0;
		}
	}
	
	//reset playlists to the start
	if (button == BUTTON_BLUE_LONG || button == BUTTON_ORANGE_LONG){
		this_gun.private_effect = this_gun.private_playlist[0];
		this_gun.shared_effect = this_gun.shared_playlist[0];
		if (this_gun.private_effect <= -1) this_gun.private_effect = GST_VIDEOTESTSRC;
		if (this_gun.shared_effect <= -1) this_gun.shared_effect = GST_VIDEOTESTSRC;
		this_gun.shared_playlist_index = 1;
		this_gun.private_playlist_index = 1;	
		//special case of playlist 1 item long
		if (this_gun.private_playlist[this_gun.private_playlist_index] <= -1) this_gun.private_playlist_index = 0;
		if (this_gun.shared_playlist[this_gun.shared_playlist_index] <= -1) this_gun.shared_playlist_index = 0;
	}
	
	//load next shared playlist item
	if (this_gun.shared_state == 5 && this_gun.shared_state_previous == 4){ 
		this_gun.shared_effect = this_gun.shared_playlist[this_gun.shared_playlist_index];
		this_gun.shared_playlist_index++;
		if (this_gun.shared_playlist[this_gun.shared_playlist_index] <= -1) this_gun.shared_playlist_index = 0;
		if (this_gun.shared_playlist_index >= SHARED_PLAYLIST_SIZE) this_gun.shared_playlist_index = 0;
	}
	
	//load next private playlist item
	if ((this_gun.private_state == 3 && this_gun.private_state_previous == 4) || (this_gun.private_state == -3 && this_gun.private_state_previous == -4)) { 
		this_gun.private_effect = this_gun.private_playlist[this_gun.private_playlist_index];
		this_gun.private_playlist_index++;
		if (this_gun.private_playlist[this_gun.private_playlist_index] <= -1) this_gun.private_playlist_index = 0;
		if (this_gun.private_playlist_index >= PRIVATE_PLAYLIST_SIZE) this_gun.private_playlist_index = 0;
	}
}
