#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <stdexcept>

#include <GLES/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "../png_texture.h"

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define EX 20     // X dimension
#define EY 20     // Y dimension

static GLuint orange_1,red,orange_0,blue_0,blue_1,texture_orange,texture_blue;
float portal_spin = 0;
float portal_background_spin = 0;
float portal_background_fader = 0;
float portal_spin2 = 0;
float closed_fader = 0;
int close_processed = 999; //impossible value to force process on boot

float blank_fader = 0;
int  blank_processed = 999; //impossible value to force process on boot

int lastcolor = -1;
GLfloat texturescroller = 0.0;
static void Normal(GLfloat *n, GLfloat nx, GLfloat ny, GLfloat nz){
	n[0] = nx;
	n[1] = ny;
	n[2] = nz;
}

static void Vertex(GLfloat *v, GLfloat vx, GLfloat vy, GLfloat vz){
	v[0] = vx;
	v[1] = vy;
	v[2] = vz;
}

static void Texcoord(GLfloat *v, GLfloat s, GLfloat t){
	v[0] = s ;
	v[1] = t- texturescroller;
}

/* Borrowed from glut, adapted */
static void draw_torus(GLfloat r, GLfloat R, GLint nsides, GLint rings){
	int i, j;
	GLfloat theta, phi, theta1;
	GLfloat cosTheta, sinTheta;
	GLfloat cosTheta1, sinTheta1;
	GLfloat ringDelta, sideDelta;
	GLfloat varray[100][3], narray[100][3], tarray[100][2];
	int vcount;

	glVertexPointer(3, GL_FLOAT, 0, varray);
	glNormalPointer(GL_FLOAT, 0, narray);
	glTexCoordPointer(2, GL_FLOAT, 0, tarray);

	glEnableClientState(GL_NORMAL_ARRAY);

	ringDelta = 2.0 * M_PI / rings;
	sideDelta = 2.0 * M_PI / nsides;

	theta = 0.0;
	cosTheta = 1.0;
	sinTheta = 0.0;
	for (i = rings - 1; i >= 0; i--) {
		theta1 = theta + ringDelta;
		cosTheta1 = cos(theta1);
		sinTheta1 = sin(theta1);

		vcount = 0; /* glBegin(GL_QUAD_STRIP); */

		phi = 0.0;
		for (j = nsides; j >= 0; j--) {
			GLfloat s0, s1, t;
			GLfloat cosPhi, sinPhi, dist;

			phi += sideDelta;
			cosPhi = cos(phi);
			sinPhi = sin(phi);
			dist = R + r * cosPhi;

			s0 = 20.0 * theta / (2.0 * M_PI);
			s1 = 20.0 * theta1 / (2.0 * M_PI);
			t = 2.0 * phi / (2.0 * M_PI);  //this seems to control texture wrap around the nut

			Normal(narray[vcount], cosTheta1 * cosPhi, -sinTheta1 * cosPhi, sinPhi);
			Texcoord(tarray[vcount], s0, t);
			Vertex(varray[vcount], cosTheta1 * dist, -sinTheta1 * dist, r * sinPhi);
			vcount++;

			Normal(narray[vcount], cosTheta * cosPhi, -sinTheta * cosPhi, sinPhi);
			Texcoord(tarray[vcount], s1, t);
			Vertex(varray[vcount], cosTheta * dist, -sinTheta * dist,  r * sinPhi);
			vcount++;
		}

		/*glEnd();*/
		assert(vcount <= 100);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, vcount);

		theta = theta1;
		cosTheta = cosTheta1;
		sinTheta = sinTheta1;
	}
	glDisableClientState(GL_NORMAL_ARRAY);

}


void model_board_init(void)
{
	red = png_texture_load(ASSET_DIR "/test.png", NULL, NULL);
	//override default of clamp
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	orange_0 = png_texture_load(ASSET_DIR "/orange_0.png", NULL, NULL);
	orange_1 = png_texture_load(ASSET_DIR "/orange_1.png", NULL, NULL);

	blue_0 = png_texture_load(ASSET_DIR "/blue_0.png", NULL, NULL);
	blue_1 = png_texture_load(ASSET_DIR "/blue_1.png", NULL, NULL);
	
	texture_orange = png_texture_load(ASSET_DIR "/orange_portal.png", NULL, NULL);
	texture_blue = png_texture_load(ASSET_DIR "/blue_portal.png", NULL, NULL);
	
	if (red == 0 || texture_orange == 0 || texture_blue == 0 || orange_0 == 0 || orange_1 == 0 || blue_0 == 0 || blue_1 == 0)
	{
		throw std::runtime_error("Loading textures failed.");
	}
	
	//setup texture unit 0 
	glActiveTexture(GL_TEXTURE0);	
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	GLfloat rgba[4] = {1.0,1.0,1.0,0.0};
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, rgba);
	//use the texture's color, no blending
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);
	//glTexEnvi(GL_TEXTURE_ENV, GL_SRC2_RGB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC2_RGB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
	//use the texture's alpha channel
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA); 
	
	//setup texture unit 1 
	glActiveTexture(GL_TEXTURE1);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	//use the texture's color, no blending
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);
	//glTexEnvi(GL_TEXTURE_ENV, GL_SRC2_RGB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC2_RGB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
	//use the texture's alpha channel
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA); 
	
}

GLfloat pts[100];
 GLfloat colors[100];
void model_board_redraw(float * acceleration, float * magnetic_field, int frame)
{	

	//Create the framebuffer and bind it.


	
	
	
	portal_spin += .50;
	if (portal_spin > 360) portal_spin -= 360;
	portal_spin2 += 5;
	if (portal_spin2 > 360) portal_spin2 -= 360;
	
	
	portal_background_spin -= .01;
	if (portal_background_spin < 360) portal_background_spin += 360;

	portal_background_fader += .01;
	if (portal_background_fader > .75)	portal_background_fader = -.75;
	
	if CHECK_BIT(frame,1){
		if (lastcolor != 1){
			blank_fader = -100;
			blank_processed = 1;
			closed_fader = .99;
			close_processed=2;
		} 
	}else{
		if (lastcolor != 2){
			blank_fader = -100;
			blank_processed = 1;
			closed_fader = .99;
			close_processed=2;
		} 
	}

	//this controls making the portal zoom in (shutter)
	if CHECK_BIT(frame,3){
		if (blank_processed !=1){
			blank_fader = -100;
			blank_processed = 1;
		}
	}else{
		if (blank_processed != 2){
			blank_fader = -99.99;
			blank_processed=2;
		}
	}

	//this controls making the portal fade into the backgroud video
	if CHECK_BIT(frame,0){
		
		if (close_processed !=1){
			closed_fader = 1;
			close_processed = 1;
		}
	}else{
		//wait for zoom to finish before fading to background
		if (close_processed != 2 and blank_fader >= -.5){
			closed_fader = .99;
			close_processed=2;
		}
	}
	
	//this controls fadeout speed of the portal
	if (closed_fader > 0 and closed_fader < 1 ){
		closed_fader = closed_fader - 0.007;
		if (closed_fader<0)	closed_fader = 0;
	}
	
	//iris speed
	if( blank_fader < 0 ){
		blank_fader = blank_fader - .18 * blank_fader ;
		if ( blank_fader > 0 ) blank_fader = 0;
	}
	
	//depth checking
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	
	
	GLfloat vertices1[] = {-EX,-EY,-.5,EX,-EY,-.5,-EX,EY,-.5,EX,EY,-.5};
	GLfloat texCoords1[] = {0,0,1,0,0,1,1,1};
	
	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, vertices1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoords1);

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, vertices1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoords1);
	
	glEnable(GL_TEXTURE0);	
	glActiveTexture(GL_TEXTURE0);	
	glEnable(GL_TEXTURE_2D);

	CHECK_BIT(frame,1) ? glBindTexture(GL_TEXTURE_2D, orange_0) : glBindTexture(GL_TEXTURE_2D, blue_0);
	
	
	glEnable(GL_TEXTURE1);
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	
	CHECK_BIT(frame,1) ? glBindTexture(GL_TEXTURE_2D, orange_1) : glBindTexture(GL_TEXTURE_2D, blue_1);
	
	GLfloat rgba2[4] = {1.0,1.0,1.0,(GLfloat)fabs(portal_background_fader)};
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, rgba2);
	
	glColor4f(1.0 ,1.0 ,1.0 ,closed_fader);
	
	glPushMatrix(); //save before rotation
	
	glRotatef(portal_background_spin, 0, 0, 1); //rotate background	
	glDrawArrays(GL_TRIANGLE_STRIP,0, 4);
	
	glPopMatrix();//un-rotate background	
	
	
	glDisable(GL_TEXTURE1);		
	glBindTexture(GL_TEXTURE_2D, 0); 
	
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE0);	
	glBindTexture(GL_TEXTURE_2D, 0); 
	
	
	
	glClientActiveTexture(GL_TEXTURE4);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, vertices1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoords1);
	
	glActiveTexture(GL_TEXTURE4);
	
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	
	glEnable(GL_TEXTURE_2D);

	glColor4f(0,0,0,1);
	// Portal
	CHECK_BIT(frame,2) ? glBindTexture(GL_TEXTURE_2D, texture_orange) : glBindTexture(GL_TEXTURE_2D, texture_blue);


	GLfloat texCoords2[] = {0,0,1,0,0,1,1,1};
	texCoords2[0] = texCoords2[1] = texCoords2[3] =  texCoords2[4] = (blank_fader);
	texCoords2[2]= texCoords2[5] = texCoords2[6] = texCoords2[7] = (1.0 - blank_fader);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoords2);
	
	GLfloat vertices2[] = {-EX,-EY,0,EX,-EY,0,-EX,EY,0,EX,EY,0};
	glVertexPointer(3, GL_FLOAT, 0, vertices2);
	
	glPushMatrix(); //save before rotation
	
	glScalef(2.08,1.17,1);
	glRotatef(portal_spin, 0, 0, 1);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	
	glBindTexture(GL_TEXTURE_2D, red);
	draw_torus(1 , 8 * ( 1 - blank_fader / -100.0), 30, 60);
	
	glPopMatrix();//un-rotate background	
	
	
	glBindTexture(GL_TEXTURE_2D, 0);

	
	glEnableClientState(GL_COLOR_ARRAY);
	
	
	float oscilator = cos(portal_spin2 * M_PI/180.0) *1.5;
	float xtweak =  1.17* sin(portal_spin * M_PI/180.0) *oscilator; 
	float ytweak = 2.08*cos(portal_spin* M_PI/180.0) *oscilator;
	
	float x =  1.17* sin(portal_spin * M_PI/180.0) *7.8; 
	float y = 2.08*cos(portal_spin* M_PI/180.0) *7.8;
	
	glColor4f(1,1,1,1);
	
    
	for (int xx = 0; xx < 20; xx++){
		//shuffle positions
		pts[xx*3+0] = pts[(xx +1)*3+0];
		pts[xx*3+1] = pts[(xx +1)*3+1];
		pts[xx*3+2] = pts[(xx +1)*3+2];
		//shuffle colors
		colors[xx*4+0] = colors[(xx + 1)*4+0];
		colors[xx*4+1] = colors[(xx + 1)*4+1];
		colors[xx*4+2] = colors[(xx + 1)*4+2];
		colors[xx*4+3] = colors[(xx + 1)*4+3];
		//decay colors
		colors[xx*4+3] = MAX(0,colors[xx*4+3]-.04);
	}
	
	//load coordinates
	pts[20*3+0] = y + ytweak;
	pts[20*3+1] = x + xtweak;
	pts[20*3+2] = cos((portal_spin2/2) * M_PI/180.0) *1.5;
	//load color
	colors[20*4+0] = 1.0;
	colors[20*4+1] = 1.0;
	colors[20*4+2] = 1.0;
	colors[20*3+3] = 1.0; 
	
	glColorPointer(4, GL_FLOAT, 0, colors);
	glVertexPointer(3, GL_FLOAT, 0, pts);
	glPointSize(20);
	glEnable(GL_POINT_SMOOTH);
	glDrawArrays(GL_POINTS,0,20);
	
	
	glDisableClientState(GL_COLOR_ARRAY);
	
	texturescroller += .05;
	//disable textures for shutter
	glDisable(GL_TEXTURE4);	
	glBindTexture(GL_TEXTURE_2D, 0); 
	
	// shutter
	if  CHECK_BIT(frame,3){
		glColor4f(0,0,0,1); //blocking black
		lastcolor = -1;
		blank_fader = -100;
		blank_processed = 1;
		closed_fader = 1;
		close_processed = 1;
	}else{
		glColor4f(0,0,0,0); //transparent black
	}
	
	GLfloat vertices3[] = {-EX,-EY,10,EX,-EY,10,-EX,EY,10,EX,EY,10};
	glVertexPointer(3, GL_FLOAT, 0, vertices3);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


	lastcolor = CHECK_BIT(frame,1) ? 1 : 2; //blue : orange
}
