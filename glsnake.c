/* $Id: glsnake.c,v 1.56 2003/02/23 09:42:38 jaq Exp $
 * 
 * An OpenGL imitation of Rubik's Snake 
 * (c) 2001 Jamie Wilkinson <jaq@spacepants.org>,
 * Andrew Bennetts <andrew@puzzling.org>, 
 * and Peter Aylett <peter@ylett.com>
 * 
 * based on the Allegro snake.c by Peter Aylett and Andrew Bennetts
 *
 * Compile using a command like:
 *    gcc -O2 -lGL -lGLU -lglut -lm -o glsnake glsnake.c
 *
 * Jamie rewrote all the drawing code for OpenGL, and the trackball interface
 * Andrew fixed up the morphing code
 * Peter added a ton of new models, file input, some OpenGL and snake metrics
 * Mark Assad made it compile under Windows
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <GL/glut.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libgen.h>
#include "models.h"

#define X_MASK	1
#define Y_MASK	2
#define Z_MASK	4

#define ROTATION_RATE1		0.10
#define ROTATION_RATE2		0.14
#define EXPLODE_INCREMENT	0.03
/* time in milliseconds between morphs */
#define MODEL_STATIC_TIME	5000L
#define MORPH_ANG_VELOCITY	1.0
#define MORPH_ANG_ACCEL		0.1

/* the connecting string that holds the snake together */
#define MAGICAL_RED_STRING 0

/* default field of view */
#define FOV 25.0

#define GETSCALAR(vec,mask) ((vec)==(mask) ? 1 : ((vec)==-(mask) ? -1 : 0 ))

#ifndef M_SQRT1_2	/* Win32 doesn't have this constant  */
#define M_SQRT1_2 0.70710678118654752440084436210485
#endif

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/* the id for the window we make */
int window;

/* the id of the display lists for drawing a node */
int node_solid, node_wire;

/* the triangular prism what makes up the basic unit */
#define VOFFSET 0.045
float solid_prism_v[][3] = {
    /* first corner, bottom left front */
    { VOFFSET, VOFFSET, 1.0 },
    { VOFFSET, 0.00, 1.0 - VOFFSET },
    { 0.00, VOFFSET, 1.0 - VOFFSET },
    /* second corner, rear */
    { VOFFSET, VOFFSET, 0.00 },
    { VOFFSET, 0.00, VOFFSET },
    { 0.00, VOFFSET, VOFFSET },
    /* third, right front */
    { 1.0 - VOFFSET / M_SQRT1_2, VOFFSET, 1.0 },
    { 1.0 - VOFFSET / M_SQRT1_2, 0.0, 1.0 - VOFFSET },
    { 1.0 - VOFFSET * M_SQRT1_2, VOFFSET, 1.0 - VOFFSET },
    /* fourth, right rear */
    { 1.0 - VOFFSET / M_SQRT1_2, VOFFSET, 0.0 },
    { 1.0 - VOFFSET / M_SQRT1_2, 0.0, VOFFSET },
    { 1.0 - VOFFSET * M_SQRT1_2, VOFFSET, VOFFSET },
    /* fifth, upper front */
    { VOFFSET, 1.0 - VOFFSET / M_SQRT1_2, 1.0 },
    { VOFFSET / M_SQRT1_2, 1.0 - VOFFSET * M_SQRT1_2, 1.0 - VOFFSET },
    { 0.0, 1.0 - VOFFSET / M_SQRT1_2, 1.0 - VOFFSET},
    /* sixth, upper rear */
    { VOFFSET, 1.0 - VOFFSET / M_SQRT1_2, 0.0 },
    { VOFFSET / M_SQRT1_2, 1.0 - VOFFSET * M_SQRT1_2, VOFFSET },
    { 0.0, 1.0 - VOFFSET / M_SQRT1_2, VOFFSET }};

float solid_prism_n[][3] = {/* corners */
    { -VOFFSET, -VOFFSET, VOFFSET },
    { VOFFSET, -VOFFSET, VOFFSET },
    { -VOFFSET, VOFFSET, VOFFSET },
    { -VOFFSET, -VOFFSET, -VOFFSET },
    { VOFFSET, -VOFFSET, -VOFFSET },
    { -VOFFSET, VOFFSET, -VOFFSET },
    /* edges */
    { -VOFFSET, 0.0, VOFFSET },
    { 0.0, -VOFFSET, VOFFSET },
    { VOFFSET, VOFFSET, VOFFSET },
    { -VOFFSET, 0.0, -VOFFSET },
    { 0.0, -VOFFSET, -VOFFSET },
    { VOFFSET, VOFFSET, -VOFFSET },
    { -VOFFSET, -VOFFSET, 0.0 },
    { VOFFSET, -VOFFSET, 0.0 },
    { -VOFFSET, VOFFSET, 0.0 },
    /* faces */
    { 0.0, 0.0, 1.0 },
    { 0.0, -1.0, 0.0 },
    { M_SQRT1_2, M_SQRT1_2, 0.0 },
    { -1.0, 0.0, 0.0 },
    { 0.0, 0.0, -1.0 }};

float wire_prism_v[][3] = {{ 0.0, 0.0, 1.0 },
			   { 1.0, 0.0, 1.0 },
			   { 0.0, 1.0, 1.0 },
			   { 0.0, 0.0, 0.0 },
			   { 1.0, 0.0, 0.0 },
			   { 0.0, 1.0, 0.0 }};

float wire_prism_n[][3] = {{ 0.0, 0.0, 1.0},
			   { 0.0,-1.0, 0.0},
			   { M_SQRT1_2, M_SQRT1_2, 0.0},
			   {-1.0, 0.0, 0.0},
			   { 0.0, 0.0,-1.0}};

typedef struct {
    float prevAngle;
    float curAngle;
    float destAngle;
} nodeAng;

int selected = 11;

nodeAng node[24];

int m;
int curModel;

/* model morphing */
float morph_angular_velocity = MORPH_ANG_VELOCITY;

typedef float (*morphFunc)(long);

float morph(long);
float morph_one_at_a_time(long);

morphFunc morphFuncs[] = {
	morph,
	morph_one_at_a_time
};

int new_morph = 1;

/* snake metrics */
int is_cyclic = 0;
int is_legal = 1;
int last_turn = -1;
int debug = 0;

/* colour cycling constants */
GLfloat colour_cyclic[2][3]    = { { 0.4,  0.8, 0.2  },
				   { 1.0,  1.0, 1.0  } };
GLfloat colour_normal[2][3]    = { { 0.3,  0.1, 0.9  },
				   { 1.0,  1.0, 1.0  } };
GLfloat colour_invalid[2][3]   = { { 0.5,  0.5, 0.5  },
				   { 1.0,  1.0, 1.0  } };
GLfloat colour_authentic[2][3] = { { 0.38, 0.0, 0.55 },
				   { 0.0,  0.5, 0.34 } };

/* colour variables */
GLfloat colour[2][3];
GLfloat colour_prev[2][3];

void morph_colour(float percent);

/* rotation angle */
float rotang1 = 0.0;
float rotang2 = 0.0;

struct timeb last_iteration;
struct timeb last_morph;

/* window size */
int width, height;
int old_width, old_height;

/* field of view for zoom */
float zoom = FOV;

/* font list number */
int font;

char * interactstr = "interactive";

/* option variables */
float explode = VOFFSET;
int wireframe = 0;
int interactive = 0;
int paused = 0;
int fullscreen = 0;
int titles = 1;
int authentic = 0;

int w = 0;

/* trackball stuff */
float cumquat[4] = {0.0,0.0,0.0,0.0}, oldquat[4] = {0.0,0.0,0.0,1.0};
float rotation[16];
/* mouse drag vectors: start and end */
float m_s[3], m_e[3];
int dragging;

/* mmm, quaternion arithmetic */
void calc_rotation() {
    double Nq, s;
    double xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;

    /* this bit ripped from Shoemake's quaternion notes from SIGGRAPH */
    Nq = cumquat[0] * cumquat[0] + cumquat[1] * cumquat[1] +
	cumquat[2] * cumquat[2] + cumquat[3] * cumquat[3];
    s = (Nq > 0.0) ? (2.0 / Nq) : 0.0;
    xs = cumquat[0] *  s; ys = cumquat[1] *  s; zs = cumquat[2] * s;
    wx = cumquat[3] * xs; wy = cumquat[3] * ys; wz = cumquat[3] * zs;
    xx = cumquat[0] * xs; xy = cumquat[0] * ys; xz = cumquat[0] * zs;
    yy = cumquat[1] * ys; yz = cumquat[1] * zs; zz = cumquat[2] * zs;

    rotation[0] = 1.0 - (yy + zz);
    rotation[1] = xy + wz;
    rotation[2] = xz - wy;
    rotation[4] = xy - wz;
    rotation[5] = 1.0 - (xx + zz);
    rotation[6] = yz + wx;
    rotation[8] = xz + wy;
    rotation[9] = yz - wx;
    rotation[10] = 1.0 - (xx + yy);
    rotation[3] = rotation[7] = rotation[11] = 0.0;
    rotation[12] = rotation[13] = rotation[14] = 0.0;
    rotation[15] = 1.0;
}

/* wot initialises it */
void init(void) {
    float light_pos[][3] = {{0.0, 0.0, 20.0}, {0.0, 20.0, 0.0}};
    float light_dir[][3] = {{0.0, 0.0,-20.0}, {0.0,-20.0, 0.0}};
    
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    
    /* gouraud shadin' */
    glShadeModel(GL_SMOOTH);
	
    /* enable backface culling */
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_NORMALIZE);
	
    /* set up our camera */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(zoom, width/(float)height, 0.05, 100.0);
    glMatrixMode(GL_MODELVIEW);
    gluLookAt(0.0, 0.0, 20.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    glLoadIdentity();
    
    /* set up lighting */
    glColor3f(1.0, 1.0, 1.0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos[0]);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_dir[0]);
    glLightfv(GL_LIGHT1, GL_POSITION, light_pos[1]);
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, light_dir[1]);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_COLOR_MATERIAL);
    
    /* build a solid display list */
    node_solid = glGenLists(1);
    glNewList(node_solid, GL_COMPILE);
    /* corners */
    glBegin(GL_TRIANGLES);
    glNormal3fv(solid_prism_n[0]);
    glVertex3fv(solid_prism_v[0]);
    glVertex3fv(solid_prism_v[2]);
    glVertex3fv(solid_prism_v[1]);
    
    glNormal3fv(solid_prism_n[1]);
    glVertex3fv(solid_prism_v[6]);
    glVertex3fv(solid_prism_v[7]);
    glVertex3fv(solid_prism_v[8]);
    
    glNormal3fv(solid_prism_n[2]);
    glVertex3fv(solid_prism_v[12]);
    glVertex3fv(solid_prism_v[13]);
    glVertex3fv(solid_prism_v[14]);
    
    glNormal3fv(solid_prism_n[3]);
    glVertex3fv(solid_prism_v[3]);
    glVertex3fv(solid_prism_v[4]);
    glVertex3fv(solid_prism_v[5]);
    
    glNormal3fv(solid_prism_n[4]);
    glVertex3fv(solid_prism_v[9]);
    glVertex3fv(solid_prism_v[11]);
    glVertex3fv(solid_prism_v[10]);
    
    glNormal3fv(solid_prism_n[5]);
    glVertex3fv(solid_prism_v[16]);
    glVertex3fv(solid_prism_v[15]);
    glVertex3fv(solid_prism_v[17]);
    glEnd();
    /* edges */
    glBegin(GL_QUADS);
    glNormal3fv(solid_prism_n[6]);
    glVertex3fv(solid_prism_v[0]);
    glVertex3fv(solid_prism_v[12]);
    glVertex3fv(solid_prism_v[14]);
    glVertex3fv(solid_prism_v[2]);
    
    glNormal3fv(solid_prism_n[7]);
    glVertex3fv(solid_prism_v[0]);
    glVertex3fv(solid_prism_v[1]);
    glVertex3fv(solid_prism_v[7]);
    glVertex3fv(solid_prism_v[6]);
    
    glNormal3fv(solid_prism_n[8]);
    glVertex3fv(solid_prism_v[6]);
    glVertex3fv(solid_prism_v[8]);
    glVertex3fv(solid_prism_v[13]);
    glVertex3fv(solid_prism_v[12]);
    
    glNormal3fv(solid_prism_n[9]);
    glVertex3fv(solid_prism_v[3]);
    glVertex3fv(solid_prism_v[5]);
    glVertex3fv(solid_prism_v[17]);
    glVertex3fv(solid_prism_v[15]);
    
    glNormal3fv(solid_prism_n[10]);
    glVertex3fv(solid_prism_v[3]);
    glVertex3fv(solid_prism_v[9]);
    glVertex3fv(solid_prism_v[10]);
    glVertex3fv(solid_prism_v[4]);
    
    glNormal3fv(solid_prism_n[11]);
    glVertex3fv(solid_prism_v[15]);
    glVertex3fv(solid_prism_v[16]);
    glVertex3fv(solid_prism_v[11]);
    glVertex3fv(solid_prism_v[9]);
    
    glNormal3fv(solid_prism_n[12]);
    glVertex3fv(solid_prism_v[1]);
    glVertex3fv(solid_prism_v[2]);
    glVertex3fv(solid_prism_v[5]);
    glVertex3fv(solid_prism_v[4]);
    
    glNormal3fv(solid_prism_n[13]);
    glVertex3fv(solid_prism_v[8]);
    glVertex3fv(solid_prism_v[7]);
    glVertex3fv(solid_prism_v[10]);
    glVertex3fv(solid_prism_v[11]);
    
    glNormal3fv(solid_prism_n[14]);
    glVertex3fv(solid_prism_v[13]);
    glVertex3fv(solid_prism_v[16]);
    glVertex3fv(solid_prism_v[17]);
    glVertex3fv(solid_prism_v[14]);
    glEnd();
    
    /* faces */
    glBegin(GL_TRIANGLES);
    glNormal3fv(solid_prism_n[15]);
    glVertex3fv(solid_prism_v[0]);
    glVertex3fv(solid_prism_v[6]);
    glVertex3fv(solid_prism_v[12]);
    
    glNormal3fv(solid_prism_n[19]);
    glVertex3fv(solid_prism_v[3]);
    glVertex3fv(solid_prism_v[15]);
    glVertex3fv(solid_prism_v[9]);
    glEnd();
    
    glBegin(GL_QUADS);
    glNormal3fv(solid_prism_n[16]);
    glVertex3fv(solid_prism_v[1]);
    glVertex3fv(solid_prism_v[4]);
    glVertex3fv(solid_prism_v[10]);
    glVertex3fv(solid_prism_v[7]);
    
    glNormal3fv(solid_prism_n[17]);
    glVertex3fv(solid_prism_v[8]);
    glVertex3fv(solid_prism_v[11]);
    glVertex3fv(solid_prism_v[16]);
    glVertex3fv(solid_prism_v[13]);
    
    glNormal3fv(solid_prism_n[18]);
    glVertex3fv(solid_prism_v[2]);
    glVertex3fv(solid_prism_v[14]);
    glVertex3fv(solid_prism_v[17]);
    glVertex3fv(solid_prism_v[5]);
    glEnd();
    glEndList();
    
    /* build wire display list */
    node_wire = glGenLists(1);
    glNewList(node_wire, GL_COMPILE);
    glBegin(GL_LINE_STRIP);
    glVertex3fv(wire_prism_v[0]);
    glVertex3fv(wire_prism_v[1]);
    glVertex3fv(wire_prism_v[2]);
    glVertex3fv(wire_prism_v[0]);
    glVertex3fv(wire_prism_v[3]);
    glVertex3fv(wire_prism_v[4]);
    glVertex3fv(wire_prism_v[5]);
    glVertex3fv(wire_prism_v[3]);
    glEnd();
    glBegin(GL_LINES);
    glVertex3fv(wire_prism_v[1]);
    glVertex3fv(wire_prism_v[4]);
    glVertex3fv(wire_prism_v[2]);
    glVertex3fv(wire_prism_v[5]);
    glEnd();
    glEndList();
    
    /* initialise the rotation */
    calc_rotation();
    
}

void draw_title(void) {
    /* draw some text */
    glPushAttrib(GL_TRANSFORM_BIT | GL_ENABLE_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);
    glColor3f(1.0, 1.0, 1.0);
    {
	char * s;
	int i = 0;
	int w;
	
	if (interactive)
	    s = interactstr;
	else
	    s = model[curModel].name;
	w = glutBitmapLength(GLUT_BITMAP_HELVETICA_12, (unsigned char *) s);
	glRasterPos2f(width - w - 3, 4);
	while (s[i] != 0)
	    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, s[i++]);
    }
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

#define DOT() { glBegin(GL_POINTS); glVertex3f(0.0, 0.0, 0.0); glEnd(); }

/* apply the matrix to the origin and stick it in vec */
void matmult_origin(float rotmat[16], float vec[4]) {
#if 1
    vec[0] = 0.5 * rotmat[0] + 0.5 * rotmat[4] + 0.5 * rotmat [8] + 1 * rotmat[12];
    vec[1] = 0.5 * rotmat[1] + 0.5 * rotmat[5] + 0.5 * rotmat [9] + 1 * rotmat[13];
    vec[2] = 0.5 * rotmat[2] + 0.5 * rotmat[6] + 0.5 * rotmat[10] + 1 * rotmat[14];
    vec[3] = 0.5 * rotmat[3] + 0.5 * rotmat[7] + 0.5 * rotmat[11] + 1 * rotmat[15];
#else
    vec[0] = 0 * rotmat [0] + 0 * rotmat [1] + 0 * rotmat [2] + 1 * rotmat [3];
    vec[1] = 0 * rotmat [4] + 0 * rotmat [5] + 0 * rotmat [6] + 1 * rotmat [7];
    vec[2] = 0 * rotmat [8] + 0 * rotmat [9] + 0 * rotmat[10] + 1 * rotmat[11];
    vec[3] = 0 * rotmat[12] + 0 * rotmat[13] + 0 * rotmat[14] + 1 * rotmat[15];
#endif
    vec[0] /= vec[3];
    vec[1] /= vec[3];
    vec[2] /= vec[3];
    vec[3] = 1.0;
}

/* wot draws it */
void display(void) {
    int i;
    float ang;
    float positions[24][4]; /* origin points for each node */
    float com[4]; /* it's the CENTRE of MASS */
    
    /* clear the buffer */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    /* go into the modelview stack */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    /* rotate and translate into snake space */
    /*
    glRotatef(45.0, -5.0, 0.0, 1.0);
    glRotatef(45.0, -5.0, 0.0, 1.0);
    glTranslatef(-0.5, 0.0, 0.5);
    */

    /* rotate the snake around the centre of the first node */
    /* move to the centre of the first piece */
    //glTranslatef(0.5, 0.0, 0.5);

    /* move back to origin */
    //glTranslatef(-0.5, 0.0, -0.5);

    /* get the centre of each node, by moving through the snake and
     * performing the rotations, then grabbing the matrix at each point
     * and applying it to the origin */
    glPushMatrix();

    /* apply the mouse drag rotation */
    glMultMatrixf(rotation);
    
    /* apply the continuous rotation */
    glRotatef(rotang1, 0.0, 1.0, 0.0); 
    glRotatef(rotang2, 0.0, 0.0, 1.0); 
    
    com[0] = 0.0;
    com[1] = 0.0;
    com[2] = 0.0;
    com[3] = 0.0;
    for (i = 0; i < 24; i++) {
	float rotmat[16];

	ang = node[i].curAngle;
	
	glTranslatef(0.5, 0.5, 0.5);		/* move to center */
	glRotatef(90, 0.0, 0.0, -1.0);		/* reorient  */
	glTranslatef(1.0 + explode, 0.0, 0.0);	/* move to new pos. */
	glRotatef(180 + ang, 1.0, 0.0, 0.0);	/* pivot to new angle */
	glTranslatef(-0.5, -0.5, -0.5);		/* return from center */

	glGetFloatv(GL_MODELVIEW_MATRIX, rotmat);

#if 0
	printf("[ %f, %f, %f, %f ]\n", rotmat[0], rotmat[1], rotmat[2], rotmat[3]);
	printf("[ %f, %f, %f, %f ]\n", rotmat[4], rotmat[5], rotmat[6], rotmat[7]);
	printf("[ %f, %f, %f, %f ]\n", rotmat[8], rotmat[9], rotmat[10], rotmat[11]);
	printf("[ %f, %f, %f, %f ]\n", rotmat[12], rotmat[13], rotmat[14], rotmat[15]);

	printf("\n");
#endif
	matmult_origin(rotmat, positions[i]);

	//printf("[ %f, %f, %f, %f ]\n", positions[i][0], positions[i][1], positions[i][2], positions[i][2]);
	com[0] += positions[i][0];
	com[1] += positions[i][1];
	com[2] += positions[i][2];
	com[3] += positions[i][3];
    }
    glPopMatrix();
    com[0] /= 24;
    com[1] /= 24;
    com[2] /= 24;
    com[3] /= 24;

    com[0] /= com[3];
    com[1] /= com[3];
    com[2] /= com[3];

#if 0 /* centre of mass lines */
    //    glDisable(GL_LIGHTING);
    glColor3f(0.0, 1.0, 1.0);
    glBegin(GL_LINES);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(-com[0], -com[1], -com[2]);
    glEnd();
    glColor3f(1.0, 0.0, 1.0);
    glBegin(GL_LINES);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3fv(com);
    glEnd();
    //    glEnable(GL_LIGHTING);
#endif

#ifdef MAGICAL_RED_STRING
    glPushMatrix();
    glTranslatef(-com[0], -com[1], -com[2]);
    //glRotatef(rotang1, 0.0, 1.0, 0.0); 

    //glDisable(GL_LIGHTING);
    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_LINE_STRIP);
    for (i = 0; i < 23; i++) {
	glVertex3fv(positions[i]);
    }
    glEnd();
    //glEnable(GL_LIGHTING);
    glPopMatrix();
#endif

    glPushMatrix();
    glTranslatef(-com[0], -com[1], -com[2]);

    /* apply the mouse drag rotation */
    glMultMatrixf(rotation);
    
    /* apply the continuous rotation */
    glRotatef(rotang1, 0.0, 1.0, 0.0); 
    glRotatef(rotang2, 0.0, 0.0, 1.0); 

    /* now draw each node along the snake -- this is quite ugly :p */
    for (i = 0; i < 24; i++) {
	/* choose a colour for this node */
	if ((i == selected || i == selected+1) && interactive)
	    /* yellow */
	    glColor3f(1.0, 1.0, 0.0);
	else {
	    if (authentic)
		glColor3fv(colour_authentic[(i+1)%2]);
	    else
		glColor3fv(colour[(i+1)%2]);
	}

#if 0
	if (i == 0) glColor3f(0.0, 1.0, 1.0);
#endif
	/* draw the node */
	if (wireframe)
	    glCallList(node_wire);
	else
	    glCallList(node_solid);

	/* now work out where to draw the next one */
	
	/* Interpolate between models */
	ang = node[i].curAngle;
	
	glTranslatef(0.5, 0.5, 0.5);		/* move to center */
	glRotatef(90, 0.0, 0.0, -1.0);		/* reorient  */
	glTranslatef(1.0 + explode, 0.0, 0.0);	/* move to new pos. */
	glRotatef(180 + ang, 1.0, 0.0, 0.0);	/* pivot to new angle */
	glTranslatef(-0.5, -0.5, -0.5);		/* return from center */

    }
    glPopMatrix();
    
    if (titles)
	draw_title();
    
    glFlush();
    glutSwapBuffers();
}

/* wot gets called when the winder is resized */
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //gluLookAt(0.0, 0.0, 20.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    gluPerspective(zoom, w/(float)h, 0.05, 100.0);
    gluLookAt(0.0, 0.0, 20.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    glMatrixMode(GL_MODELVIEW);
    width = w;
    height = h;
}

/* Returns the new dst_dir for the given src_dir and dst_dir */
int cross_product(src_dir, dst_dir) {
    return X_MASK*(GETSCALAR(src_dir,Y_MASK) * GETSCALAR(dst_dir,Z_MASK) -
		   GETSCALAR(src_dir,Z_MASK) * GETSCALAR(dst_dir,Y_MASK))+ 
	Y_MASK*(GETSCALAR(src_dir,Z_MASK) * GETSCALAR(dst_dir,X_MASK) -
		GETSCALAR(src_dir,X_MASK) * GETSCALAR(dst_dir,Z_MASK))+ 
	Z_MASK*(GETSCALAR(src_dir,X_MASK) * GETSCALAR(dst_dir,Y_MASK) -
		GETSCALAR(src_dir,Y_MASK) * GETSCALAR(dst_dir,X_MASK));
}

/* calculate orthogonal snake metrics
 *  is_legal  = true if model does not pass through itself
 *  is_cyclic = true if last node connects back to first node
 *  last_turn = for cyclic snakes, specifes what the 24th turn would be
 */
void calc_snake_metrics() {
    int srcDir, dstDir;
    int i, x, y, z;
    int prevSrcDir = -Y_MASK;
    int prevDstDir = Z_MASK;
    int grid[25][25][25];
    
    /* zero the grid */
    memset(&grid, 0, sizeof(int) * 25*25*25);
    
    is_legal = 1;
    x = y = z = 12;
    
    /* trace path of snake - and keep record for is_legal */
    for (i = 0; i < 23; i++) {
	/* establish new state vars */
	srcDir = -prevDstDir;
	x += GETSCALAR(prevDstDir, X_MASK);
	y += GETSCALAR(prevDstDir, Y_MASK);
	z += GETSCALAR(prevDstDir, Z_MASK);

	switch ((int) node[i].destAngle) {
	  case (int) (ZERO * 90.0):
	    dstDir = -prevSrcDir;
	    break;
	  case (int) (PIN * 90.0):
	    dstDir = prevSrcDir;
	    break;
	  case (int) (RIGHT * 90.0):
	  case (int) (LEFT * 90.0):
	    dstDir = cross_product(prevSrcDir, prevDstDir);
	    if (node[i].destAngle == (int) (RIGHT * 90.0))
		dstDir = -dstDir;
	    break;
	  default:
	    /* Prevent spurious "might be used 
	     * uninitialised" warnings when compiling
	     * with -O2 */
	    dstDir = 0;
	    break;
	}
	
	if (grid[x][y][z] == 0)
	    grid[x][y][z] = srcDir + dstDir;
	else if (grid[x][y][z] + srcDir + dstDir == 0)
	    grid[x][y][z] = 8;
	else
	    is_legal = 0;
	
	prevSrcDir = srcDir;
	prevDstDir = dstDir;
    }	
    
    /* determine if the snake is cyclic */
    is_cyclic = (dstDir == Y_MASK && x == 12 && y == 11 && z == 12);
    
    /* determine last_turn */
    last_turn = -1;
    if (is_cyclic)
	switch (srcDir) {
	  case -Z_MASK: last_turn = ZERO  * 90.0; break;
	  case Z_MASK:  last_turn = PIN   * 90.0; break;
	  case X_MASK:  last_turn = LEFT  * 90.0; break;
	  case -X_MASK: last_turn = RIGHT * 90.0; break;
	}
    
}

/* Is a morph currently in progress? */
int morphing = 0;

void start_colour_morph(void) {
	memcpy(colour_prev, colour, sizeof(colour));
	//morph_colour(0.0);
}

/* Start morph process to this model */
void start_morph(int modelIndex, int immediate) {
    int i;
    float max_angle;
    
    new_morph = 1;
    max_angle = 0.0;
    for (i = 0; i < 23; i++) {
	node[i].prevAngle = node[i].curAngle;
	node[i].destAngle = model[modelIndex].node[i];
	if (immediate)
	    node[i].curAngle = model[modelIndex].node[i];
	if (fabs(node[i].destAngle - node[i].curAngle) > max_angle)
	    max_angle = fabs(node[i].destAngle - node[i].curAngle);
    }
    if (max_angle > 180.0)
	max_angle = 180.0;
    
    calc_snake_metrics();
    
    start_colour_morph();
    
    curModel = modelIndex;
    morphing = 1;
}

void special(int key, int x, int y) {
    int i;
    float *destAngle = &(node[selected].destAngle);
    int unknown_key = 0;
    
    if (interactive) {
	switch (key) {
	  case GLUT_KEY_UP:
	    selected = (selected + 22) % 23;
	    break;
	  case GLUT_KEY_DOWN:
	    selected = (selected + 1) % 23;
	    break;
	  case GLUT_KEY_LEFT:
	    *destAngle = fmod(*destAngle+(LEFT*90.0), 360);
	    morphing = new_morph = 1;
	    break;
	  case GLUT_KEY_RIGHT:
	    *destAngle = fmod(*destAngle+(RIGHT*90.0), 360);
	    morphing = new_morph = 1;
	    break;
	  case GLUT_KEY_HOME:
	    for (i = 0; i < 24; i++)
		node[i].destAngle = (ZERO*90.0);
	    morphing = new_morph = 1;
	    break;
	  default:
	    unknown_key = 1;
	    break;
	}
    }
    calc_snake_metrics();
    //morph_colour(fabs(fmod(node[selected].destAngle - node[selected].curAngle,180))/180.0);
    if (!unknown_key)
	glutPostRedisplay();
}

void keyboard(unsigned char c, int x, int y) {
    int i;
    
    switch (c) {
      case 27:  /* ESC */
      case 'q':
	exit(0);
	break;
      case 'e':
	explode += EXPLODE_INCREMENT;
	glutPostRedisplay();
	break;
      case 'E':
	explode -= EXPLODE_INCREMENT;
	if (explode < 0.0) explode = 0.0;
	glutPostRedisplay();
	break;
      case '.':
	/* next model */
	curModel++;
	curModel %= models;
	start_morph(curModel , 0);
	
	/* Reset last_morph time */
	ftime(&last_morph);			
	break;
      case ',':
	/* previous model */
	curModel = (curModel + models - 1) % models;
	start_morph(curModel, 0);
	
	/* Reset last_morph time */
	ftime(&last_morph);			
	break;
      case '+':
	morph_angular_velocity += MORPH_ANG_ACCEL;
	break;
      case '-':
	if (morph_angular_velocity > MORPH_ANG_ACCEL)
	    morph_angular_velocity -= MORPH_ANG_ACCEL;
	break;
      case 'i':
	if (interactive) {
	    /* Reset last_iteration and last_morph time */
	    ftime(&last_iteration);
	    ftime(&last_morph);
	}
	interactive = 1 - interactive;
	glutPostRedisplay();
	break;
      case 'w':
	wireframe = 1 - wireframe;
	if (wireframe)
	    glDisable(GL_LIGHTING);
	else
	    glEnable(GL_LIGHTING);
	glutPostRedisplay();
	break;
      case 'p':
	if (paused) {
	    /* unpausing, reset last_iteration and last_morph time */
	    ftime(&last_iteration);
	    ftime(&last_morph);
	}
	paused = 1 - paused;
	break;
      case 'd':
	/* dump the current model so we can add it! */
	printf("# %s\nnoname:\t", model[curModel].name);
	for (i = 0; i < 24; i++) {
	    if (node[i].curAngle == ZERO*90.0)
		printf("Z");
	    else if (node[i].curAngle == LEFT*90.0)
		printf("L");
	    else if (node[i].curAngle == PIN*90.0)
		printf("P");
	    else if (node[i].curAngle == RIGHT*90.0)
		printf("R");
	    /*
	      else
	      printf("%f", node[i].curAngle);
	    */
	    if (i < 23)
		printf(" ");
	}
	printf("\n");
	break;
      case 'f':
	fullscreen = 1 - fullscreen;
	if (fullscreen) {
	    old_width = width;
	    old_height = height;
	    glutFullScreen();
	} else {
	    glutReshapeWindow(old_width, old_height);
	    glutPositionWindow(50,50);
	}
	break;
      case 't':
	titles = 1 - titles;
	if (interactive || paused)
	    glutPostRedisplay();
	break;
      case 'a':
	authentic = 1 - authentic;
	break;
      case 'z':
	zoom += 1.0;
	reshape(width, height);
	break;
      case 'Z':
	zoom -= 1.0;
	reshape(width, height);
	break;
      default:
	break;
    }
}

void mouse(int button, int state, int x, int y) {
    if (button==0) {
	switch (state) {
	  case GLUT_DOWN:
	    dragging = 1;
	    m_s[0] = M_SQRT1_2 * 
		(x - (width / 2.0)) / (width / 2.0);
	    m_s[1] = M_SQRT1_2 * 
		((height / 2.0) - y) / (height / 2.0);
	    m_s[2] = sqrt(1-(m_s[0]*m_s[0]+m_s[1]*m_s[1]));
	    break;
	  case GLUT_UP:
	    dragging = 0;
	    oldquat[0] = cumquat[0];
	    oldquat[1] = cumquat[1];
	    oldquat[2] = cumquat[2];
	    oldquat[3] = cumquat[3];
	    break;
	  default:
	    break;
	}
    }
    glutPostRedisplay();
}

void motion(int x, int y) {
    double norm;
    float q[4];
    
    if (dragging) {
	/* construct the motion end vector from the x,y position on the
	 * window */
	m_e[0] = (x - (width/ 2.0)) / (width / 2.0);
	m_e[1] = ((height / 2.0) - y) / (height / 2.0);
	/* calculate the normal of the vector... */
	norm = m_e[0] * m_e[0] + m_e[1] * m_e[1];
	/* check if norm is outside the sphere and wraparound if necessary */
	if (norm > 1.0) {
	    m_e[0] = -m_e[0];
	    m_e[1] = -m_e[1];
	    m_e[2] = sqrt(norm - 1);
	} else {
	    /* the z value comes from projecting onto an elliptical spheroid */
	    m_e[2] = sqrt(1 - norm);
	}

	/* now here, build a quaternion from m_s and m_e */
	q[0] = m_s[1] * m_e[2] - m_s[2] * m_e[1];
	q[1] = m_s[2] * m_e[0] - m_s[0] * m_e[2];
	q[2] = m_s[0] * m_e[1] - m_s[1] * m_e[0];
	q[3] = m_s[0] * m_e[0] + m_s[1] * m_e[1] + m_s[2] * m_e[2];

	/* new rotation is the product of the new one and the old one */
	cumquat[0] = q[3] * oldquat[0] + q[0] * oldquat[3] + 
	    q[1] * oldquat[2] - q[2] * oldquat[1];
	cumquat[1] = q[3] * oldquat[1] + q[1] * oldquat[3] + 
	    q[2] * oldquat[0] - q[0] * oldquat[2];
	cumquat[2] = q[3] * oldquat[2] + q[2] * oldquat[3] + 
	    q[0] * oldquat[1] - q[1] * oldquat[0];
	cumquat[3] = q[3] * oldquat[3] - q[0] * oldquat[0] - 
	    q[1] * oldquat[1] - q[2] * oldquat[2];
	
	calc_rotation();
    }
	
    glutPostRedisplay();
}

/* Returns morph progress */
float morph(long iter_msec) {
	/* work out the maximum angle for this iteration */
	int still_morphing;
	float iter_angle_max, largest_diff, largest_progress;
	int i;

	if (new_morph)
	    new_morph = 0;
	
	iter_angle_max = 90.0 * (morph_angular_velocity/200.0) * iter_msec;
	
	still_morphing = 0;
	largest_diff = largest_progress = 0.0;
	for (i = 0; i < 24; i++) {
		float curAngle = node[i].curAngle;
		float destAngle = node[i].destAngle;
		if (curAngle != destAngle) {
			still_morphing = 1;
			if (fabs(curAngle-destAngle) <= iter_angle_max)
				node[i].curAngle = destAngle;
			else if (fmod(curAngle-destAngle+360,360) > 180)
				node[i].curAngle = fmod(curAngle + iter_angle_max, 360);
			else
				node[i].curAngle = fmod(curAngle+360 - iter_angle_max, 360);
			largest_diff = MAX(largest_diff, fabs(destAngle-node[i].curAngle));
			largest_progress = MAX(largest_diff, fabs(node[i].curAngle - node[i].prevAngle));
		}
	}
	
	return MIN(largest_diff / largest_progress, 1.0);
}

float morph_one_at_a_time(long iter_msec) {
	/* work out the maximum angle for this iteration */
	int still_morphing, iter_done;
	float iter_angle_max;
	unsigned int i;
	static int done[24-1] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	static float progress, final;

	if (new_morph) {
		memset(&done, 0, sizeof(done));
		new_morph = 0;
		progress = final = 0.0;
		for (i = 0; i < 24; i++) {
			final += fabs(fmod(node[i].destAngle-node[i].curAngle, 180));
		}
	}
	
	iter_angle_max = 90.0 * (morph_angular_velocity/1000.0) * iter_msec;
	
	still_morphing = iter_done = 0;
	for (i = 0; i < 24; i++) {
		if (iter_done)
			/* Stop after turning one join */
			break;
		
		if (!done[i]) {
			float curAngle = node[i].curAngle;
			float destAngle = node[i].destAngle;
			if (curAngle != destAngle) {
				still_morphing = iter_done = 1;
				if (fabs(curAngle-destAngle) <= iter_angle_max)
					node[i].curAngle = destAngle;
				else if (fmod(curAngle-destAngle+360,360) > 180)
					node[i].curAngle = fmod(curAngle + iter_angle_max, 360);
				else
					node[i].curAngle = fmod(curAngle+360 - iter_angle_max, 360);
				progress += fabs(fmod(node[i].curAngle-curAngle, 180));
			} else {
				done[i] = 1;
			}
		}
	}

	printf("progress = %0.3f, final = %0.3f\n", progress, final);
	return progress/final;
}

void morph_colour(float percent) {
    GLfloat target[2][3];
    float compct; /* complement of percentage */

    compct = 1.0 - percent;

    if (authentic)
	memcpy(&target, &colour_authentic, sizeof(target));
    else if (!is_legal)
	memcpy(&target, &colour_invalid, sizeof(target));
    else if (is_cyclic)
	memcpy(&target, &colour_cyclic, sizeof(target));
    else
	memcpy(&target, &colour_normal, sizeof(target));

    colour[0][0] = colour_prev[0][0] * compct + target[0][0] * percent;
    colour[0][1] = colour_prev[0][1] * compct + target[0][1] * percent;
    colour[0][2] = colour_prev[0][2] * compct + target[0][2] * percent;

    colour[1][0] = colour_prev[1][0] * compct + target[1][0] * percent;
    colour[1][1] = colour_prev[1][1] * compct + target[1][1] * percent;
    colour[1][2] = colour_prev[1][2] * compct + target[1][2] * percent;
}

void restore_idol(int value);
void quick_sleep(void);

/* "jwz?  no way man, he's my idle" */
void idol(void) {
    /* time since last iteration */
    long iter_msec;
    /* time since the beginning of last morph */
    long morf_msec;
    float morph_progress;
    struct timeb current_time;
    morphFunc transition;
    
    /* Do nothing to the model if we are paused */
    if (paused) {
	/* Avoid busy waiting when nothing is changing */
	quick_sleep();
	glutSwapBuffers();
	glutPostRedisplay();
	return;
    }

    /* ftime is winDOS compatible */
    ftime(&current_time);
    
    /* <spiv> Well, ftime gives time with millisecond resolution.
     * <spiv> (or worse, perhaps... who knows what the OS will do)
     * <spiv> So if no discernable amount of time has passed:
     * <spiv>   a) There's no point updating the screen, because
     *             it would be the same
     * <spiv>   b) The code will divide by zero
     * <Jaq> i.e. if current time is exactly equal to last iteration,
     *       then don't do this block
     */
    iter_msec = (long) current_time.millitm - last_iteration.millitm + 
	((long) current_time.time - last_iteration.time) * 1000L;
    if (iter_msec) {
	/* save the current time */
	memcpy(&last_iteration, &current_time, sizeof(struct timeb));
	
	/* work out if we have to switch models */
	morf_msec = last_iteration.millitm - last_morph.millitm +
	    ((long) (last_iteration.time-last_morph.time) * 1000L);
	if ((morf_msec > MODEL_STATIC_TIME) && !interactive && !morphing) {
	    start_morph(rand() % models, 0);
	}
	
	if (interactive && !morphing) {
	    quick_sleep();
	    return;
	}
	
	if (!dragging && !interactive) {
	    rotang1 += 360/((1000/ROTATION_RATE1)/iter_msec);
	    rotang2 += 360/((1000/ROTATION_RATE2)/iter_msec);
	}
	//	transition = morphFuncs[rand() % (sizeof(morphFuncs)/sizeof(morphFunc))];

	transition = morph_one_at_a_time;

	morph_progress = MIN(transition(iter_msec), 1.0);
	
	if (morph_progress >= 1.0) {
	    memcpy(&last_morph, &current_time, sizeof(struct timeb));
	    morphing = 0;
	}
	
	morph_colour(morph_progress);
	
	glutSwapBuffers();
	glutPostRedisplay();
    } else {
	/* We are going too fast, so we may as well let the 
	 * cpu relax a little by sleeping for a millisecond. */
	quick_sleep();
    }
}

void restore_idol(int value)
{
    glutIdleFunc(idol);
}

void quick_sleep(void)
{
    /* By using glutTimerFunc we can keep responding to 
     * mouse and keyboard events, unlike using something like
     * usleep. */
    glutIdleFunc(NULL);
    glutTimerFunc(1, restore_idol, 0);
}

/* stick anything that needs to be shutdown properly here */
void unmain(void) {
    glutDestroyWindow(window);
}

int main(int argc, char ** argv) {
    char * basedir;
    int i;
    
    width = 320;
    height = 240;
    
    glutInit(&argc, argv);
    
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(width, height);
    window = glutCreateWindow("glsnake");
    
    ftime(&last_iteration);
    memcpy(&last_morph, &last_iteration, sizeof(struct timeb));
    srand((unsigned int)last_iteration.time);
    memcpy(&colour, &colour_normal, sizeof(colour));
    memcpy(&colour_prev, &colour_normal, sizeof(colour_prev));
    
    m = rand() % models;
    for (i = 0; i < 24; i++)
	    node[i].curAngle = 0.0;
    start_morph(0, 1);	
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutIdleFunc(idol);
    
    init();
    
    atexit(unmain);
    
    glutSwapBuffers();
    glutMainLoop();
    
    return 0;
}
