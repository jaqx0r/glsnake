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

#define NODE_COUNT 24

#define VOFFSET 0.045

#define X_MASK	1
#define Y_MASK	2
#define Z_MASK	4

#define DEF_YSPIN               0.10
#define DEF_ZSPIN               0.14
#define DEF_EXPLODE             0.03
#define DEF_VELOCITY            1.0
#define DEF_ACCEL               0.1
#define DEF_FOV                25.0
#define DEF_STATICTIME       5000L

#ifndef M_SQRT1_2	/* Win32 doesn't have this constant  */
#define M_SQRT1_2 0.70710678118654752440084436210485
#endif

/* the connecting string that holds the snake together */
#define MAGICAL_RED_STRING 0

#define GETSCALAR(vec,mask) ((vec)==(mask) ? 1 : ((vec)==-(mask) ? -1 : 0 ))

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/* the triangular prism what makes up the basic unit */
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

struct nodeang {
    float prevAngle;
    float curAngle;
    float destAngle;
};

/* glsnake state configuration struct
 * keep all state variables in here */
struct glsnake_cfg {
    /* window id */
    int window;

    /* is a morph in progress? */
    int morphing;

    /* has the model been paused? */
    int paused;

    /* snake metrics */
    int is_cyclic;
    int is_legal;
    int last_turn;
    int debug;

    /* the shape of the model */
    struct nodeang node[NODE_COUNT];

    /* currently selected node for interactive mode */
    int selected;

    /* current model, next model */
    int curModel;
    int m;

    /* model morphing */
    float morph_angular_velocity;
    int new_morph;

    /* colours */
    GLfloat colour[2][3];
    GLfloat colour_prev[2][3];

    /* rotation angles */
    float rotang1;
    float rotang2;

    /* timing variables */
    struct timeb last_iteration;
    struct timeb last_morph;

    /* window size */
    int width, height;
    int old_width, old_height;

    /* field of view for zoom */
    float zoom;

    /* the id of the display lists for drawing a node */
    int node_solid, node_wire;

    /* font list number */
    int font;

    /* explode distance */
    float explode;

    /* is the model in wireframe mode? */
    int wireframe;

    /* is the window fullscreen? */
    int fullscreen;

    /* are we in interactive mode? */
    int interactive;

    /* are we drawing model names? */
    int titles;

    /* what colour scheme are we using? */
    int authentic;

    /* trackball stuff */
    float cumquat[4], oldquat[4];
    float rotation[16];
    /* mouse drag vectors: start and end */
    float m_s[3], m_e[3];
    int dragging;
};

static struct glsnake_cfg * glc = NULL;

typedef float (*morphFunc)(long);

float morph(long);
float morph_one_at_a_time(long);

morphFunc morphFuncs[] = {
	morph,
	morph_one_at_a_time
};

/* colour cycling constants */
GLfloat colour_cyclic[2][3]    = { { 0.4,  0.8, 0.2  },
				   { 1.0,  1.0, 1.0  } };
GLfloat colour_normal[2][3]    = { { 0.3,  0.1, 0.9  },
				   { 1.0,  1.0, 1.0  } };
GLfloat colour_invalid[2][3]   = { { 0.5,  0.5, 0.5  },
				   { 1.0,  1.0, 1.0  } };
GLfloat colour_authentic[2][3] = { { 0.38, 0.0, 0.55 },
				   { 0.0,  0.5, 0.34 } };

void morph_colour(float percent);

/* mmm, quaternion arithmetic */
void calc_rotation() {
    double Nq, s;
    double xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;

    /* this bit ripped from Shoemake's quaternion notes from SIGGRAPH */
    Nq = glc->cumquat[0] * glc->cumquat[0] + glc->cumquat[1] * glc->cumquat[1] +
	glc->cumquat[2] * glc->cumquat[2] + glc->cumquat[3] * glc->cumquat[3];
    s = (Nq > 0.0) ? (2.0 / Nq) : 0.0;
    xs = glc->cumquat[0] *  s; ys = glc->cumquat[1] *  s; zs = glc->cumquat[2] * s;
    wx = glc->cumquat[3] * xs; wy = glc->cumquat[3] * ys; wz = glc->cumquat[3] * zs;
    xx = glc->cumquat[0] * xs; xy = glc->cumquat[0] * ys; xz = glc->cumquat[0] * zs;
    yy = glc->cumquat[1] * ys; yz = glc->cumquat[1] * zs; zz = glc->cumquat[2] * zs;

    glc->rotation[0] = 1.0 - (yy + zz);
    glc->rotation[1] = xy + wz;
    glc->rotation[2] = xz - wy;
    glc->rotation[4] = xy - wz;
    glc->rotation[5] = 1.0 - (xx + zz);
    glc->rotation[6] = yz + wx;
    glc->rotation[8] = xz + wy;
    glc->rotation[9] = yz - wx;
    glc->rotation[10] = 1.0 - (xx + yy);
    glc->rotation[3] = glc->rotation[7] = glc->rotation[11] = 0.0;
    glc->rotation[12] = glc->rotation[13] = glc->rotation[14] = 0.0;
    glc->rotation[15] = 1.0;
}

/* wot initialises it */
void glsnake_init(void) {
    float light_pos[][3] = {{0.0, 0.0, 20.0}, {0.0, 20.0, 0.0}};
    float light_dir[][3] = {{0.0, 0.0,-20.0}, {0.0,-20.0, 0.0}};

    /* initialise the config struct */
    glc->selected = 11;
    glc->morph_angular_velocity = DEF_VELOCITY;
    glc->morphing = 0;
    glc->new_morph = 1;
    glc->is_cyclic = 1;
    glc->is_legal = 1;
    glc->last_turn = -1;
    glc->debug = 0;

    glc->curModel = glc->m = 0;

    glc->rotang1 = 0.0;
    glc->rotang2 = 0.0;

    glc->zoom = DEF_FOV;
    glc->explode = VOFFSET;
    glc->wireframe = 0;
    glc->paused = 0;
    glc->fullscreen = 0;
    glc->titles = 1;
    glc->authentic = 0;
    glc->interactive = 0;
    glc->dragging = 0;

    glc->cumquat[0] = 0.0;
    glc->cumquat[1] = 0.0;
    glc->cumquat[2] = 0.0;
    glc->cumquat[3] = 0.0;
    glc->oldquat[0] = 0.0;
    glc->oldquat[1] = 0.0;
    glc->oldquat[2] = 0.0;
    glc->oldquat[3] = 1.0;
    
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
    gluPerspective(glc->zoom, glc->width/(float)glc->height, 0.05, 100.0);
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
    glc->node_solid = glGenLists(1);
    glNewList(glc->node_solid, GL_COMPILE);
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
    glc->node_wire = glGenLists(1);
    glNewList(glc->node_wire, GL_COMPILE);
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
    gluOrtho2D(0, glc->width, 0, glc->height);
    glColor3f(1.0, 1.0, 1.0);
    {
	char interactstr[] = "interactive";
	char * s;
	int i = 0;
	int w;
	
	if (glc->interactive)
	    s = interactstr;
	else
	    s = model[glc->curModel].name;
	w = glutBitmapLength(GLUT_BITMAP_HELVETICA_12, (unsigned char *) s);
	glRasterPos2f(glc->width - w - 3, 4);
	while (s[i] != 0)
	    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, s[i++]);
    }
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

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
void glsnake_display(void) {
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
    glMultMatrixf(glc->rotation);
    
    /* apply the continuous rotation */
    glRotatef(glc->rotang1, 0.0, 1.0, 0.0); 
    glRotatef(glc->rotang2, 0.0, 0.0, 1.0); 
    
    com[0] = 0.0;
    com[1] = 0.0;
    com[2] = 0.0;
    com[3] = 0.0;
    for (i = 0; i < 24; i++) {
	float rotmat[16];

	ang = glc->node[i].curAngle;
	
	glTranslatef(0.5, 0.5, 0.5);		/* move to center */
	glRotatef(90, 0.0, 0.0, -1.0);		/* reorient  */
	glTranslatef(1.0 + glc->explode, 0.0, 0.0);	/* move to new pos. */
	glRotatef(180 + ang, 1.0, 0.0, 0.0);	/* pivot to new angle */
	glTranslatef(-0.5, -0.5, -0.5);		/* return from center */

	glGetFloatv(GL_MODELVIEW_MATRIX, rotmat);

	matmult_origin(rotmat, positions[i]);

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
    glMultMatrixf(glc->rotation);
    
    /* apply the continuous rotation */
    glRotatef(glc->rotang1, 0.0, 1.0, 0.0); 
    glRotatef(glc->rotang2, 0.0, 0.0, 1.0); 

    /* now draw each node along the snake -- this is quite ugly :p */
    for (i = 0; i < 24; i++) {
	/* choose a colour for this node */
	if ((i == glc->selected || i == glc->selected+1) && glc->interactive)
	    /* yellow */
	    glColor3f(1.0, 1.0, 0.0);
	else {
	    if (glc->authentic)
		glColor3fv(colour_authentic[(i+1)%2]);
	    else
		glColor3fv(glc->colour[(i+1)%2]);
	}

	/* draw the node */
	if (glc->wireframe)
	    glCallList(glc->node_wire);
	else
	    glCallList(glc->node_solid);

	/* now work out where to draw the next one */
	
	/* Interpolate between models */
	ang = glc->node[i].curAngle;
	
	glTranslatef(0.5, 0.5, 0.5);		/* move to center */
	glRotatef(90, 0.0, 0.0, -1.0);		/* reorient  */
	glTranslatef(1.0 + glc->explode, 0.0, 0.0);	/* move to new pos. */
	glRotatef(180 + ang, 1.0, 0.0, 0.0);	/* pivot to new angle */
	glTranslatef(-0.5, -0.5, -0.5);		/* return from center */

    }
    glPopMatrix();
    
    if (glc->titles)
	draw_title();
    
    glFlush();
    glutSwapBuffers();
}

/* wot gets called when the winder is resized */
void glsnake_reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //gluLookAt(0.0, 0.0, 20.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    gluPerspective(glc->zoom, w/(float)h, 0.05, 100.0);
    gluLookAt(0.0, 0.0, 20.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    glMatrixMode(GL_MODELVIEW);
    glc->width = w;
    glc->height = h;
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
    
    glc->is_legal = 1;
    x = y = z = 12;
    
    /* trace path of snake - and keep record for is_legal */
    for (i = 0; i < 23; i++) {
	/* establish new state vars */
	srcDir = -prevDstDir;
	x += GETSCALAR(prevDstDir, X_MASK);
	y += GETSCALAR(prevDstDir, Y_MASK);
	z += GETSCALAR(prevDstDir, Z_MASK);

	switch ((int) glc->node[i].destAngle) {
	  case (int) (ZERO):
	    dstDir = -prevSrcDir;
	    break;
	  case (int) (PIN):
	    dstDir = prevSrcDir;
	    break;
	  case (int) (RIGHT):
	  case (int) (LEFT):
	    dstDir = cross_product(prevSrcDir, prevDstDir);
	    if (glc->node[i].destAngle == (int) (RIGHT))
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
	    glc->is_legal = 0;
	
	prevSrcDir = srcDir;
	prevDstDir = dstDir;
    }	
    
    /* determine if the snake is cyclic */
    glc->is_cyclic = (dstDir == Y_MASK && x == 12 && y == 11 && z == 12);
    
    /* determine last_turn */
    glc->last_turn = -1;
    if (glc->is_cyclic)
	switch (srcDir) {
	  case -Z_MASK: glc->last_turn = ZERO; break;
	  case Z_MASK:  glc->last_turn = PIN; break;
	  case X_MASK:  glc->last_turn = LEFT; break;
	  case -X_MASK: glc->last_turn = RIGHT; break;
	}
    
}

void start_colour_morph(void) {
	memcpy(glc->colour_prev, glc->colour, sizeof(glc->colour));
	//morph_colour(0.0);
}

/* Start morph process to this model */
void start_morph(int modelIndex, int immediate) {
    int i;
    float max_angle;
    
    glc->new_morph = 1;
    max_angle = 0.0;
    for (i = 0; i < 23; i++) {
	glc->node[i].prevAngle = glc->node[i].curAngle;
	glc->node[i].destAngle = model[modelIndex].node[i];
	if (immediate)
	    glc->node[i].curAngle = model[modelIndex].node[i];
	if (fabs(glc->node[i].destAngle - glc->node[i].curAngle) > max_angle)
	    max_angle = fabs(glc->node[i].destAngle - glc->node[i].curAngle);
    }
    if (max_angle > 180.0)
	max_angle = 180.0;
    
    calc_snake_metrics();
    
    start_colour_morph();
    
    glc->curModel = modelIndex;
    glc->morphing = 1;
}

void special(int key, int x, int y) {
    int i;
    float *destAngle = &(glc->node[glc->selected].destAngle);
    int unknown_key = 0;
    
    if (glc->interactive) {
	switch (key) {
	  case GLUT_KEY_UP:
	    glc->selected = (glc->selected + 22) % 23;
	    break;
	  case GLUT_KEY_DOWN:
	    glc->selected = (glc->selected + 1) % 23;
	    break;
	  case GLUT_KEY_LEFT:
	    *destAngle = fmod(*destAngle+(LEFT), 360);
	    glc->morphing = glc->new_morph = 1;
	    break;
	  case GLUT_KEY_RIGHT:
	    *destAngle = fmod(*destAngle+(RIGHT), 360);
	    glc->morphing = glc->new_morph = 1;
	    break;
	  case GLUT_KEY_HOME:
	    for (i = 0; i < 24; i++)
		glc->node[i].destAngle = (ZERO);
	    glc->morphing = glc->new_morph = 1;
	    break;
	  default:
	    unknown_key = 1;
	    break;
	}
    }
    calc_snake_metrics();
    //morph_colour(fabs(fmod(node[glc->selected].destAngle - node[glc->selected].curAngle,180))/180.0);
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
	glc->explode += DEF_EXPLODE;
	glutPostRedisplay();
	break;
      case 'E':
	glc->explode -= DEF_EXPLODE;
	if (glc->explode < 0.0) glc->explode = 0.0;
	glutPostRedisplay();
	break;
      case '.':
	/* next model */
	glc->curModel++;
	glc->curModel %= models;
	start_morph(glc->curModel , 0);
	
	/* Reset last_morph time */
	ftime(&glc->last_morph);			
	break;
      case ',':
	/* previous model */
	glc->curModel = (glc->curModel + models - 1) % models;
	start_morph(glc->curModel, 0);
	
	/* Reset glc->last_morph time */
	ftime(&glc->last_morph);			
	break;
      case '+':
	glc->morph_angular_velocity += DEF_ACCEL;
	break;
      case '-':
	if (glc->morph_angular_velocity > DEF_ACCEL)
	    glc->morph_angular_velocity -= DEF_ACCEL;
	break;
      case 'i':
	if (glc->interactive) {
	    /* Reset last_iteration and last_morph time */
	    ftime(&glc->last_iteration);
	    ftime(&glc->last_morph);
	}
	glc->interactive = 1 - glc->interactive;
	glutPostRedisplay();
	break;
      case 'w':
	glc->wireframe = 1 - glc->wireframe;
	if (glc->wireframe)
	    glDisable(GL_LIGHTING);
	else
	    glEnable(GL_LIGHTING);
	glutPostRedisplay();
	break;
      case 'p':
	if (glc->paused) {
	    /* unpausing, reset last_iteration and last_morph time */
	    ftime(&glc->last_iteration);
	    ftime(&glc->last_morph);
	}
	glc->paused = 1 - glc->paused;
	break;
      case 'd':
	/* dump the current model so we can add it! */
	printf("# %s\nnoname:\t", model[glc->curModel].name);
	for (i = 0; i < 24; i++) {
	    if (glc->node[i].curAngle == ZERO)
		printf("Z");
	    else if (glc->node[i].curAngle == LEFT)
		printf("L");
	    else if (glc->node[i].curAngle == PIN)
		printf("P");
	    else if (glc->node[i].curAngle == RIGHT)
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
	glc->fullscreen = 1 - glc->fullscreen;
	if (glc->fullscreen) {
	    glc->old_width = glc->width;
	    glc->old_height = glc->height;
	    glutFullScreen();
	} else {
	    glutReshapeWindow(glc->old_width, glc->old_height);
	    glutPositionWindow(50,50);
	}
	break;
      case 't':
	glc->titles = 1 - glc->titles;
	if (glc->interactive || glc->paused)
	    glutPostRedisplay();
	break;
      case 'a':
	glc->authentic = 1 - glc->authentic;
	break;
      case 'z':
	glc->zoom += 1.0;
	glsnake_reshape(glc->width, glc->height);
	break;
      case 'Z':
	glc->zoom -= 1.0;
	glsnake_reshape(glc->width, glc->height);
	break;
      default:
	break;
    }
}

void mouse(int button, int state, int x, int y) {
    if (button==0) {
	switch (state) {
	  case GLUT_DOWN:
	    glc->dragging = 1;
	    glc->m_s[0] = M_SQRT1_2 * 
		(x - (glc->width / 2.0)) / (glc->width / 2.0);
	    glc->m_s[1] = M_SQRT1_2 * 
		((glc->height / 2.0) - y) / (glc->height / 2.0);
	    glc->m_s[2] = sqrt(1-(glc->m_s[0]*glc->m_s[0]+glc->m_s[1]*glc->m_s[1]));
	    break;
	  case GLUT_UP:
	    glc->dragging = 0;
	    glc->oldquat[0] = glc->cumquat[0];
	    glc->oldquat[1] = glc->cumquat[1];
	    glc->oldquat[2] = glc->cumquat[2];
	    glc->oldquat[3] = glc->cumquat[3];
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
    
    if (glc->dragging) {
	/* construct the motion end vector from the x,y position on the
	 * window */
	glc->m_e[0] = (x - (glc->width/ 2.0)) / (glc->width / 2.0);
	glc->m_e[1] = ((glc->height / 2.0) - y) / (glc->height / 2.0);
	/* calculate the normal of the vector... */
	norm = glc->m_e[0] * glc->m_e[0] + glc->m_e[1] * glc->m_e[1];
	/* check if norm is outside the sphere and wraparound if necessary */
	if (norm > 1.0) {
	    glc->m_e[0] = -glc->m_e[0];
	    glc->m_e[1] = -glc->m_e[1];
	    glc->m_e[2] = sqrt(norm - 1);
	} else {
	    /* the z value comes from projecting onto an elliptical spheroid */
	    glc->m_e[2] = sqrt(1 - norm);
	}

	/* now here, build a quaternion from m_s and m_e */
	q[0] = glc->m_s[1] * glc->m_e[2] - glc->m_s[2] * glc->m_e[1];
	q[1] = glc->m_s[2] * glc->m_e[0] - glc->m_s[0] * glc->m_e[2];
	q[2] = glc->m_s[0] * glc->m_e[1] - glc->m_s[1] * glc->m_e[0];
	q[3] = glc->m_s[0] * glc->m_e[0] + glc->m_s[1] * glc->m_e[1] + glc->m_s[2] * glc->m_e[2];

	/* new rotation is the product of the new one and the old one */
	glc->cumquat[0] = q[3] * glc->oldquat[0] + q[0] * glc->oldquat[3] + 
	    q[1] * glc->oldquat[2] - q[2] * glc->oldquat[1];
	glc->cumquat[1] = q[3] * glc->oldquat[1] + q[1] * glc->oldquat[3] + 
	    q[2] * glc->oldquat[0] - q[0] * glc->oldquat[2];
	glc->cumquat[2] = q[3] * glc->oldquat[2] + q[2] * glc->oldquat[3] + 
	    q[0] * glc->oldquat[1] - q[1] * glc->oldquat[0];
	glc->cumquat[3] = q[3] * glc->oldquat[3] - q[0] * glc->oldquat[0] - 
	    q[1] * glc->oldquat[1] - q[2] * glc->oldquat[2];
	
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

	if (glc->new_morph)
	    glc->new_morph = 0;
	
	iter_angle_max = 90.0 * (glc->morph_angular_velocity/200.0) * iter_msec;
	
	still_morphing = 0;
	largest_diff = largest_progress = 0.0;
	for (i = 0; i < 24; i++) {
		float curAngle = glc->node[i].curAngle;
		float destAngle = glc->node[i].destAngle;
		if (curAngle != destAngle) {
			still_morphing = 1;
			if (fabs(curAngle-destAngle) <= iter_angle_max)
				glc->node[i].curAngle = destAngle;
			else if (fmod(curAngle-destAngle+360,360) > 180)
				glc->node[i].curAngle = fmod(curAngle + iter_angle_max, 360);
			else
				glc->node[i].curAngle = fmod(curAngle+360 - iter_angle_max, 360);
			largest_diff = MAX(largest_diff, fabs(destAngle-glc->node[i].curAngle));
			largest_progress = MAX(largest_diff, fabs(glc->node[i].curAngle - glc->node[i].prevAngle));
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

	if (glc->new_morph) {
		memset(&done, 0, sizeof(done));
		glc->new_morph = 0;
		progress = final = 0.0;
		for (i = 0; i < 24; i++) {
			final += fabs(fmod(glc->node[i].destAngle-glc->node[i].curAngle, 180));
		}
	}
	
	iter_angle_max = 90.0 * (glc->morph_angular_velocity/1000.0) * iter_msec;
	
	still_morphing = iter_done = 0;
	for (i = 0; i < 24; i++) {
		if (iter_done)
			/* Stop after turning one join */
			break;
		
		if (!done[i]) {
			float curAngle = glc->node[i].curAngle;
			float destAngle = glc->node[i].destAngle;
			if (curAngle != destAngle) {
				still_morphing = iter_done = 1;
				if (fabs(curAngle-destAngle) <= iter_angle_max)
					glc->node[i].curAngle = destAngle;
				else if (fmod(curAngle-destAngle+360,360) > 180)
					glc->node[i].curAngle = fmod(curAngle + iter_angle_max, 360);
				else
					glc->node[i].curAngle = fmod(curAngle+360 - iter_angle_max, 360);
				progress += fabs(fmod(glc->node[i].curAngle-curAngle, 180));
			} else {
				done[i] = 1;
			}
		}
	}
	return progress/final;
}

void morph_colour(float percent) {
    GLfloat target[2][3];
    float compct; /* complement of percentage */

    compct = 1.0 - percent;

    if (glc->authentic)
	memcpy(&target, &colour_authentic, sizeof(target));
    else if (!glc->is_legal)
	memcpy(&target, &colour_invalid, sizeof(target));
    else if (glc->is_cyclic)
	memcpy(&target, &colour_cyclic, sizeof(target));
    else
	memcpy(&target, &colour_normal, sizeof(target));

    glc->colour[0][0] = glc->colour_prev[0][0] * compct + target[0][0] * percent;
    glc->colour[0][1] = glc->colour_prev[0][1] * compct + target[0][1] * percent;
    glc->colour[0][2] = glc->colour_prev[0][2] * compct + target[0][2] * percent;

    glc->colour[1][0] = glc->colour_prev[1][0] * compct + target[1][0] * percent;
    glc->colour[1][1] = glc->colour_prev[1][1] * compct + target[1][1] * percent;
    glc->colour[1][2] = glc->colour_prev[1][2] * compct + target[1][2] * percent;
}

void restore_idle(int value);
void quick_sleep(void);

void glsnake_idle(void) {
    /* time since last iteration */
    long iter_msec;
    /* time since the beginning of last morph */
    long morf_msec;
    float morph_progress;
    struct timeb current_time;
    morphFunc transition;
    
    /* Do nothing to the model if we are paused */
    if (glc->paused) {
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
    iter_msec = (long) current_time.millitm - glc->last_iteration.millitm + 
	((long) current_time.time - glc->last_iteration.time) * 1000L;
    if (iter_msec) {
	/* save the current time */
	memcpy(&glc->last_iteration, &current_time, sizeof(struct timeb));
	
	/* work out if we have to switch models */
	morf_msec = glc->last_iteration.millitm - glc->last_morph.millitm +
	    ((long) (glc->last_iteration.time-glc->last_morph.time) * 1000L);
	if ((morf_msec > DEF_STATICTIME) && !glc->interactive && !glc->morphing) {
	    start_morph(rand() % models, 0);
	}
	
	if (glc->interactive && !glc->morphing) {
	    quick_sleep();
	    return;
	}
	
	if (!glc->dragging && !glc->interactive) {
	    glc->rotang1 += 360/((1000/DEF_YSPIN)/iter_msec);
	    glc->rotang2 += 360/((1000/DEF_ZSPIN)/iter_msec);
	}
	//	transition = morphFuncs[rand() % (sizeof(morphFuncs)/sizeof(morphFunc))];

	transition = morph_one_at_a_time;

	morph_progress = MIN(transition(iter_msec), 1.0);
	
	if (morph_progress >= 1.0) {
	    memcpy(&glc->last_morph, &current_time, sizeof(struct timeb));
	    glc->morphing = 0;
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

void restore_idle(int value)
{
    glutIdleFunc(glsnake_idle);
}

void quick_sleep(void)
{
    /* By using glutTimerFunc we can keep responding to 
     * mouse and keyboard events, unlike using something like
     * usleep. */
    glutIdleFunc(NULL);
    glutTimerFunc(1, restore_idle, 0);
}

/* stick anything that needs to be shutdown properly here */
void unmain(void) {
    glutDestroyWindow(glc->window);
    free(glc);
}

int main(int argc, char ** argv) {
    char * basedir;
    int i;
    
    glc = malloc(sizeof(struct glsnake_cfg));

    glc->width = 320;
    glc->height = 240;
    
    glutInit(&argc, argv);
    
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(glc->width, glc->height);
    glc->window = glutCreateWindow("glsnake");
    
    ftime(&glc->last_iteration);
    memcpy(&glc->last_morph, &glc->last_iteration, sizeof(struct timeb));
    srand((unsigned int)glc->last_iteration.time);
    memcpy(&glc->colour, &colour_normal, sizeof(glc->colour));
    memcpy(&glc->colour_prev, &colour_normal, sizeof(glc->colour_prev));
    
    glc->m = rand() % models;
    for (i = 0; i < 24; i++)
	glc->node[i].curAngle = 0.0;
    start_morph(0, 1);	
    
    glutDisplayFunc(glsnake_display);
    glutReshapeFunc(glsnake_reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutIdleFunc(glsnake_idle);

    glsnake_init();
    
    atexit(unmain);
    
    glutSwapBuffers();
    glutMainLoop();
    
    return 0;
}
