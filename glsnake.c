/* $Id: glsnake.c,v 1.22 2001/10/06 01:32:07 jaq Exp $
 * 
 * An OpenGL imitation of Rubik's Snake 
 * (c) 2001 Jamie Wilkinson <jaq@spacepants.org>,
 * Andrew Bennetts <andrew@puzzling.org>, 
 * and Peter Aylett <peter@ylett.com>
 * 
 * based on the Allegro snake.c by Peter Aylett and Andrew Bennetts
 *
 * Jamie rewrote all the drawing code for OpenGL, and the trackball interface
 * Andrew fixed up the morphing code
 * Peter added a ton of new models, and the snake metrics
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

#include <GL/glut.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* angles */
#define ZERO	0.0
#define RIGHT   270.0
#define PIN     180.0
#define LEFT    90.0

#define X_MASK	1
#define Y_MASK	2
#define Z_MASK	4

#define ROTATION_RATE1		0.10
#define ROTATION_RATE2		0.14
#define EXPLODE_INCREMENT	0.05
/* time in milliseconds between morphs */
#define MODEL_STATIC_TIME	5000L
#define MORPH_ANG_VELOCITY	1.0
#define MORPH_ANG_ACCEL		0.1

#define GETSCALAR(vec,mask) ((vec) == (mask) ? 1 : ( (vec) == -(mask) ? -1 : 0 ))

/* the id for the window we make */
int window;

/* the id of the display lists for drawing a node */
int node_solid, node_wire, node_shiny;

/* the triangular prism what makes up the basic unit */
float prism_v[][3] = {{ 0.0, 0.0, 1.0 },
                      { 1.0, 0.0, 1.0 },
                      { 0.0, 1.0, 1.0 },
                      { 0.0, 0.0, 0.0 },
                      { 1.0, 0.0, 0.0 },
                      { 0.0, 1.0, 0.0 }};

/* face normals */
float prism_n[][3] = {{ 0.0, 0.0, 1.0},
                      { 0.0,-1.0, 0.0},
                      { M_SQRT1_2, M_SQRT1_2, 0.0},
                      {-1.0, 0.0, 0.0},
                      { 0.0, 0.0,-1.0}};

/* the actual models -- all with 24 nodes (23 joints) */

float ball[] = { RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT,
	RIGHT, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT,
	LEFT, RIGHT, LEFT };

float half_balls[] = { LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT,
	LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT,
	RIGHT, LEFT, LEFT, LEFT };

float cat[] = { ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN, PIN,
	ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO };

float zigzag1[] = { RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT,
	LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, RIGHT,
	RIGHT, LEFT, LEFT };

float zigzag2[] = { PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN,
	ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN};

float zigzag3[] = { PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN,
	LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN };

float caterpillar[] = { RIGHT, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN,
	LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN,
	LEFT, LEFT };

float bow[] = { RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT,
	LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT,
	RIGHT, LEFT, LEFT };

float snowflake[] = { RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT,
	RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT, RIGHT, RIGHT,
	RIGHT, RIGHT, LEFT, LEFT, LEFT};

float turtle[] = { ZERO, RIGHT, LEFT, ZERO, ZERO, RIGHT, LEFT, PIN, RIGHT,
	RIGHT, LEFT, RIGHT, LEFT, LEFT, PIN, LEFT, LEFT, LEFT, RIGHT, LEFT,
	RIGHT, RIGHT, RIGHT };

float basket[] = { RIGHT, PIN, ZERO, ZERO, PIN, LEFT, ZERO, LEFT, LEFT, ZERO,
	LEFT, PIN, ZERO, ZERO, PIN, RIGHT, PIN, LEFT, PIN, ZERO, ZERO, PIN,
	LEFT };

float thing[] = { PIN, RIGHT, LEFT, RIGHT, RIGHT, LEFT, PIN, LEFT, RIGHT, LEFT,
	LEFT, RIGHT, PIN, RIGHT, LEFT, RIGHT, RIGHT, LEFT, PIN, LEFT, RIGHT,
	LEFT, LEFT };

float straight[] = { ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO,
	ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO,
	ZERO, ZERO, ZERO };

float propellor[] = { ZERO, ZERO, ZERO, RIGHT, LEFT, RIGHT, ZERO, LEFT, ZERO,
	ZERO, ZERO, RIGHT, LEFT, RIGHT, ZERO, LEFT, ZERO, ZERO, ZERO, RIGHT,
	LEFT, RIGHT, ZERO, LEFT };

float hexagon[] = { ZERO, ZERO, ZERO, ZERO, LEFT, ZERO, ZERO, RIGHT, ZERO,
	ZERO, ZERO, ZERO, LEFT, ZERO, ZERO, RIGHT, ZERO, ZERO, ZERO, ZERO,
	LEFT, ZERO, ZERO, RIGHT };

float tri1[] = { ZERO, ZERO, LEFT, RIGHT, ZERO, LEFT, ZERO, RIGHT,
	ZERO, ZERO, LEFT, RIGHT, ZERO, LEFT, ZERO, RIGHT,
	ZERO, ZERO, LEFT, RIGHT, ZERO, LEFT, ZERO, RIGHT };

float triangle[] = {ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, LEFT, RIGHT, ZERO,
	ZERO, ZERO, ZERO, ZERO, ZERO, LEFT, RIGHT, ZERO, ZERO, ZERO, ZERO,
	ZERO, ZERO, LEFT, RIGHT };

float flower[] = { ZERO, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, LEFT, PIN, RIGHT,
	RIGHT, PIN, ZERO, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, LEFT, PIN, RIGHT,
	RIGHT, PIN };

float crucifix[] = { ZERO, PIN, PIN, ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN,
	ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO, ZERO,
	ZERO, PIN };

float kayak[] = { PIN, RIGHT, LEFT, PIN, LEFT, PIN, ZERO, ZERO, RIGHT, PIN,
	LEFT, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO,
	PIN, RIGHT };

float bird[] = { ZERO, ZERO, ZERO, ZERO, RIGHT, RIGHT, ZERO, LEFT, PIN, RIGHT,
	ZERO, RIGHT, ZERO, RIGHT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, LEFT,
	ZERO, PIN };   

float seal[] = { RIGHT, LEFT, LEFT, PIN, RIGHT, LEFT, ZERO, PIN, PIN, ZERO,
	LEFT, ZERO, LEFT, PIN, RIGHT, ZERO, LEFT, LEFT, LEFT, PIN, RIGHT,
	RIGHT, LEFT };

float dog[] = { ZERO, ZERO, ZERO, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO, PIN,
	ZERO, PIN, PIN, ZERO, ZERO, ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN };

float frog[] = { RIGHT, RIGHT, LEFT, LEFT, RIGHT, PIN, RIGHT, PIN, LEFT, PIN,
	RIGHT, ZERO, LEFT, ZERO, LEFT, PIN, RIGHT, ZERO, LEFT, LEFT, RIGHT,
	LEFT, LEFT};

float quavers[] = { LEFT,LEFT,RIGHT,LEFT,RIGHT,RIGHT,ZERO,ZERO,ZERO,RIGHT,ZERO,
	ZERO,LEFT,RIGHT,ZERO,ZERO,ZERO,LEFT,LEFT,RIGHT,LEFT,RIGHT,RIGHT };

float fly[] = { LEFT,LEFT,RIGHT,LEFT,RIGHT,RIGHT,ZERO,PIN,ZERO,ZERO,LEFT,PIN,RIGHT,
	ZERO,ZERO,PIN,ZERO,LEFT,LEFT,RIGHT,LEFT,RIGHT,RIGHT };

float puppy[] = { ZERO,PIN,ZERO,PIN,PIN,ZERO,PIN,PIN,ZERO,ZERO,ZERO,RIGHT,
	RIGHT,PIN,RIGHT,LEFT,PIN,LEFT,RIGHT,PIN,RIGHT,LEFT };

float stars[] = {LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, 
	ZERO, ZERO, ZERO, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT };

float mountains[] = { RIGHT,PIN,RIGHT,PIN,RIGHT,PIN,
			LEFT,PIN,LEFT,PIN,LEFT,PIN,
			RIGHT,PIN,RIGHT,PIN,RIGHT,PIN,
			LEFT,PIN,LEFT,PIN,LEFT,PIN };

float quad1[] = { RIGHT,PIN,RIGHT,RIGHT,RIGHT,PIN,
			LEFT,LEFT,LEFT,PIN,LEFT,PIN,
			RIGHT,PIN,RIGHT,RIGHT,RIGHT,PIN,
			LEFT,LEFT,LEFT,PIN,LEFT,PIN };

float quad2[] = { ZERO,PIN,RIGHT,RIGHT,RIGHT,PIN,
			LEFT,LEFT,LEFT,PIN,ZERO,PIN,
			ZERO,PIN,RIGHT,RIGHT,RIGHT,PIN,
			LEFT,LEFT,LEFT,PIN,ZERO,PIN };

float glasses[] = { ZERO,PIN,ZERO,RIGHT,RIGHT,PIN,
			LEFT,LEFT,ZERO,PIN,ZERO,PIN,
			ZERO,PIN,ZERO,RIGHT,RIGHT,PIN,
			LEFT,LEFT,ZERO,PIN,ZERO,PIN };

float em[] = { ZERO,PIN,ZERO,ZERO,RIGHT,PIN,
			LEFT,ZERO,ZERO,PIN,ZERO,PIN,
			ZERO,PIN,ZERO,ZERO,RIGHT,PIN,
			LEFT,ZERO,ZERO,PIN,ZERO,PIN };

float quad3[] = { ZERO,RIGHT,ZERO,ZERO,RIGHT,PIN,
			LEFT,ZERO,ZERO,LEFT,ZERO,PIN,
			ZERO,RIGHT,ZERO,ZERO,RIGHT,PIN,
			LEFT,ZERO,ZERO,LEFT,ZERO,PIN };

float vee[] = { ZERO,ZERO,ZERO,ZERO,RIGHT,PIN,
			LEFT,ZERO,ZERO,ZERO,ZERO,PIN,
			ZERO,ZERO,ZERO,ZERO,RIGHT,PIN,
			LEFT,ZERO,ZERO,ZERO,ZERO,PIN };

float square[] = { ZERO,ZERO,ZERO,RIGHT,RIGHT,PIN,
			LEFT,LEFT,ZERO,ZERO,ZERO,PIN,
			ZERO,ZERO,ZERO,RIGHT,RIGHT,PIN,
			LEFT,LEFT,ZERO,ZERO,ZERO,PIN };

float eagle[] = { RIGHT,ZERO,ZERO,RIGHT,RIGHT,PIN,
			LEFT,LEFT,ZERO,ZERO,LEFT,PIN,
			RIGHT,ZERO,ZERO,RIGHT,RIGHT,PIN,
			LEFT,LEFT,ZERO,ZERO,LEFT,PIN };

float volcano[] = { RIGHT,ZERO,LEFT,RIGHT,RIGHT,PIN,
			LEFT,LEFT,RIGHT,ZERO,LEFT,PIN,
			RIGHT,ZERO,LEFT,RIGHT,RIGHT,PIN,
			LEFT,LEFT,RIGHT,ZERO,LEFT,PIN };

float saddle[] = { RIGHT,ZERO,LEFT,ZERO,RIGHT,PIN,
			LEFT,ZERO,RIGHT,ZERO,LEFT,PIN,
			RIGHT,ZERO,LEFT,ZERO,RIGHT,PIN,
			LEFT,ZERO,RIGHT,ZERO,LEFT,PIN };

float c3d[] = { ZERO,ZERO,RIGHT,ZERO,ZERO,PIN,
				ZERO,ZERO,LEFT,ZERO,ZERO,PIN,
				ZERO,ZERO,RIGHT,ZERO,ZERO,PIN,
				ZERO,ZERO,LEFT,ZERO,ZERO,PIN };

float block[] = { ZERO,ZERO,PIN,PIN,ZERO,RIGHT,PIN,LEFT,PIN,RIGHT,PIN,
RIGHT,PIN,LEFT,PIN,RIGHT,ZERO,ZERO,PIN,ZERO,ZERO,LEFT,PIN,RIGHT };

float duck[] = { LEFT, PIN, LEFT, PIN, ZERO, PIN, PIN, ZERO, PIN, 
	ZERO, LEFT, PIN, RIGHT, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO,
	LEFT, PIN, LEFT };

float prayer[] = { RIGHT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, ZERO, ZERO,
	ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, ZERO, RIGHT, RIGHT, LEFT, RIGHT,
	LEFT, LEFT, LEFT, PIN };

/* these models from http://home.t-online.de/home/thlet.wolter/ */

float wolf[] = { PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO, ZERO, ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO };

float pitti[] = { LEFT, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO, PIN, RIGHT, PIN, LEFT, PIN, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO, RIGHT, ZERO };

float leaf[] = { LEFT, ZERO, LEFT, ZERO, PIN, PIN, ZERO, ZERO, LEFT, ZERO, LEFT, ZERO, ZERO, PIN, ZERO, ZERO, RIGHT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, ZERO };

float doubled[] = { LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, RIGHT, LEFT, ZERO, LEFT, PIN, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, ZERO };

float symmetry[] = { RIGHT, ZERO, LEFT, RIGHT, LEFT, ZERO, LEFT, RIGHT, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, LEFT, RIGHT, ZERO, RIGHT, LEFT, RIGHT, ZERO, LEFT, ZERO };

float ostrich[] = { PIN, ZERO, ZERO, ZERO, ZERO, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, LEFT, ZERO, PIN, PIN, ZERO, ZERO, ZERO };

float ribbon[] = { RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, PIN, ZERO, PIN, PIN, ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, ZERO };

float roofed[] = { ZERO, LEFT, PIN, RIGHT, ZERO, PIN, LEFT, ZERO, PIN, ZERO, RIGHT, PIN, ZERO, LEFT, PIN, RIGHT, ZERO, PIN, LEFT, ZERO, PIN, ZERO, RIGHT, ZERO };

float furby[] = { PIN, ZERO, RIGHT, PIN, RIGHT, ZERO, PIN, ZERO, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, RIGHT, PIN, RIGHT, ZERO, PIN, ZERO, ZERO, PIN, ZERO };

float mushroom[] = { PIN, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, ZERO, LEFT, PIN, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, LEFT, ZERO, PIN, ZERO };

float taperingturned[] = { ZERO, ZERO, RIGHT, LEFT, PIN, LEFT, ZERO, PIN, PIN, ZERO, LEFT, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, RIGHT, PIN, RIGHT, LEFT, ZERO, ZERO, ZERO };

float viaduct[] = { PIN, RIGHT, PIN, LEFT, PIN, ZERO, ZERO, PIN, RIGHT, ZERO, RIGHT, RIGHT, ZERO, RIGHT, PIN, ZERO, ZERO, PIN, LEFT, PIN, RIGHT, PIN, ZERO, ZERO };

float window2world[] = { PIN, LEFT, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO, RIGHT, PIN, LEFT, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO };

float platform[] = { RIGHT, PIN, ZERO, ZERO, ZERO, ZERO, PIN, ZERO, ZERO, PIN, PIN, ZERO, PIN, LEFT, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO };

float heart[] = { ZERO, PIN, ZERO, ZERO, LEFT, ZERO, LEFT, ZERO, ZERO, ZERO, ZERO, PIN, ZERO, ZERO, ZERO, ZERO, RIGHT, ZERO, RIGHT, ZERO, ZERO, PIN, ZERO, ZERO };

float flatontop[] = { ZERO, PIN, PIN, ZERO, PIN, RIGHT, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, RIGHT, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO };

float noname /* aka memory card */[] = { LEFT, PIN, RIGHT, PIN, RIGHT, ZERO, PIN, ZERO, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, LEFT, ZERO };

float twowings[] = { PIN, LEFT, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, LEFT, ZERO, ZERO };

float dnastrand[] = { RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, ZERO };

float crown[] = { LEFT, ZERO, PIN, ZERO, RIGHT, ZERO, ZERO, LEFT, ZERO, PIN, ZERO, RIGHT, LEFT, ZERO, PIN, ZERO, RIGHT, ZERO, ZERO, LEFT, ZERO, PIN, ZERO, ZERO };

float wings2[] = { RIGHT, ZERO, PIN, ZERO, LEFT, PIN, RIGHT, PIN, RIGHT, LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT, PIN, LEFT, PIN, RIGHT, ZERO, PIN, ZERO, ZERO };

float cross2[] = { ZERO, ZERO, PIN, PIN, ZERO, LEFT, ZERO, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, ZERO, PIN, PIN, ZERO, LEFT, ZERO, ZERO, PIN, PIN, ZERO, ZERO };

float speedboat[] = { LEFT, ZERO, ZERO, LEFT, PIN, RIGHT, ZERO, ZERO, LEFT, ZERO, ZERO, PIN, ZERO, ZERO, RIGHT, ZERO, ZERO, LEFT, PIN, RIGHT, ZERO, ZERO, RIGHT, ZERO };

float snowman[] = { ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, ZERO };

float mask[] = { ZERO, ZERO, ZERO, RIGHT, ZERO, RIGHT, LEFT, ZERO, LEFT, PIN, ZERO, PIN, ZERO, ZERO, PIN, ZERO, PIN, RIGHT, ZERO, RIGHT, LEFT, ZERO, LEFT, ZERO };

float doubled2[] = { LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, ZERO, RIGHT, ZERO, RIGHT, ZERO, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, ZERO };

float podracer[] = { ZERO, PIN, ZERO, PIN, RIGHT, PIN, ZERO, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, LEFT, ZERO, PIN, PIN, ZERO, ZERO, LEFT, ZERO, PIN, LEFT, ZERO };

float obelisk[] = { PIN, ZERO, ZERO, ZERO, PIN, RIGHT, PIN, LEFT, PIN, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, LEFT, PIN, ZERO, ZERO, ZERO, ZERO };

float windwheel[] = { PIN, RIGHT, RIGHT, PIN, ZERO, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, ZERO };

float transport[] = { PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO, ZERO, ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO };

float microscope[] = { PIN, PIN, ZERO, ZERO, PIN, ZERO, RIGHT, PIN, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, PIN, LEFT, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO };

float upright[] = { PIN, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, PIN, ZERO, ZERO, LEFT, PIN, RIGHT, ZERO, ZERO, PIN, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, PIN, ZERO };

float twoslants[] = { ZERO, PIN, ZERO, ZERO, PIN, PIN, ZERO, PIN, ZERO, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, ZERO, ZERO, RIGHT, PIN, ZERO };

float embrace[] = { PIN, ZERO, ZERO, PIN, RIGHT, PIN, LEFT, PIN, ZERO, RIGHT, PIN, RIGHT, PIN, LEFT, PIN, LEFT, ZERO, PIN, RIGHT, PIN, LEFT, PIN, ZERO, ZERO };

float compact3[] = { ZERO, PIN, ZERO, PIN, PIN, ZERO, LEFT, PIN, RIGHT, ZERO, PIN, PIN, ZERO, PIN, ZERO, PIN, PIN, ZERO, LEFT, PIN, RIGHT, ZERO, PIN, ZERO };

float gate[] = { ZERO, ZERO, PIN, ZERO, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, LEFT, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, RIGHT, ZERO, PIN, PIN, ZERO, ZERO };

float revelation[] = { ZERO, ZERO, ZERO, PIN, ZERO, ZERO, PIN, RIGHT, LEFT, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, PIN, ZERO, ZERO, PIN, ZERO };

float threelegged[] = { RIGHT, ZERO, LEFT, RIGHT, ZERO, LEFT, PIN, RIGHT, ZERO, RIGHT, ZERO, PIN, ZERO, LEFT, ZERO, LEFT, PIN, RIGHT, ZERO, LEFT, RIGHT, ZERO, LEFT, ZERO };

float shelter[] = { LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT, ZERO, ZERO, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO, ZERO, ZERO, RIGHT, ZERO };

float kink[] = { ZERO, PIN, PIN, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, PIN, PIN, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO };

float dogface[] = { ZERO, ZERO, PIN, PIN, ZERO, LEFT, LEFT, RIGHT, PIN, ZERO, PIN, PIN, ZERO, PIN, LEFT, RIGHT, RIGHT, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO };

float symbol[] = { RIGHT, RIGHT, PIN, ZERO, PIN, PIN, ZERO, PIN, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, PIN, ZERO, PIN, PIN, ZERO, PIN, LEFT, LEFT, RIGHT, ZERO };

float wingnut[] = { ZERO, ZERO, ZERO, ZERO, PIN, RIGHT, RIGHT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, RIGHT, RIGHT, PIN, ZERO, ZERO, ZERO, ZERO, ZERO };

float chains[] = { PIN, ZERO, ZERO, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, ZERO, PIN, ZERO, ZERO, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, ZERO };

float smallship[] = { ZERO, LEFT, RIGHT, ZERO, RIGHT, LEFT, ZERO, LEFT, RIGHT, ZERO, LEFT, RIGHT, ZERO, LEFT, RIGHT, ZERO, RIGHT, LEFT, ZERO, LEFT, RIGHT, ZERO, LEFT, ZERO };

float compact2[] = { PIN, RIGHT, ZERO, ZERO, PIN, PIN, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, RIGHT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, ZERO, ZERO, ZERO, ZERO, ZERO };

float rowhouses[] = { RIGHT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, PIN, LEFT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, PIN, LEFT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, ZERO };

float doubled3[] = { LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, LEFT, RIGHT, ZERO, RIGHT, LEFT, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, ZERO };

float slide[] = { LEFT, RIGHT, LEFT, RIGHT, ZERO, LEFT, RIGHT, LEFT, PIN, ZERO, ZERO, PIN, ZERO, ZERO, PIN, RIGHT, LEFT, ZERO, ZERO, RIGHT, LEFT, RIGHT, LEFT, ZERO };

float upanddown[] = { ZERO, PIN, ZERO, PIN, ZERO, PIN, LEFT, PIN, RIGHT, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, LEFT, PIN, RIGHT, PIN, ZERO, ZERO };

float windwheelfromdown[] = { PIN, RIGHT, RIGHT, PIN, ZERO, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, ZERO };

float speedboat2[] = { PIN, RIGHT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, ZERO, LEFT, PIN, RIGHT, ZERO, LEFT, LEFT, LEFT, RIGHT, RIGHT, LEFT, PIN, ZERO, RIGHT, PIN, LEFT, ZERO };

float top[] = { ZERO, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, ZERO, LEFT, RIGHT, ZERO, ZERO, LEFT, LEFT, ZERO, LEFT, ZERO, ZERO, PIN, ZERO };

float tapering[] = { ZERO, ZERO, RIGHT, LEFT, PIN, LEFT, ZERO, PIN, PIN, ZERO, LEFT, PIN, RIGHT, ZERO, PIN, PIN, ZERO, RIGHT, PIN, RIGHT, LEFT, ZERO, ZERO, ZERO };

float waterfall[] = { LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, ZERO };

float ra[] = { PIN, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, ZERO, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, ZERO };

float mountain[] = { ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, LEFT, PIN, LEFT, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, ZERO };

float grotto[] = { PIN, PIN, ZERO, LEFT, RIGHT, LEFT, ZERO, PIN, RIGHT, PIN, LEFT, ZERO, ZERO, ZERO, ZERO, RIGHT, PIN, LEFT, PIN, ZERO, RIGHT, LEFT, RIGHT, ZERO };

float squarehole[] = { PIN, ZERO, PIN, ZERO, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO, PIN, ZERO };

float compact4[] = { PIN, RIGHT, ZERO, ZERO, PIN, ZERO, ZERO, PIN, PIN, ZERO, PIN, RIGHT, PIN, LEFT, PIN, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO, ZERO, ZERO };

float cactus[] = { PIN, LEFT, ZERO, PIN, PIN, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, PIN, RIGHT, PIN, LEFT, ZERO, ZERO, RIGHT, PIN, LEFT, PIN, ZERO, ZERO, ZERO };

float rocket[] = { RIGHT, ZERO, LEFT, PIN, RIGHT, ZERO, RIGHT, ZERO, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, ZERO, LEFT, ZERO, LEFT, PIN, RIGHT, ZERO, LEFT, ZERO };

float cross[] = { ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO, ZERO, ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN, ZERO, PIN, ZERO };

float infinity[] = { LEFT, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT, ZERO };

float twins[] = { ZERO, PIN, ZERO, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, PIN, ZERO, ZERO, PIN, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, ZERO, PIN, ZERO, ZERO, ZERO };

float frog2[] = { LEFT, ZERO, LEFT, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, LEFT, RIGHT, PIN, LEFT, LEFT, RIGHT, ZERO, RIGHT, ZERO };

float tie_fighter[] = { PIN, LEFT, RIGHT, LEFT, LEFT, PIN, RIGHT, ZERO, RIGHT,
	LEFT, ZERO, PIN, LEFT, LEFT, RIGHT, RIGHT, RIGHT, PIN, LEFT, ZERO,
	LEFT, RIGHT, ZERO };

	/* list of the above models */

float * model[] = {
	/* linear */
	straight, stars, thing, caterpillar, zigzag1, zigzag2, zigzag3,
	/* spherical */
	ball, half_balls,
	/* three-shapes */
	tri1, bow, snowflake, propellor, hexagon, triangle,
	/* quads */
	c3d, saddle, volcano, eagle, square, vee, quad3, em, glasses, quad2,
	quad1, mountains,
	/* flat */
	cat, dog, crucifix,
	/* misc */
	block, flower, turtle, basket, kayak, bird, seal, frog, quavers, fly,
	puppy, duck, prayer, tie_fighter,
	/* eric & thomas' easy models */
	wolf, pitti, leaf, doubled, symmetry, ostrich, ribbon, roofed, furby,
	mushroom, taperingturned, viaduct, window2world, platform, heart,
	flatontop, noname, twowings, dnastrand, crown, wings2, cross2, speedboat,
	snowman, mask, doubled2, podracer, obelisk, windwheel, transport,
	microscope, upright, twoslants, embrace, compact3, gate, revelation,
	threelegged, shelter, kink, dogface, symbol, wingnut, chains, smallship,
	compact2, rowhouses, doubled3, slide, upanddown, windwheelfromdown,
	speedboat2, top, tapering, waterfall, ra, mountain, grotto, squarehole,
	compact4, cactus, rocket, cross, infinity, twins, frog2
};
	
typedef struct {
	float curAngle;
	float destAngle;
} nodeAng;

int selected = 11;

nodeAng node[24];

int models = sizeof(model) / sizeof(float *);
int m;
int curModel;

/* model morphing */
float morph_angular_velocity = MORPH_ANG_VELOCITY;

/* snake metrics */
int is_cyclic = 0;
int is_legal = 1;
int debug = 0;

/* colour cycling */
float colour[3] = {0.0,0.0,1.0};
float colour_t[3];
float colour_i[3];

/* rotation angle */
float rotang1 = 0.0;
float rotang2 = 0.0;

struct timeb last_iteration;
struct timeb last_morph;

/* window size */
int width, height;

/* option variables */
float explode = 0.1;
int wireframe = 0;
int shiny = 0;
int interactive = 0;
int paused = 0;

/* trackball stuff */
float cumquat[4] = {0.0,0.0,0.0,0.0}, oldquat[4] = {0.0,0.0,0.0,1.0};
float rotation[16];
float m_s[3], m_e[3];
int dragging;

/* mmm, quaternion arithmetic */
void calc_rotation() {
	double Nq, s;
	double xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;

	/* this bit ripped from Shoemake's Quaternion notes from siggraph */
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
	
	/* set up our camera */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(40.0, 640/480.0, 0.05, 100.0);
	gluLookAt(0.0, 0.0, 20.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	glMatrixMode(GL_MODELVIEW);
	
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
	glBegin(GL_TRIANGLES);
	glNormal3fv(prism_n[0]);
	glVertex3fv(prism_v[0]);
	glVertex3fv(prism_v[1]);
	glVertex3fv(prism_v[2]);

	glNormal3fv(prism_n[4]);
	glVertex3fv(prism_v[3]);
	glVertex3fv(prism_v[5]);
	glVertex3fv(prism_v[4]);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3fv(prism_n[1]);
	glVertex3fv(prism_v[1]);
	glVertex3fv(prism_v[0]);
	glVertex3fv(prism_v[3]);
	glVertex3fv(prism_v[4]);
	
	glNormal3fv(prism_n[2]);
	glVertex3fv(prism_v[2]);
	glVertex3fv(prism_v[1]);
	glVertex3fv(prism_v[4]);
	glVertex3fv(prism_v[5]);
	
	glNormal3fv(prism_n[3]);
	glVertex3fv(prism_v[0]);
	glVertex3fv(prism_v[2]);
	glVertex3fv(prism_v[5]);
	glVertex3fv(prism_v[3]);
	glEnd();
	glEndList();
	
	/* build shiny display list */
	node_shiny = glGenLists(1);
	glNewList(node_shiny, GL_COMPILE);
	glBegin(GL_TRIANGLES);
	glNormal3fv(prism_n[0]);
	glVertex3fv(prism_v[0]);
	glNormal3fv(prism_n[1]);
	glVertex3fv(prism_v[1]);
	glNormal3fv(prism_n[2]);
	glVertex3fv(prism_v[2]);

	glNormal3fv(prism_n[4]);
	glVertex3fv(prism_v[3]);
	glNormal3fv(prism_n[3]);
	glVertex3fv(prism_v[5]);
	glNormal3fv(prism_n[2]);
	glVertex3fv(prism_v[4]);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3fv(prism_n[1]);
	glVertex3fv(prism_v[1]);
	glNormal3fv(prism_n[2]);
	glVertex3fv(prism_v[0]);
	glNormal3fv(prism_n[3]);
	glVertex3fv(prism_v[3]);
	glNormal3fv(prism_n[4]);
	glVertex3fv(prism_v[4]);

	glNormal3fv(prism_n[2]);
	glVertex3fv(prism_v[2]);
	glVertex3fv(prism_v[1]);
	glVertex3fv(prism_v[4]);
	glVertex3fv(prism_v[5]);

	glNormal3fv(prism_n[3]);
	glVertex3fv(prism_v[0]);
	glNormal3fv(prism_n[2]);
	glVertex3fv(prism_v[2]);
	glNormal3fv(prism_n[1]);
	glVertex3fv(prism_v[5]);
	glNormal3fv(prism_n[0]);
	glVertex3fv(prism_v[3]);
	glEnd();
	glEndList();

	/* build wire display list */
	node_wire = glGenLists(1);
	glNewList(node_wire, GL_COMPILE);
	glBegin(GL_LINE_LOOP);
	glVertex3fv(prism_v[0]);
	glVertex3fv(prism_v[1]);
	glVertex3fv(prism_v[2]);
	glEnd();
	glBegin(GL_LINE_LOOP);
	glVertex3fv(prism_v[3]);
	glVertex3fv(prism_v[5]);
	glVertex3fv(prism_v[4]);
	glEnd();
	glBegin(GL_LINE_LOOP);
	glVertex3fv(prism_v[1]);
	glVertex3fv(prism_v[0]);
	glVertex3fv(prism_v[3]);
	glVertex3fv(prism_v[4]);
	glEnd();
	glBegin(GL_LINE_LOOP);
	glVertex3fv(prism_v[2]);
	glVertex3fv(prism_v[1]);
	glVertex3fv(prism_v[4]);
	glVertex3fv(prism_v[5]);
	glEnd();
	glBegin(GL_LINE_LOOP);
	glVertex3fv(prism_v[0]);
	glVertex3fv(prism_v[2]);
	glVertex3fv(prism_v[5]);
	glVertex3fv(prism_v[3]);
	glEnd();
	glEndList();

	/* initialise the rotation */
	calc_rotation();
}

/* wot draws it */
void display(void) {
	int i;
	float ang;
	
	/* clear the buffer */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	/* go into the modelview stack */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glShadeModel(GL_SMOOTH); 

	/* draw this dang thing */
	
	/* rotate and translate into snake space */
	glRotatef(45.0,-5.0,0.0,1.0);
	glTranslatef(-0.5,0.0,0.5);
	
	/* rotate the 0th junction */
	glTranslatef(0.5,0.0,0.5);
	glMultMatrixf(rotation);
	glRotatef(rotang1, 0.0,1.0,0.0); 
	glRotatef(rotang2, 0.0,0.0,1.0); 
	glTranslatef(-0.5,0.0,-0.5);

	/* translate center to middle node */
	/* (disgusting hack by peter who knows naught of opengl) */
	for (i = 11; i >= 0; i--) {
		ang = node[i].curAngle;
		if (i % 2) {
			glTranslatef(0.5, 0.0, 0.5);
			glRotatef(-ang, 0.0, 1.0, 0.0);
			glTranslatef(-0.5, 0.0, -0.5);
			glTranslatef(1.0, -explode, 0.0);
		} else {
			glTranslatef(0.0, 0.5, 0.5);
			glRotatef(-ang, 1.0, 0.0, 0.0);
			glTranslatef(0.0, -0.5, -0.5);
			glTranslatef(-explode, 1.0, 0.0);
		}
		glRotatef(-180.0, 0.0, 0.0, 1.0);
	}

	/* now draw each node along the snake -- this is quite ugly :p */
	for (i = 0; i < 24; i++) {
		glPushMatrix();
		
		/* choose a colour for this node */
		if ((i == selected || i == selected+1) && interactive)
			glColor3f(1.0,1.0,0.0);
		else {
			if (i % 2)
				glColor3fv(colour);
			else
				glColor3f(1.0, 1.0, 1.0);
		}

		/* draw the node */
		if (wireframe)
			glCallList(node_wire);
		else if (shiny)
			glCallList(node_shiny);
		else
			glCallList(node_solid);

		/* now work out where to draw the next one */
		
		/* Interpolate between models */
		ang = node[i].curAngle;
		
		glRotatef(180.0, 0.0, 0.0, 1.0);
		if (i % 2) {
			glTranslatef(-1.0,explode,0.0);
			/* rotation of the joint */
			glTranslatef(0.5,0.0,0.5);
			glRotatef(ang, 0.0, 1.0, 0.0);
			glTranslatef(-0.5,0.0,-0.5);
		} else {
			glTranslatef(explode,-1.0,0.0);
			/* rotation of the joint */
			glTranslatef(0.0,0.5,0.5);
			glRotatef(ang, 1.0, 0.0, 0.0);
			glTranslatef(0.0,-0.5,-0.5);
		}
	}
	/* clear up the matrix stack */
	for (i = 0; i < 24; i++) {
		glPopMatrix();
	}

	glFlush();
	glutSwapBuffers();
}

int window_width, window_height;
int fullscreen = 0;

/* wot gets called when the winder is resized */
void reshape(int width, int height) {
	glViewport(0, 0, width, height);
	gluPerspective(60.0, width/(float)height, 0.05, 100.0);
	if (!fullscreen) {
		window_width = width;
		window_height = height;
	}
}

/* calculate snake metrics */
void calc_snake_metrics() {
	int srcDir, dstDir;
	int i, x, y, z;
	int prevSrcDir = -Y_MASK;
	int prevDstDir = Z_MASK;
	int grid[25][25][25];

	for (x = 0; x < 25; x++) 
		for (y = 0; y < 25; y++)
			for (z = 0; z < 25; z++)
				grid[x][y][z] = 0;

	is_legal = 1;
	x = y = z = 12;

	for (i = 0; i < 23; i++) {
		/* establish new state vars */
		srcDir = -prevDstDir;
		x += GETSCALAR(prevDstDir, X_MASK);
		y += GETSCALAR(prevDstDir, Y_MASK);
		z += GETSCALAR(prevDstDir, Z_MASK);

		switch ((int) node[i].destAngle) {
			case (int) ZERO:
				dstDir = -prevSrcDir;
				break;
			case (int) PIN:
				dstDir = prevSrcDir;
				break;
			case (int) RIGHT:
			case (int) LEFT:
				/* think cross product */
				dstDir = X_MASK * (GETSCALAR(prevSrcDir, Y_MASK) * GETSCALAR(prevDstDir, Z_MASK) -
					GETSCALAR(prevSrcDir, Z_MASK) * GETSCALAR(prevDstDir, Y_MASK) ) + 
					Y_MASK * (	GETSCALAR(prevSrcDir, Z_MASK) * GETSCALAR(prevDstDir, X_MASK) -
					GETSCALAR(prevSrcDir, X_MASK) * GETSCALAR(prevDstDir, Z_MASK) ) + 
					Z_MASK * (	GETSCALAR(prevSrcDir, X_MASK) * GETSCALAR(prevDstDir, Y_MASK) -
							GETSCALAR(prevSrcDir, Y_MASK) * GETSCALAR(prevDstDir, X_MASK) );
				if (node[i].destAngle == (int) RIGHT)
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
	
	is_cyclic = (dstDir == Y_MASK && x == 12 && y == 11 && z == 12);
}

void set_colours(float max_angle) {
	/* set target colour */
	if (!is_legal) {
		colour_t[0] = 0.5;
		colour_t[1] = 0.5;
		colour_t[2] = 0.5;
	} else if (is_cyclic) {
		colour_t[0] = 0.4;
		colour_t[1] = 0.8;
		colour_t[2] = 0.2;
	} else {
		colour_t[0] = 0.3;
		colour_t[1] = 0.1;
		colour_t[2] = 0.9;
	}
	/* jamie doesn't know if max_angle is the best thing for this */
	if (max_angle == 0.0)
		max_angle = 1.0;
	colour_i[0] = (colour_t[0] - colour[0]) / max_angle;
	colour_i[1] = (colour_t[1] - colour[1]) / max_angle;
	colour_i[2] = (colour_t[2] - colour[2]) / max_angle;
}
	
/* Start morph process to this model */
void start_morph(int modelIndex, int immediate) {
	int i;
	float max_angle;

	max_angle = 0.0;
	for (i = 0; i < 23; i++) {
		node[i].destAngle = model[modelIndex][i];
		if (immediate)
			node[i].curAngle = model[modelIndex][i];
		if (fabs(node[i].destAngle - node[i].curAngle) > max_angle)
			max_angle = fabs(node[i].destAngle - node[i].curAngle);
	}

	calc_snake_metrics();

	set_colours(max_angle);

	curModel = modelIndex;
}

void special(int key, int x, int y) {
	int i;

	if (interactive) {
		switch (key) {
			case GLUT_KEY_UP:
				selected = (selected + 22) % 23;
				break;
			case GLUT_KEY_DOWN:
				selected = (selected + 1) % 23;
				break;
			case GLUT_KEY_LEFT:
				node[selected].destAngle = fmod(node[selected].destAngle + LEFT, 360);
				break;
			case GLUT_KEY_RIGHT:
				node[selected].destAngle = fmod(node[selected].destAngle + RIGHT, 360);
				break;
			case GLUT_KEY_HOME:
				for (i = 0; i < 24; i++)
					node[i].destAngle = ZERO;
				break;
			default:
				break;
		}
	}
	calc_snake_metrics();
	set_colours(fabs(node[selected].destAngle - node[selected].curAngle));
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
			interactive = 1 - interactive;
			break;
		case 's':
			if (wireframe)
				glEnable(GL_LIGHTING);
			wireframe = 0;
			shiny = 1 - shiny;
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
				/* Reset last_iteration and last_morph time */
				ftime(&last_iteration);
				ftime(&last_morph);
			}
			paused = 1 - paused;
			break;
		case 'd':
			/* dump the current model so we can add it! */
			printf("float MODEL = { ");
			for (i = 0; i < 24; i++) {
				if (node[i].curAngle == ZERO)
					printf("ZERO");
				else if (node[i].curAngle == LEFT)
					printf("LEFT");
				else if (node[i].curAngle == PIN)
					printf("PIN");
				else if (node[i].curAngle == RIGHT)
					printf("RIGHT");
				else
					printf("%f", node[i].curAngle);
				if (i < 23)
					printf(", ");
			}
			printf(" };\n");
			break;
		case 'f':
			if (fullscreen) {
				fullscreen = 0;
				glutReshapeWindow(window_width, window_height);
			} else {
				fullscreen = 1;
				glutFullScreen();
			}
			break;
		default:
			break;
	}
}

void mouse(int button, int state, int x, int y) {
	switch (button) {
		case 0:
			switch (state) {
				case GLUT_DOWN:
					dragging = 1;
					m_s[0] = M_SQRT1_2 * (x - (width/ 2.0)) / (width / 2.0);
					m_s[1] = M_SQRT1_2 * ((height / 2.0) - y) / (height / 2.0);
					m_s[2] = sqrt(1 - (m_s[0] * m_s[0] + m_s[1] * m_s[1]));
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
		default:
			break;
	}
	glutPostRedisplay();
}

void motion(int x, int y) {
	double norm;
	float q[4];

	if (dragging) {
		m_e[0] = M_SQRT1_2 * (x - (width/ 2.0)) / (width / 2.0);
		m_e[1] = M_SQRT1_2 * ((height / 2.0) - y) / (height / 2.0);
		norm = m_e[0] * m_e[0] + m_e[1] * m_e[1];
		m_e[2] = sqrt(1 - (m_e[0] * m_e[0] + m_e[1] * m_e[1]));
		
		/* now here, build a quaternion from m_s and m_e */
		q[0] = m_s[1] * m_e[2] - m_s[2] * m_e[1];
		q[1] = m_s[2] * m_e[0] - m_s[0] * m_e[2];
		q[2] = m_s[0] * m_e[1] - m_s[1] * m_e[0];
		q[3] = m_s[0] * m_e[0] + m_s[1] * m_e[1] + m_s[2] * m_e[2];

		/* new rotation is the product of the new one and the old one */
		cumquat[0] = q[3] * oldquat[0] + q[0] * oldquat[3] + q[1] * oldquat[2] - q[2] * oldquat[1];
		cumquat[1] = q[3] * oldquat[1] + q[1] * oldquat[3] + q[2] * oldquat[0] - q[0] * oldquat[2];
		cumquat[2] = q[3] * oldquat[2] + q[2] * oldquat[3] + q[0] * oldquat[1] - q[1] * oldquat[0];
		cumquat[3] = q[3] * oldquat[3] - q[0] * oldquat[0] - q[1] * oldquat[1] - q[2] * oldquat[2];
		
		calc_rotation();
	}
	
	glutPostRedisplay();
}

void restore_idol(int value);
void quick_sleep(void);

/* "jwz?  no way man, he's my idle" -- Jamie, 2001.
 * I forget the context :( */
void idol(void) {
	/* time since last iteration */
	long iter_msec;
	/* time since the beginning of last morph */
	long morf_msec;
	float iter_angle_max;
	int i;
	struct timeb current_time;

	/* Do nothing to the model if we are paused */
	if (paused) {
		/* Avoid busy waiting when nothing is changing */
		quick_sleep();
		return;
	}
	/* ftime is winDOS compatible */
	ftime(&current_time);

	/* <spiv> Well, ftime gives time with millisecond resolution.
	 * <Jaq> if current time is exactly equal to last iteration, 
	 *       then don't do this block
	 * <spiv> (or worse, perhaps... who knows what the OS will do)
	 * <spiv> So if no discernable amount of time has passed:
	 * <spiv>   a) There's no point updating the screen, because
	 *             it would be the same
	 * <spiv>   b) The code will divide by zero
	 */
	iter_msec = (long) current_time.millitm - last_iteration.millitm + 
			((long) current_time.time - last_iteration.time) * 1000L;
	if (iter_msec) {
		/* save the current time */
		memcpy(&last_iteration, &current_time, sizeof(struct timeb));
		
		/* work out if we have to switch models */
		morf_msec = last_iteration.millitm - last_morph.millitm +
			((long) (last_iteration.time - last_morph.time) * 1000L);
		if ((morf_msec > MODEL_STATIC_TIME) && !interactive) {
			memcpy(&last_morph, &last_iteration, sizeof(struct timeb));
			start_morph(rand() % models, 0);
		}

		if (!dragging && !interactive) {
			rotang1 += 360/((1000/ROTATION_RATE1)/iter_msec);
			rotang2 += 360/((1000/ROTATION_RATE2)/iter_msec);
		}

		/* work out the maximum angle for this iteration */
		iter_angle_max = 90.0 * (morph_angular_velocity/1000.0) * iter_msec;

		for (i = 0; i < 24; i++) {
			if (node[i].curAngle != node[i].destAngle) {
				if (fabs(node[i].curAngle - node[i].destAngle) <= iter_angle_max)
					node[i].curAngle = node[i].destAngle;
				else if (fmod(node[i].curAngle - node[i].destAngle + 360, 360) > 180)
					node[i].curAngle = fmod(node[i].curAngle + iter_angle_max, 360);
				else
					node[i].curAngle = fmod(node[i].curAngle + 360 - iter_angle_max, 360);
			}
		}

		/* colour cycling */
		if (fabs(colour[0] - colour_t[0]) <= fabs(colour_i[0]))
			colour[0] = colour_t[0];
		else
			colour[0] += colour_i[0];
		if (fabs(colour[1] - colour_t[1]) <= fabs(colour_i[1]))
			colour[1] = colour_t[1];
		else
			colour[1] += colour_i[1];
		if (fabs(colour[2] - colour_t[2]) <= fabs(colour_i[2]))
			colour[2] = colour_t[2];
		else
			colour[2] += colour_i[2];

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
	width = 640;
	height = 480;
	
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(width, height);
	window = glutCreateWindow("glsnake");
	
	ftime(&last_iteration);
	memcpy(&last_morph, &last_iteration, sizeof(struct timeb));
	srand((unsigned int)last_iteration.time);

	m = rand() % models;
	start_morph(0, 1);	
	
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutIdleFunc(idol);
	
	init();
	
	/* glutFullScreen(); */
	
	atexit(unmain);
	
	glutSwapBuffers();
	glutMainLoop();

	return 0;
}
