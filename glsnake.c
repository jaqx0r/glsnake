/* $Id: glsnake.c,v 1.36 2001/10/11 08:57:14 andrew Exp $
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

#define GETSCALAR(vec,mask) ((vec)==(mask) ? 1 : ((vec)==-(mask) ? -1 : 0 ))

#ifndef M_SQRT1_2	/* Win32 doesn't have this constant */
#define M_SQRT1_2 0.70710678118654752440084436210485
#endif

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

typedef struct model_s {
	char * name;
	float node[24];
} model_t;

/* the actual models -- all with 24 nodes (23 joints) */

model_t model[] = {
    { "ball",
        { RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT,
          LEFT, RIGHT, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT, 
          RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT }
    },
    { "Spiv's half balls",
        { LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, 
          LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT,
          RIGHT, LEFT, RIGHT, LEFT, LEFT, LEFT }
    },
    { "cat",
        { ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN,
          PIN, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO, 
          ZERO, ZERO, ZERO }
    },
    { "zigzag1", 
        { RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, RIGHT,
          RIGHT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT,
          LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT }
    },
    { "zigzag2",
        { PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO,
          PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO,
          PIN, ZERO, PIN }
    },
    { "zigzag3",
        { PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT,
          PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT,
          PIN, LEFT, PIN }
    },
    { "caterpillar", 
        { RIGHT, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN,
          LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, LEFT, PIN,
          RIGHT, RIGHT, PIN, LEFT, LEFT }
    },
    { "bow", 
        { RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT,
          RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT,
          RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT }
    },
    { "snowflake",
        { RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT,
          RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT,
          RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT }
    },
    { "turtle",
        { ZERO, RIGHT, LEFT, ZERO, ZERO, RIGHT, LEFT, PIN, RIGHT,
          RIGHT, LEFT, RIGHT, LEFT, LEFT, PIN, LEFT, LEFT, LEFT,
          RIGHT, LEFT, RIGHT, RIGHT, RIGHT }
    },
    { "basket",
        { RIGHT, PIN, ZERO, ZERO, PIN, LEFT, ZERO, LEFT, LEFT,
          ZERO, LEFT, PIN, ZERO, ZERO, PIN, RIGHT, PIN, LEFT, PIN,
          ZERO, ZERO, PIN, LEFT }
    },
    { "thing",
        { PIN, RIGHT, LEFT, RIGHT, RIGHT, LEFT, PIN, LEFT, RIGHT,
          LEFT, LEFT, RIGHT, PIN, RIGHT, LEFT, RIGHT, RIGHT, LEFT,
          PIN, LEFT, RIGHT, LEFT, LEFT }
    },
    { "straight",
        { ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO,
          ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO,
          ZERO, ZERO, ZERO, ZERO, ZERO }
    },
    { "propellor", 
        { ZERO, ZERO, ZERO, RIGHT, LEFT, RIGHT, ZERO, LEFT, ZERO,
          ZERO, ZERO, RIGHT, LEFT, RIGHT, ZERO, LEFT, ZERO, ZERO,
          ZERO, RIGHT, LEFT, RIGHT, ZERO, LEFT }
    },
    { "hexagon", 
        { ZERO, ZERO, ZERO, ZERO, LEFT, ZERO, ZERO, RIGHT, ZERO,
          ZERO, ZERO, ZERO, LEFT, ZERO, ZERO, RIGHT, ZERO, ZERO,
          ZERO, ZERO, LEFT, ZERO, ZERO, RIGHT }
    },
    { "tri1",
        { ZERO, ZERO, LEFT, RIGHT, ZERO, LEFT, ZERO, RIGHT, ZERO,
          ZERO, LEFT, RIGHT, ZERO, LEFT, ZERO, RIGHT, ZERO, ZERO,
          LEFT, RIGHT, ZERO, LEFT, ZERO, RIGHT }
    },
    { "triangle",
        { ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, LEFT, RIGHT, ZERO,
          ZERO, ZERO, ZERO, ZERO, ZERO, LEFT, RIGHT, ZERO, ZERO,
          ZERO, ZERO, ZERO, ZERO, LEFT, RIGHT }
    },
    { "flower",
        { ZERO, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, LEFT, PIN,
          RIGHT, RIGHT, PIN, ZERO, LEFT, PIN, RIGHT, RIGHT, PIN,
          ZERO, LEFT, PIN, RIGHT, RIGHT, PIN }
    },
    { "crucifix", 
        { ZERO, PIN, PIN, ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN,
          ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO, ZERO, PIN, PIN,
          ZERO, ZERO, ZERO, PIN }
    },
    { "kayak", 
        { PIN, RIGHT, LEFT, PIN, LEFT, PIN, ZERO, ZERO, RIGHT, PIN,
          LEFT, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, RIGHT, PIN,
          LEFT, ZERO, ZERO, PIN, RIGHT }
    },
    { "bird",
        { ZERO, ZERO, ZERO, ZERO, RIGHT, RIGHT, ZERO, LEFT,
          PIN, RIGHT, ZERO, RIGHT, ZERO, RIGHT, ZERO, RIGHT,
          PIN, LEFT, ZERO, RIGHT, LEFT, ZERO, PIN }
    },
    { "seal",
        { RIGHT, LEFT, LEFT, PIN, RIGHT, LEFT, ZERO, PIN, PIN,
          ZERO, LEFT, ZERO, LEFT, PIN, RIGHT, ZERO, LEFT, LEFT,
          LEFT, PIN, RIGHT, RIGHT, LEFT }
    },
    { "dog",
        { ZERO, ZERO, ZERO, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO,
          PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO, PIN, ZERO, PIN,
          PIN, ZERO, PIN }
    },
    { "frog",
        { RIGHT, RIGHT, LEFT, LEFT, RIGHT, PIN, RIGHT, PIN, LEFT,
          PIN, RIGHT, ZERO, LEFT, ZERO, LEFT, PIN, RIGHT, ZERO,
          LEFT, LEFT, RIGHT, LEFT, LEFT }
    },
    { "quavers",
        { LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, ZERO, ZERO, ZERO,
          RIGHT, ZERO, ZERO, LEFT, RIGHT, ZERO, ZERO, ZERO, LEFT,
          LEFT, RIGHT, LEFT, RIGHT, RIGHT }
    },
    { "fly",
        { LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, ZERO, PIN, ZERO,
          ZERO, LEFT, PIN, RIGHT, ZERO, ZERO, PIN, ZERO, LEFT,
          LEFT, RIGHT, LEFT, RIGHT, RIGHT }
    },
    { "puppy",
        { ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, ZERO,
          ZERO, RIGHT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, RIGHT,
          PIN, RIGHT, LEFT }
    },
    { "stars",
        { LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, RIGHT, PIN,
          RIGHT, ZERO, ZERO, ZERO, RIGHT, PIN, RIGHT, LEFT, PIN,
          LEFT, RIGHT, PIN, RIGHT, LEFT }
    },
    { "mountains",
        { RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, LEFT, PIN, LEFT,
          PIN, LEFT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN,
          LEFT, PIN, LEFT, PIN, LEFT, PIN }
    },
    { "quad1",
        { RIGHT, PIN, RIGHT, RIGHT, RIGHT, PIN, LEFT, LEFT, LEFT,
          PIN, LEFT, PIN, RIGHT, PIN, RIGHT, RIGHT, RIGHT, PIN,
          LEFT, LEFT, LEFT, PIN, LEFT, PIN }
    },
    { "quad2",
        { ZERO, PIN, RIGHT, RIGHT, RIGHT, PIN, LEFT, LEFT, LEFT,
          PIN, ZERO, PIN, ZERO, PIN, RIGHT, RIGHT, RIGHT, PIN,
          LEFT, LEFT, LEFT, PIN, ZERO, PIN }
    },
    { "glasses",
        { ZERO, PIN, ZERO, RIGHT, RIGHT, PIN, LEFT, LEFT, ZERO, PIN,
          ZERO, PIN, ZERO, PIN, ZERO, RIGHT, RIGHT, PIN, LEFT, LEFT,
          ZERO, PIN, ZERO, PIN }
    },
    { "em",
        { ZERO, PIN, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, PIN,
          ZERO, PIN, ZERO, PIN, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO,
          ZERO, PIN, ZERO, PIN }
    },
    { "quad3", 
        { ZERO, RIGHT, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO,
          LEFT, ZERO, PIN, ZERO, RIGHT, ZERO, ZERO, RIGHT, PIN,
          LEFT, ZERO, ZERO, LEFT, ZERO, PIN }
    },
    { "vee",
        { ZERO, ZERO, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO,
          ZERO, ZERO, PIN, ZERO, ZERO, ZERO, ZERO, RIGHT, PIN,
          LEFT, ZERO, ZERO, ZERO, ZERO, PIN }
    }, 
    { "square",
        { ZERO, ZERO, ZERO, RIGHT, RIGHT, PIN, LEFT, LEFT, ZERO,
          ZERO, ZERO, PIN, ZERO, ZERO, ZERO, RIGHT, RIGHT, PIN,
          LEFT, LEFT, ZERO, ZERO, ZERO, PIN }
    }, 
    { "eagle",
        { RIGHT, ZERO, ZERO, RIGHT, RIGHT, PIN, LEFT, LEFT, ZERO,
          ZERO, LEFT, PIN, RIGHT, ZERO, ZERO, RIGHT, RIGHT, PIN,
          LEFT, LEFT, ZERO, ZERO, LEFT, PIN }
    },
    { "volcano",
        { RIGHT, ZERO, LEFT, RIGHT, RIGHT, PIN, LEFT, LEFT, RIGHT,
          ZERO, LEFT, PIN, RIGHT, ZERO, LEFT, RIGHT, RIGHT, PIN,
          LEFT, LEFT, RIGHT, ZERO, LEFT, PIN }
    }, 
    { "saddle",
        { RIGHT, ZERO, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT,
          ZERO, LEFT, PIN, RIGHT, ZERO, LEFT, ZERO, RIGHT, PIN,
          LEFT, ZERO, RIGHT, ZERO, LEFT, PIN }
    }, 
    { "c3d",
        { ZERO, ZERO, RIGHT, ZERO, ZERO, PIN, ZERO, ZERO, LEFT,
          ZERO, ZERO, PIN, ZERO, ZERO, RIGHT, ZERO, ZERO, PIN,
          ZERO, ZERO, LEFT, ZERO, ZERO, PIN }
    }, 
    { "block",
        { ZERO, ZERO, PIN, PIN, ZERO, RIGHT, PIN, LEFT, PIN,
          RIGHT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, ZERO, ZERO,
          PIN, ZERO, ZERO, LEFT, PIN, RIGHT }
    }, 
    { "duck",
        { LEFT, PIN, LEFT, PIN, ZERO, PIN, PIN, ZERO, PIN, ZERO,
          LEFT, PIN, RIGHT, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO,
          LEFT, PIN, LEFT }
    },
    { "prayer",
        { RIGHT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, ZERO, ZERO,
          ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, ZERO, RIGHT, RIGHT,
          LEFT, RIGHT, LEFT, LEFT, LEFT, PIN }
    }, 
    { "giraffe",
        { ZERO, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, ZERO,
          RIGHT, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, PIN, ZERO,
          LEFT, RIGHT, PIN, LEFT, LEFT, LEFT }
    },
    { "tie fighter",
        { PIN, LEFT, RIGHT, LEFT, LEFT, PIN, RIGHT, ZERO, RIGHT,
          LEFT, ZERO, PIN, LEFT, LEFT, RIGHT, RIGHT, RIGHT, PIN,
          LEFT, ZERO, LEFT, RIGHT, ZERO }
    }, 

    /* these models from http://home.t-online.de/home/thlet.wolter/ */

    { "Abstract",
        { RIGHT, LEFT, RIGHT, ZERO, PIN, ZERO, LEFT, RIGHT, LEFT, PIN,
          ZERO, ZERO, PIN, LEFT, RIGHT, LEFT, ZERO, PIN, ZERO,
          RIGHT, LEFT, RIGHT, ZERO }
    },
    { "AlanH1",
        { LEFT, RIGHT, ZERO, RIGHT, LEFT, ZERO, ZERO, RIGHT, LEFT, PIN,
          RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT, RIGHT,
          RIGHT, PIN, RIGHT, LEFT, PIN }
    },
    { "AlanH2",
        { LEFT, RIGHT, ZERO, RIGHT, LEFT, ZERO, ZERO, RIGHT, LEFT, PIN,
          RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT, RIGHT,
          LEFT, LEFT, RIGHT, LEFT, RIGHT }
    },
    { "AlanH3",
        { LEFT, RIGHT, ZERO, RIGHT, LEFT, ZERO, ZERO, RIGHT, LEFT, PIN,
          RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT, RIGHT,
          LEFT, PIN, LEFT, RIGHT, PIN }
    },
    { "AlanH4",
        { ZERO, ZERO, PIN, LEFT, RIGHT, LEFT, ZERO, RIGHT, LEFT, RIGHT,
          ZERO, PIN, ZERO, LEFT, RIGHT, LEFT, ZERO, RIGHT, LEFT,
          RIGHT, PIN, ZERO, ZERO }
    },
    { "Angel",
        { ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, RIGHT, LEFT, LEFT,
          RIGHT, LEFT, RIGHT, RIGHT, LEFT, LEFT, LEFT, PIN,
          RIGHT, LEFT, ZERO, ZERO, RIGHT, LEFT }
    },
    { "AnotherFigure",
        { LEFT, PIN, RIGHT, ZERO, ZERO, PIN, RIGHT, LEFT, LEFT, PIN,
          RIGHT, LEFT, ZERO, PIN, ZERO, RIGHT, LEFT, PIN, RIGHT,
          RIGHT, LEFT, PIN, ZERO }
    },
    { "Ball",
        { LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT,
          RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT,
          RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT }
    },
    { "Basket",
        { ZERO, RIGHT, RIGHT, ZERO, RIGHT, RIGHT, ZERO, RIGHT, LEFT,
          ZERO, LEFT, LEFT, PIN, RIGHT, LEFT, ZERO, LEFT, RIGHT,
          PIN, LEFT, LEFT, ZERO, LEFT }
    },
    { "Beetle",
        { PIN, LEFT, RIGHT, ZERO, LEFT, LEFT, RIGHT, LEFT, RIGHT,
          RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT,
          RIGHT, ZERO, LEFT, RIGHT, PIN, RIGHT }
    },
    { "Bone",
        { PIN, PIN, LEFT, ZERO, PIN, PIN, ZERO, LEFT, ZERO, ZERO, ZERO,
          ZERO, ZERO, ZERO, ZERO, RIGHT, ZERO, PIN, PIN, ZERO,
          RIGHT, PIN, PIN }
    },
    { "Bow",
        { LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT,
          LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT,
          LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT }
    },
    { "Bra",
        { RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT,
          LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, RIGHT,
          LEFT, RIGHT, RIGHT, LEFT, LEFT, LEFT }
    },
    { "BronchoSaurian",
        { ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO, PIN, ZERO,
          PIN, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO,
          ZERO, ZERO, PIN }
    },
    { "Cactus",
        { PIN, LEFT, ZERO, PIN, PIN, ZERO, RIGHT, PIN, LEFT, ZERO,
          ZERO, PIN, RIGHT, PIN, LEFT, ZERO, ZERO, RIGHT, PIN,
          LEFT, PIN, ZERO, ZERO }
    },
    { "Camel",
        { RIGHT, ZERO, PIN, RIGHT, PIN, RIGHT, ZERO, RIGHT, PIN, RIGHT,
          LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, ZERO, RIGHT, PIN,
          RIGHT, ZERO, ZERO, LEFT }
    },
    { "Candlestick",
        { LEFT, PIN, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, PIN,
          RIGHT, PIN, LEFT, PIN, LEFT, ZERO, RIGHT, PIN, LEFT,
          ZERO, RIGHT, PIN, RIGHT }
    },
    { "Cat",
        { ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN, PIN,
          ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO,
          ZERO, ZERO }
    },
    { "Cave",
        { RIGHT, ZERO, ZERO, PIN, LEFT, ZERO, PIN, PIN, ZERO, RIGHT,
          LEFT, PIN, RIGHT, RIGHT, LEFT, LEFT, PIN, RIGHT, RIGHT,
          LEFT, PIN, ZERO, ZERO }
    },
    { "Chains",
        { PIN, ZERO, ZERO, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN,
          ZERO, ZERO, PIN, ZERO, ZERO, PIN, LEFT, LEFT, PIN,
          RIGHT, RIGHT, PIN, ZERO }
    },
    { "Chair",
        { RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, RIGHT, ZERO, ZERO,
          PIN, PIN, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, ZERO,
          LEFT, RIGHT, LEFT, LEFT }
    },
    { "Chick",
        { RIGHT, RIGHT, RIGHT, PIN, LEFT, PIN, LEFT, PIN, RIGHT, RIGHT,
          RIGHT, PIN, LEFT, LEFT, LEFT, PIN, RIGHT, PIN, RIGHT,
          PIN, LEFT, LEFT, LEFT }
    },
    { "Clockwise",
        { RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT,
          RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT,
          RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT }
    },
    { "Cobra",
        { ZERO, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT,
          RIGHT, LEFT, LEFT, LEFT, LEFT, ZERO, LEFT, RIGHT, ZERO,
          ZERO, PIN, ZERO, ZERO, RIGHT }
    },
    { "Cobra2",
        { LEFT, ZERO, PIN, ZERO, PIN, LEFT, ZERO, PIN, ZERO, LEFT,
          LEFT, PIN, RIGHT, RIGHT, ZERO, PIN, ZERO, RIGHT, PIN,
          ZERO, PIN, ZERO, RIGHT }
    },
    { "Cobra3",
        { ZERO, LEFT, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, RIGHT,
          ZERO, PIN, ZERO, ZERO, LEFT, ZERO, ZERO, ZERO, PIN,
          ZERO, ZERO, ZERO, LEFT }
    },
    { "Compact1",
        { ZERO, ZERO, PIN, ZERO, ZERO, LEFT, PIN, RIGHT, PIN, LEFT,
          PIN, LEFT, PIN, RIGHT, PIN, LEFT, ZERO, PIN, PIN, ZERO,
          ZERO, LEFT, PIN }
    },
    { "Compact2",
        { LEFT, PIN, RIGHT, ZERO, ZERO, PIN, PIN, ZERO, RIGHT, PIN,
          LEFT, ZERO, ZERO, RIGHT, PIN, RIGHT, PIN, LEFT, PIN,
          RIGHT, ZERO, ZERO, ZERO }
    },
    { "Compact3",
        { ZERO, PIN, ZERO, PIN, PIN, ZERO, LEFT, PIN, RIGHT, ZERO, PIN,
          PIN, ZERO, PIN, ZERO, PIN, PIN, ZERO, LEFT, PIN, RIGHT,
          ZERO, PIN }
    },
    { "Compact4",
        { PIN, RIGHT, ZERO, ZERO, PIN, ZERO, ZERO, PIN, PIN, ZERO, PIN,
          RIGHT, PIN, LEFT, PIN, ZERO, PIN, PIN, ZERO, ZERO, PIN,
          ZERO, ZERO }
    },
    { "Compact5",
        { LEFT, ZERO, LEFT, PIN, RIGHT, PIN, LEFT, PIN, LEFT, PIN,
          RIGHT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, ZERO, RIGHT,
          PIN, RIGHT, PIN, LEFT }
    },
    { "Contact",
        { PIN, ZERO, ZERO, PIN, LEFT, LEFT, PIN, LEFT, RIGHT, RIGHT,
          PIN, LEFT, LEFT, RIGHT, PIN, RIGHT, RIGHT, PIN, ZERO,
          ZERO, PIN, RIGHT, PIN }
    },
    { "Cook",
        { ZERO, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, RIGHT, LEFT, PIN,
          LEFT, ZERO, PIN, PIN, ZERO, LEFT, PIN, LEFT, RIGHT,
          ZERO, RIGHT, ZERO, PIN }
    },
    { "Counterclockwise",
        { LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT,
          LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT,
          LEFT, LEFT, LEFT, LEFT }
    },
    { "Cradle",
        { LEFT, LEFT, ZERO, PIN, LEFT, RIGHT, LEFT, LEFT, RIGHT, LEFT,
          RIGHT, RIGHT, LEFT, RIGHT, PIN, ZERO, RIGHT, RIGHT,
          LEFT, LEFT, ZERO, ZERO, RIGHT }
    },
    { "Cross",
        { ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO, ZERO, PIN,
          PIN, ZERO, ZERO, ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN,
          ZERO, PIN }
    },
    { "Cross2",
        { ZERO, ZERO, PIN, PIN, ZERO, LEFT, ZERO, ZERO, PIN, PIN, ZERO,
          RIGHT, ZERO, ZERO, PIN, PIN, ZERO, LEFT, ZERO, ZERO,
          PIN, PIN, ZERO }
    },
    { "Cross3",
        { ZERO, ZERO, PIN, PIN, ZERO, LEFT, ZERO, ZERO, PIN, PIN, ZERO,
          RIGHT, ZERO, ZERO, PIN, PIN, ZERO, LEFT, ZERO, ZERO,
          PIN, PIN, ZERO }
    },
    { "CrossVersion1",
        { PIN, ZERO, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, PIN,
          LEFT, PIN, RIGHT, ZERO, PIN, RIGHT, PIN, RIGHT, LEFT,
          PIN, LEFT, RIGHT, PIN }
    },
    { "CrossVersion2",
        { RIGHT, LEFT, PIN, LEFT, LEFT, ZERO, RIGHT, LEFT, PIN, RIGHT,
          RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, LEFT, ZERO, LEFT,
          LEFT, PIN, LEFT, RIGHT }
    },
    { "Crown",
        { LEFT, ZERO, PIN, ZERO, RIGHT, ZERO, ZERO, LEFT, ZERO, PIN,
          ZERO, RIGHT, LEFT, ZERO, PIN, ZERO, RIGHT, ZERO, ZERO,
          LEFT, ZERO, PIN, ZERO }
    },
    { "DNAStrand",
        { RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN,
          RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT,
          PIN, RIGHT, PIN, RIGHT }
    },
    { "Diamond",
        { ZERO, RIGHT, ZERO, ZERO, LEFT, ZERO, ZERO, RIGHT, PIN, LEFT,
          LEFT, RIGHT, LEFT, RIGHT, RIGHT, PIN, LEFT, ZERO, ZERO,
          RIGHT, ZERO, ZERO, LEFT }
    },
    { "Dog",
        { RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, LEFT, RIGHT, RIGHT,
          LEFT, RIGHT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT,
          RIGHT, LEFT, LEFT, ZERO, LEFT, RIGHT }
    },
    { "DogFace",
        { ZERO, ZERO, PIN, PIN, ZERO, LEFT, LEFT, RIGHT, PIN, ZERO,
          PIN, PIN, ZERO, PIN, LEFT, RIGHT, RIGHT, ZERO, PIN,
          PIN, ZERO, ZERO, PIN }
    },
    { "DoublePeak",
        { ZERO, ZERO, PIN, ZERO, ZERO, RIGHT, LEFT, PIN, LEFT, RIGHT,
          PIN, RIGHT, LEFT, LEFT, ZERO, PIN, ZERO, RIGHT, RIGHT,
          LEFT, PIN, LEFT, RIGHT }
    },
    { "DoubleRoof",
        { ZERO, LEFT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT,
          RIGHT, LEFT, RIGHT, RIGHT, LEFT, LEFT, ZERO, LEFT,
          RIGHT, PIN, LEFT, LEFT, PIN, RIGHT }
    },
    { "DoubleToboggan",
        { ZERO, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, ZERO, PIN,
          PIN, ZERO, ZERO, ZERO, ZERO, LEFT, PIN, RIGHT, ZERO,
          ZERO, ZERO, ZERO, PIN }
    },
    { "Doubled",
        { LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, RIGHT,
          LEFT, ZERO, LEFT, PIN, LEFT, PIN, LEFT, RIGHT, PIN,
          RIGHT, LEFT, PIN, LEFT }
    },
    { "Doubled1",
        { LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, ZERO,
          RIGHT, ZERO, RIGHT, ZERO, LEFT, PIN, LEFT, RIGHT, PIN,
          RIGHT, LEFT, PIN, LEFT }
    },
    { "Doubled2",
        { LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, LEFT,
          RIGHT, ZERO, RIGHT, LEFT, LEFT, PIN, LEFT, RIGHT, PIN,
          RIGHT, LEFT, PIN, LEFT }
    },
    { "DumblingSpoon",
        { PIN, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, LEFT, ZERO, ZERO,
          LEFT, RIGHT, ZERO, ZERO, LEFT, RIGHT, ZERO, ZERO,
          RIGHT, ZERO, ZERO, ZERO, ZERO }
    },
    { "Embrace",
        { PIN, ZERO, ZERO, PIN, RIGHT, PIN, LEFT, PIN, ZERO, RIGHT,
          PIN, RIGHT, PIN, LEFT, PIN, LEFT, ZERO, PIN, RIGHT,
          PIN, LEFT, PIN, ZERO }
    },
    { "EndlessBelt",
        { ZERO, RIGHT, LEFT, ZERO, ZERO, ZERO, LEFT, RIGHT, ZERO, PIN,
          RIGHT, LEFT, ZERO, LEFT, RIGHT, LEFT, PIN, LEFT, RIGHT,
          LEFT, ZERO, LEFT, RIGHT }
    },
    { "Entrance",
        { LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT,
          RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, LEFT,
          RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT }
    },
    { "Esthetic",
        { LEFT, LEFT, PIN, RIGHT, RIGHT, ZERO, LEFT, PIN, RIGHT, PIN,
          LEFT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, ZERO, RIGHT,
          LEFT, PIN, RIGHT, RIGHT }
    },
    { "Explotion",
        { RIGHT, RIGHT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT,
          RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, LEFT, RIGHT,
          RIGHT, LEFT, RIGHT, LEFT, LEFT, LEFT }
    },
    { "F-ZeroXCar",
        { RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, PIN, RIGHT, LEFT,
          ZERO, ZERO, RIGHT, LEFT, ZERO, ZERO, LEFT, RIGHT, PIN,
          RIGHT, LEFT, PIN, LEFT, RIGHT }
    },
    { "Face",
        { ZERO, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, LEFT, PIN, RIGHT,
          RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, PIN, LEFT, PIN,
          LEFT, RIGHT, PIN, RIGHT }
    },
    { "Fantasy",
        { LEFT, LEFT, RIGHT, PIN, ZERO, RIGHT, ZERO, LEFT, PIN, LEFT,
          PIN, RIGHT, PIN, RIGHT, ZERO, LEFT, ZERO, PIN, LEFT,
          RIGHT, RIGHT, RIGHT, PIN }
    },
    { "Fantasy1",
        { PIN, ZERO, ZERO, PIN, PIN, ZERO, PIN, RIGHT, LEFT, RIGHT,
          RIGHT, PIN, LEFT, LEFT, RIGHT, LEFT, PIN, ZERO, PIN,
          PIN, ZERO, ZERO, PIN }
    },
    { "FaserGun",
        { ZERO, ZERO, LEFT, RIGHT, PIN, RIGHT, ZERO, RIGHT, PIN, RIGHT,
          LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, ZERO, RIGHT, PIN,
          RIGHT, RIGHT, ZERO, PIN }
    },
    { "FelixW",
        { ZERO, RIGHT, ZERO, PIN, LEFT, ZERO, LEFT, RIGHT, ZERO, ZERO,
          RIGHT, PIN, LEFT, ZERO, ZERO, LEFT, RIGHT, ZERO, RIGHT,
          PIN, ZERO, LEFT, ZERO }
    },
    { "Flamingo",
        { ZERO, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, PIN, LEFT, LEFT,
          PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, LEFT,
          ZERO, ZERO, ZERO, PIN }
    },
    { "FlatOnTheTop",
        { ZERO, PIN, PIN, ZERO, PIN, RIGHT, ZERO, RIGHT, LEFT, PIN,
          RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, RIGHT, ZERO,
          ZERO, PIN, ZERO, ZERO, PIN }
    },
    { "Fly",
        { ZERO, LEFT, PIN, RIGHT, ZERO, PIN, LEFT, PIN, LEFT, RIGHT,
          PIN, RIGHT, PIN, RIGHT, PIN, LEFT, PIN, LEFT, PIN,
          LEFT, RIGHT, PIN, RIGHT }
    },
    { "Fountain",
        { LEFT, RIGHT, LEFT, RIGHT, RIGHT, PIN, LEFT, PIN, LEFT, RIGHT,
          RIGHT, PIN, LEFT, LEFT, RIGHT, RIGHT, PIN, LEFT, LEFT,
          RIGHT, PIN, RIGHT, PIN }
    },
    { "Frog",
        { LEFT, LEFT, RIGHT, RIGHT, LEFT, PIN, LEFT, PIN, RIGHT, PIN,
          LEFT, ZERO, RIGHT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT,
          RIGHT, LEFT, RIGHT, RIGHT }
    },
    { "Frog2",
        { LEFT, ZERO, LEFT, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, ZERO,
          RIGHT, PIN, LEFT, ZERO, ZERO, LEFT, RIGHT, PIN, LEFT,
          LEFT, RIGHT, ZERO, RIGHT }
    },
    { "Furby",
        { PIN, ZERO, LEFT, PIN, RIGHT, ZERO, PIN, PIN, ZERO, PIN, PIN,
          ZERO, ZERO, PIN, ZERO, RIGHT, PIN, LEFT, ZERO, PIN,
          ZERO, ZERO, PIN }
    },
    { "Gate",
        { ZERO, ZERO, PIN, ZERO, ZERO, RIGHT, ZERO, PIN, PIN, ZERO,
          LEFT, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, RIGHT,
          ZERO, PIN, PIN, ZERO }
    },
    { "Ghost",
        { LEFT, LEFT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT,
          RIGHT, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, ZERO,
          LEFT, RIGHT, ZERO, ZERO, LEFT, RIGHT }
    },
    { "Globus",
        { RIGHT, LEFT, ZERO, PIN, LEFT, LEFT, RIGHT, RIGHT, LEFT,
          RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT,
          RIGHT, PIN, ZERO, RIGHT, LEFT, ZERO }
    },
    { "Grotto",
        { PIN, PIN, ZERO, LEFT, RIGHT, LEFT, ZERO, PIN, RIGHT, PIN,
          LEFT, ZERO, ZERO, ZERO, ZERO, RIGHT, PIN, LEFT, PIN,
          ZERO, RIGHT, LEFT, RIGHT }
    },
    { "H",
        { PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO, PIN, PIN, ZERO,
          PIN, LEFT, ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO, PIN,
          PIN, ZERO }
    },
    { "HeadOfDevil",
        { PIN, ZERO, RIGHT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, PIN,
          RIGHT, LEFT, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN,
          RIGHT, LEFT, ZERO, ZERO }
    },
    { "Heart",
        { RIGHT, ZERO, ZERO, ZERO, PIN, LEFT, PIN, LEFT, RIGHT, RIGHT,
          ZERO, PIN, ZERO, LEFT, LEFT, RIGHT, PIN, RIGHT, PIN,
          ZERO, ZERO, ZERO, LEFT }
    },
    { "Heart2",
        { ZERO, PIN, ZERO, ZERO, LEFT, ZERO, LEFT, ZERO, ZERO, ZERO,
          ZERO, PIN, ZERO, ZERO, ZERO, ZERO, RIGHT, ZERO, RIGHT,
          ZERO, ZERO, PIN, ZERO }
    },
    { "Hexagon",
        { ZERO, ZERO, ZERO, ZERO, LEFT, ZERO, ZERO, RIGHT, ZERO, ZERO,
          ZERO, ZERO, LEFT, ZERO, ZERO, RIGHT, ZERO, ZERO, ZERO,
          ZERO, LEFT, ZERO, ZERO }
    },
    { "HoleInTheMiddle1",
        { ZERO, LEFT, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, LEFT, ZERO,
          LEFT, RIGHT, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, PIN,
          LEFT, RIGHT, ZERO, RIGHT }
    },
    { "HoleInTheMiddle2",
        { ZERO, LEFT, RIGHT, ZERO, RIGHT, RIGHT, PIN, LEFT, RIGHT,
          ZERO, RIGHT, LEFT, ZERO, LEFT, RIGHT, ZERO, RIGHT,
          RIGHT, PIN, LEFT, RIGHT, ZERO, RIGHT }
    },
    { "HouseBoat",
        { RIGHT, RIGHT, PIN, LEFT, LEFT, LEFT, PIN, RIGHT, RIGHT,
          RIGHT, PIN, LEFT, RIGHT, ZERO, LEFT, PIN, RIGHT, PIN,
          LEFT, PIN, LEFT, RIGHT, PIN }
    },
    { "HouseByHouse",
        { LEFT, PIN, LEFT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, PIN,
          RIGHT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, RIGHT,
          PIN, RIGHT, PIN, RIGHT }
    },
    { "Infinity",
        { LEFT, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, RIGHT,
          LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, RIGHT, RIGHT, LEFT,
          LEFT, RIGHT, RIGHT, LEFT, LEFT }
    },
    { "Integral",
        { RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT,
          RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT,
          RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT }
    },
    { "Iron",
        { ZERO, ZERO, ZERO, ZERO, PIN, RIGHT, ZERO, RIGHT, ZERO, ZERO,
          LEFT, PIN, RIGHT, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO,
          ZERO, RIGHT, ZERO, RIGHT }
    },
    { "JustSquares",
        { RIGHT, RIGHT, LEFT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, LEFT,
          LEFT, PIN, RIGHT, RIGHT, LEFT, PIN, LEFT, PIN, RIGHT,
          PIN, RIGHT, LEFT, LEFT }
    },
    { "Kink",
        { ZERO, PIN, PIN, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO, RIGHT,
          PIN, LEFT, ZERO, ZERO, PIN, PIN, ZERO, PIN, ZERO, PIN,
          PIN, ZERO }
    },
    { "Knot",
        { LEFT, LEFT, PIN, LEFT, ZERO, LEFT, RIGHT, LEFT, PIN, LEFT,
          LEFT, RIGHT, RIGHT, PIN, RIGHT, LEFT, RIGHT, ZERO,
          RIGHT, PIN, RIGHT, RIGHT, LEFT }
    },
    { "Leaf",
        { ZERO, PIN, PIN, ZERO, ZERO, LEFT, ZERO, LEFT, ZERO, ZERO,
          PIN, ZERO, ZERO, RIGHT, ZERO, RIGHT, PIN, LEFT, ZERO,
          RIGHT, PIN, LEFT, ZERO }
    },
    { "LeftAsRight",
        { RIGHT, PIN, LEFT, RIGHT, LEFT, ZERO, RIGHT, LEFT, PIN, RIGHT,
          RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, LEFT, ZERO, RIGHT,
          LEFT, RIGHT, PIN, LEFT }
    },
    { "Long-necked",
        { PIN, ZERO, LEFT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, ZERO,
          PIN, ZERO, LEFT, PIN, LEFT, PIN, RIGHT, PIN, LEFT,
          ZERO, PIN, PIN, ZERO }
    },
    { "LunaModule",
        { PIN, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT,
          LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, PIN, LEFT,
          RIGHT, ZERO, RIGHT, LEFT, ZERO, LEFT }
    },
    { "MagnifyingGlass",
        { ZERO, ZERO, PIN, ZERO, LEFT, ZERO, PIN, PIN, ZERO, ZERO,
          RIGHT, PIN, LEFT, ZERO, ZERO, PIN, PIN, ZERO, RIGHT,
          ZERO, PIN, ZERO, ZERO }
    },
    { "Mask",
        { ZERO, ZERO, ZERO, RIGHT, ZERO, RIGHT, LEFT, ZERO, LEFT, PIN,
          ZERO, PIN, ZERO, ZERO, PIN, ZERO, PIN, RIGHT, ZERO,
          RIGHT, LEFT, ZERO, LEFT }
    },
    { "Microscope",
        { PIN, PIN, ZERO, ZERO, PIN, ZERO, RIGHT, PIN, ZERO, ZERO,
          RIGHT, PIN, LEFT, ZERO, ZERO, PIN, LEFT, ZERO, PIN,
          PIN, ZERO, PIN, PIN }
    },
    { "Mirror",
        { PIN, RIGHT, LEFT, ZERO, PIN, PIN, ZERO, ZERO, LEFT, RIGHT,
          ZERO, ZERO, PIN, ZERO, ZERO, LEFT, RIGHT, PIN, RIGHT,
          ZERO, PIN, PIN, ZERO }
    },
    { "MissPiggy",
        { ZERO, LEFT, LEFT, PIN, RIGHT, ZERO, RIGHT, RIGHT, PIN, LEFT,
          LEFT, RIGHT, RIGHT, PIN, LEFT, LEFT, ZERO, LEFT, PIN,
          RIGHT, RIGHT, ZERO, RIGHT }
    },
    { "Mole",
        { ZERO, RIGHT, ZERO, RIGHT, LEFT, RIGHT, PIN, ZERO, LEFT, PIN,
          RIGHT, ZERO, PIN, LEFT, RIGHT, LEFT, ZERO, LEFT, ZERO,
          RIGHT, RIGHT, PIN, LEFT }
    },
    { "Monk",
        { LEFT, ZERO, PIN, PIN, ZERO, LEFT, ZERO, PIN, PIN, ZERO,
          RIGHT, ZERO, PIN, PIN, ZERO, RIGHT, LEFT, RIGHT, RIGHT,
          LEFT, RIGHT, LEFT, LEFT }
    },
    { "Mountain",
        { ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO,
          LEFT, PIN, LEFT, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT,
          PIN, LEFT, RIGHT, ZERO }
    },
    { "Mountains",
        { ZERO, PIN, ZERO, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, PIN,
          RIGHT, PIN, LEFT, PIN, LEFT, PIN, LEFT, RIGHT, PIN,
          RIGHT, ZERO, PIN, ZERO }
    },
    { "MouseWithoutTail",
        { ZERO, PIN, PIN, ZERO, LEFT, ZERO, PIN, PIN, ZERO, ZERO,
          RIGHT, PIN, LEFT, ZERO, ZERO, PIN, PIN, ZERO, RIGHT,
          ZERO, PIN, PIN, ZERO }
    },
    { "Mushroom",
        { PIN, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, PIN, LEFT, RIGHT,
          ZERO, ZERO, LEFT, PIN, ZERO, RIGHT, ZERO, PIN, PIN,
          ZERO, LEFT, ZERO, PIN }
    },
    { "Necklace",
        { ZERO, ZERO, LEFT, ZERO, ZERO, ZERO, LEFT, ZERO, ZERO, ZERO,
          ZERO, PIN, ZERO, ZERO, ZERO, ZERO, RIGHT, ZERO, ZERO,
          ZERO, RIGHT, ZERO, ZERO }
    },
    { "NestledAgainst",
        { LEFT, ZERO, PIN, LEFT, LEFT, RIGHT, RIGHT, PIN, ZERO, RIGHT,
          PIN, LEFT, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, LEFT,
          RIGHT, LEFT, LEFT, LEFT }
    },
    { "NoClue",
        { ZERO, RIGHT, PIN, LEFT, LEFT, LEFT, ZERO, LEFT, PIN, RIGHT,
          RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, ZERO, RIGHT, RIGHT,
          RIGHT, PIN, LEFT, ZERO }
    },
    { "Noname",
        { LEFT, PIN, RIGHT, PIN, RIGHT, ZERO, PIN, ZERO, ZERO, PIN,
          PIN, ZERO, PIN, PIN, ZERO, RIGHT, PIN, LEFT, PIN,
          RIGHT, PIN, RIGHT, LEFT }
    },
    { "Obelisk",
        { PIN, ZERO, ZERO, ZERO, PIN, RIGHT, PIN, LEFT, PIN, LEFT, PIN,
          LEFT, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, LEFT, PIN,
          ZERO, ZERO, ZERO }
    },
    { "Ostrich",
        { ZERO, ZERO, PIN, PIN, ZERO, LEFT, ZERO, PIN, PIN, ZERO, PIN,
          PIN, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, ZERO, ZERO,
          ZERO, ZERO, PIN }
    },
    { "Ostrich2",
        { PIN, PIN, ZERO, PIN, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT,
          RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT, PIN, ZERO, PIN,
          ZERO, ZERO, PIN, ZERO }
    },
    { "PairOfGlasses",
        { ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO, LEFT,
          ZERO, PIN, ZERO, RIGHT, ZERO, PIN, ZERO, ZERO, PIN,
          ZERO, ZERO, PIN, ZERO }
    },
    { "Parrot",
        { ZERO, ZERO, ZERO, ZERO, RIGHT, RIGHT, ZERO, LEFT, PIN, RIGHT,
          ZERO, RIGHT, ZERO, RIGHT, ZERO, RIGHT, PIN, LEFT, ZERO,
          RIGHT, LEFT, ZERO, PIN }
    },
    { "Penis",
        { PIN, PIN, RIGHT, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO,
          RIGHT, PIN, LEFT, ZERO, ZERO, PIN, ZERO, PIN, PIN,
          ZERO, LEFT, PIN, PIN }
    },
    { "PictureCommingSoon",
        { LEFT, LEFT, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, PIN, RIGHT,
          LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, RIGHT, PIN, RIGHT,
          LEFT, ZERO, RIGHT, RIGHT }
    },
    { "Pitti",
        { LEFT, PIN, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO,
          RIGHT, PIN, LEFT, ZERO, ZERO, PIN, ZERO, ZERO, PIN,
          ZERO, ZERO, PIN, RIGHT }
    },
    { "Plait",
        { LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT,
          RIGHT, LEFT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT,
          RIGHT, RIGHT, RIGHT, RIGHT, LEFT }
    },
    { "Platform",
        { RIGHT, PIN, ZERO, ZERO, ZERO, ZERO, PIN, ZERO, ZERO, PIN,
          PIN, ZERO, PIN, LEFT, ZERO, RIGHT, LEFT, PIN, RIGHT,
          RIGHT, PIN, LEFT, RIGHT }
    },
    { "PodRacer",
        { ZERO, PIN, ZERO, PIN, RIGHT, PIN, ZERO, RIGHT, PIN, LEFT,
          LEFT, PIN, RIGHT, LEFT, ZERO, PIN, PIN, ZERO, ZERO,
          LEFT, ZERO, PIN, LEFT }
    },
    { "Pokemon",
        { LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, LEFT, LEFT,
          RIGHT, LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, RIGHT,
          LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT }
    },
    { "Prawn",
        { RIGHT, PIN, ZERO, PIN, RIGHT, ZERO, PIN, PIN, ZERO, ZERO,
          LEFT, PIN, RIGHT, ZERO, ZERO, PIN, PIN, ZERO, LEFT,
          PIN, ZERO, PIN, LEFT }
    },
    { "Propeller",
        { ZERO, ZERO, ZERO, RIGHT, ZERO, LEFT, RIGHT, LEFT, ZERO, ZERO,
          ZERO, RIGHT, ZERO, LEFT, RIGHT, LEFT, ZERO, ZERO, ZERO,
          RIGHT, ZERO, LEFT, RIGHT }
    },
    { "Pyramid",
        { ZERO, LEFT, PIN, RIGHT, ZERO, LEFT, PIN, RIGHT, ZERO, LEFT,
          PIN, RIGHT, ZERO, PIN, RIGHT, LEFT, LEFT, LEFT, PIN,
          RIGHT, RIGHT, RIGHT, LEFT }
    },
    { "QuarterbackTiltedAndReadyToHut",
        { PIN, ZERO, RIGHT, RIGHT, LEFT, RIGHT, PIN, RIGHT, LEFT,
          RIGHT, ZERO, PIN, ZERO, LEFT, RIGHT, LEFT, PIN, LEFT,
          RIGHT, LEFT, LEFT, ZERO, PIN }
    },
    { "Ra",
        { PIN, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT,
          RIGHT, LEFT, LEFT, ZERO, LEFT, LEFT, RIGHT, LEFT,
          RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT }
    },
    { "Rattlesnake",
        { LEFT, ZERO, LEFT, ZERO, LEFT, ZERO, LEFT, LEFT, ZERO, LEFT,
          ZERO, LEFT, ZERO, LEFT, RIGHT, ZERO, PIN, RIGHT, RIGHT,
          RIGHT, RIGHT, RIGHT, RIGHT }
    },
    { "Revelation",
        { ZERO, ZERO, ZERO, PIN, ZERO, ZERO, PIN, RIGHT, LEFT, LEFT,
          LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, RIGHT, RIGHT,
          LEFT, PIN, ZERO, ZERO, PIN }
    },
    { "Revolution1",
        { LEFT, LEFT, PIN, RIGHT, ZERO, PIN, ZERO, LEFT, PIN, RIGHT,
          RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, ZERO, PIN, ZERO,
          LEFT, PIN, RIGHT, RIGHT }
    },
    { "Ribbon",
        { RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, PIN, ZERO, PIN, PIN,
          ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN, RIGHT, RIGHT,
          LEFT, RIGHT, LEFT, LEFT }
    },
    { "Rocket",
        { RIGHT, ZERO, LEFT, PIN, RIGHT, ZERO, RIGHT, ZERO, LEFT, ZERO,
          RIGHT, PIN, LEFT, ZERO, RIGHT, ZERO, LEFT, ZERO, LEFT,
          PIN, RIGHT, ZERO, LEFT }
    },
    { "Roofed",
        { ZERO, LEFT, PIN, RIGHT, ZERO, PIN, LEFT, ZERO, PIN, ZERO,
          RIGHT, PIN, ZERO, LEFT, PIN, RIGHT, ZERO, PIN, LEFT,
          ZERO, PIN, ZERO, RIGHT }
    },
    { "Roofs",
        { PIN, PIN, RIGHT, ZERO, LEFT, PIN, RIGHT, PIN, LEFT, PIN,
          LEFT, PIN, RIGHT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT,
          ZERO, LEFT, PIN, PIN }
    },
    { "RowHouses",
        { RIGHT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, PIN, LEFT, PIN,
          LEFT, PIN, RIGHT, PIN, RIGHT, PIN, LEFT, PIN, LEFT,
          PIN, RIGHT, PIN, LEFT }
    },
    { "Sculpture",
        { RIGHT, LEFT, PIN, ZERO, ZERO, ZERO, LEFT, RIGHT, LEFT, PIN,
          ZERO, ZERO, PIN, LEFT, RIGHT, LEFT, ZERO, ZERO, ZERO,
          PIN, LEFT, RIGHT, LEFT }
    },
    { "Seal",
        { LEFT, LEFT, LEFT, PIN, RIGHT, RIGHT, RIGHT, ZERO, LEFT, PIN,
          RIGHT, ZERO, LEFT, LEFT, LEFT, PIN, RIGHT, LEFT, ZERO,
          PIN, PIN, ZERO, LEFT }
    },
    { "Seal2",
        { RIGHT, PIN, ZERO, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT,
          RIGHT, RIGHT, PIN, RIGHT, RIGHT, PIN, LEFT, RIGHT,
          ZERO, ZERO, LEFT, RIGHT, ZERO, ZERO }
    },
    { "Sheep",
        { RIGHT, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT,
          RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, RIGHT, LEFT,
          LEFT, LEFT, LEFT, LEFT, RIGHT, LEFT }
    },
    { "Shelter",
        { LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT,
          RIGHT, ZERO, ZERO, ZERO, ZERO, PIN, ZERO, ZERO, PIN,
          ZERO, ZERO, ZERO, ZERO, RIGHT }
    },
    { "Ship",
        { PIN, RIGHT, LEFT, LEFT, LEFT, LEFT, PIN, RIGHT, RIGHT, RIGHT,
          RIGHT, LEFT, ZERO, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO,
          LEFT, ZERO, PIN, PIN }
    },
    { "Shpongle",
        { LEFT, RIGHT, ZERO, RIGHT, LEFT, RIGHT, ZERO, RIGHT, LEFT,
          RIGHT, ZERO, RIGHT, LEFT, RIGHT, ZERO, RIGHT, LEFT,
          RIGHT, ZERO, RIGHT, LEFT, RIGHT, ZERO }
    },
    { "Slide",
        { LEFT, RIGHT, LEFT, RIGHT, ZERO, LEFT, RIGHT, LEFT, PIN, ZERO,
          ZERO, PIN, ZERO, ZERO, PIN, RIGHT, LEFT, ZERO, ZERO,
          RIGHT, LEFT, RIGHT, LEFT }
    },
    { "SmallShip",
        { ZERO, LEFT, RIGHT, ZERO, RIGHT, LEFT, ZERO, LEFT, RIGHT,
          ZERO, LEFT, RIGHT, ZERO, LEFT, RIGHT, ZERO, RIGHT,
          LEFT, ZERO, LEFT, RIGHT, ZERO, LEFT }
    },
    { "SnakeReadyToStrike",
        { LEFT, ZERO, LEFT, ZERO, LEFT, ZERO, LEFT, RIGHT, ZERO, RIGHT,
          ZERO, RIGHT, ZERO, LEFT, ZERO, ZERO, ZERO, PIN, ZERO,
          ZERO, ZERO, ZERO, LEFT }
    },
    { "Snowflake",
        { LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT,
          LEFT, LEFT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT,
          LEFT, LEFT, RIGHT, RIGHT, RIGHT, RIGHT }
    },
    { "Snowman",
        { ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO, PIN, PIN,
          ZERO, PIN, PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO, PIN,
          PIN, ZERO }
    },
    { "Source",
        { PIN, RIGHT, ZERO, PIN, ZERO, LEFT, PIN, RIGHT, PIN, LEFT,
          LEFT, RIGHT, LEFT, RIGHT, RIGHT, PIN, LEFT, LEFT,
          RIGHT, LEFT, RIGHT, RIGHT, PIN }
    },
    { "Speedboat",
        { LEFT, ZERO, ZERO, LEFT, PIN, RIGHT, ZERO, ZERO, LEFT, ZERO,
          ZERO, PIN, ZERO, ZERO, RIGHT, ZERO, ZERO, LEFT, PIN,
          RIGHT, ZERO, ZERO, RIGHT }
    },
    { "Speedboat2",
        { PIN, RIGHT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, ZERO, LEFT, PIN,
          RIGHT, ZERO, LEFT, LEFT, LEFT, RIGHT, RIGHT, LEFT, PIN,
          ZERO, RIGHT, PIN, LEFT }
    },
    { "Spider",
        { RIGHT, RIGHT, ZERO, ZERO, LEFT, RIGHT, LEFT, PIN, ZERO, LEFT,
          ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN, RIGHT, LEFT,
          RIGHT, ZERO, ZERO, LEFT }
    },
    { "Spitzbergen",
        { PIN, LEFT, ZERO, RIGHT, RIGHT, LEFT, PIN, ZERO, LEFT, PIN,
          RIGHT, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, ZERO, PIN,
          RIGHT, LEFT, LEFT, ZERO }
    },
    { "Square",
        { ZERO, ZERO, LEFT, LEFT, PIN, RIGHT, RIGHT, ZERO, ZERO, LEFT,
          LEFT, PIN, RIGHT, RIGHT, ZERO, ZERO, LEFT, LEFT, PIN,
          RIGHT, RIGHT, ZERO, ZERO }
    },
    { "SquareHole",
        { PIN, ZERO, PIN, ZERO, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO,
          PIN, ZERO, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO, PIN,
          ZERO, PIN }
    },
    { "Stage",
        { RIGHT, ZERO, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, RIGHT,
          PIN, RIGHT, LEFT, PIN, LEFT, RIGHT, LEFT, PIN, LEFT,
          RIGHT, PIN, RIGHT, ZERO }
    },
    { "Stairs",
        { ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO,
          PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO,
          PIN, ZERO }
    },
    { "Stairs2",
        { ZERO, PIN, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO,
          PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO,
          PIN, ZERO }
    },
    { "Straight",
        { ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO,
          ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO,
          ZERO, ZERO, ZERO, ZERO }
    },
    { "Swan",
        { ZERO, PIN, ZERO, PIN, LEFT, LEFT, PIN, LEFT, PIN, RIGHT, PIN,
          RIGHT, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, PIN, LEFT,
          PIN, LEFT, RIGHT }
    },
    { "Swan2",
        { PIN, ZERO, PIN, RIGHT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT,
          RIGHT, LEFT, RIGHT, RIGHT, RIGHT, PIN, ZERO, ZERO,
          ZERO, ZERO, ZERO, PIN, PIN }
    },
    { "Swan3",
        { PIN, PIN, ZERO, ZERO, ZERO, RIGHT, ZERO, RIGHT, ZERO, ZERO,
          LEFT, PIN, RIGHT, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO,
          ZERO, RIGHT, ZERO, RIGHT }
    },
    { "Symbol",
        { RIGHT, RIGHT, PIN, ZERO, PIN, PIN, ZERO, PIN, LEFT, LEFT,
          RIGHT, LEFT, RIGHT, RIGHT, PIN, ZERO, PIN, PIN, ZERO,
          PIN, LEFT, LEFT, RIGHT }
    },
    { "Symmetry",
        { RIGHT, ZERO, LEFT, RIGHT, LEFT, ZERO, LEFT, RIGHT, LEFT,
          ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, LEFT, RIGHT, ZERO,
          RIGHT, LEFT, RIGHT, ZERO, LEFT }
    },
    { "Symmetry2",
        { ZERO, PIN, LEFT, LEFT, PIN, ZERO, ZERO, LEFT, PIN, RIGHT,
          PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, LEFT,
          PIN, RIGHT, PIN, LEFT }
    },
    { "TableFireworks",
        { ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, RIGHT, PIN, LEFT,
          RIGHT, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, RIGHT,
          PIN, RIGHT, LEFT, ZERO, RIGHT, PIN }
    },
    { "Tapering",
        { ZERO, ZERO, RIGHT, LEFT, PIN, LEFT, ZERO, PIN, PIN, ZERO,
          LEFT, PIN, RIGHT, ZERO, PIN, PIN, ZERO, RIGHT, PIN,
          RIGHT, LEFT, ZERO, ZERO }
    },
    { "TaperingTurned",
        { ZERO, ZERO, RIGHT, LEFT, PIN, LEFT, ZERO, PIN, PIN, ZERO,
          LEFT, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, RIGHT, PIN,
          RIGHT, LEFT, ZERO, ZERO }
    },
    { "TeaLightStick",
        { RIGHT, ZERO, PIN, PIN, ZERO, LEFT, RIGHT, PIN, LEFT, LEFT,
          RIGHT, RIGHT, PIN, LEFT, LEFT, RIGHT, RIGHT, PIN, LEFT,
          LEFT, RIGHT, RIGHT, PIN }
    },
    { "Tent",
        { RIGHT, ZERO, ZERO, RIGHT, LEFT, ZERO, ZERO, RIGHT, LEFT,
          ZERO, ZERO, LEFT, RIGHT, ZERO, ZERO, RIGHT, LEFT, ZERO,
          ZERO, RIGHT, LEFT, ZERO, ZERO }
    },
    { "Terraces",
        { RIGHT, LEFT, ZERO, RIGHT, LEFT, PIN, LEFT, LEFT, PIN, LEFT,
          RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, PIN,
          RIGHT, RIGHT, PIN, RIGHT, LEFT }
    },
    { "Terrier",
        { PIN, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO, ZERO, PIN, PIN,
          ZERO, PIN, ZERO, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO,
          ZERO, ZERO }
    },
    { "Three-Legged",
        { RIGHT, ZERO, LEFT, RIGHT, ZERO, LEFT, PIN, RIGHT, ZERO,
          RIGHT, ZERO, PIN, ZERO, LEFT, ZERO, LEFT, PIN, RIGHT,
          ZERO, LEFT, RIGHT, ZERO, LEFT }
    },
    { "ThreePeaks",
        { RIGHT, ZERO, ZERO, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT,
          RIGHT, PIN, LEFT, LEFT, PIN, LEFT, PIN, RIGHT, PIN,
          LEFT, ZERO, ZERO, LEFT }
    },
    { "ToTheFront",
        { ZERO, PIN, RIGHT, LEFT, LEFT, LEFT, PIN, RIGHT, LEFT, ZERO,
          PIN, PIN, ZERO, LEFT, LEFT, PIN, ZERO, LEFT, RIGHT,
          ZERO, PIN, ZERO, LEFT }
    },
    { "Top",
        { PIN, LEFT, LEFT, PIN, LEFT, ZERO, ZERO, RIGHT, LEFT, PIN,
          RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, PIN, RIGHT, PIN,
          RIGHT, RIGHT, PIN, ZERO }
    },
    { "Transport",
        { PIN, ZERO, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, PIN, PIN,
          ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO,
          ZERO, ZERO }
    },
    { "Triangle",
        { ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, RIGHT, LEFT, ZERO, ZERO,
          ZERO, ZERO, ZERO, ZERO, RIGHT, LEFT, ZERO, ZERO, ZERO,
          ZERO, ZERO, ZERO, RIGHT }
    },
    { "Tripple",
        { PIN, ZERO, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, PIN, ZERO, PIN,
          LEFT, PIN, RIGHT, PIN, ZERO, PIN, LEFT, PIN, LEFT, PIN,
          RIGHT, PIN }
    },
    { "Turtle",
        { RIGHT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, LEFT, PIN,
          LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, PIN, LEFT,
          RIGHT, ZERO, ZERO, LEFT, RIGHT, ZERO }
    },
    { "Twins",
        { ZERO, PIN, ZERO, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, PIN,
          ZERO, ZERO, PIN, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT,
          ZERO, PIN, ZERO, ZERO }
    },
    { "TwoSlants",
        { ZERO, PIN, ZERO, ZERO, PIN, PIN, ZERO, PIN, ZERO, RIGHT, PIN,
          RIGHT, LEFT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, ZERO,
          ZERO, RIGHT, PIN }
    },
    { "TwoWings",
        { PIN, LEFT, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO,
          PIN, PIN, ZERO, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO,
          RIGHT, LEFT, ZERO }
    },
    { "UFO",
        { LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT,
          LEFT, LEFT, PIN, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT,
          RIGHT, LEFT, RIGHT, LEFT, LEFT }
    },
    { "USSEnterprice",
        { LEFT, PIN, RIGHT, PIN, RIGHT, LEFT, ZERO, PIN, PIN, ZERO,
          RIGHT, LEFT, ZERO, PIN, PIN, ZERO, RIGHT, LEFT, PIN,
          LEFT, PIN, RIGHT, ZERO }
    },
    { "UpAndDown",
        { ZERO, PIN, ZERO, PIN, ZERO, PIN, LEFT, PIN, RIGHT, PIN, ZERO,
          PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, LEFT, PIN, RIGHT,
          PIN, ZERO }
    },
    { "Upright",
        { ZERO, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, PIN, ZERO, ZERO,
          LEFT, PIN, RIGHT, ZERO, ZERO, PIN, RIGHT, RIGHT, LEFT,
          RIGHT, LEFT, LEFT, ZERO }
    },
    { "Upside-down",
        { PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO, RIGHT, RIGHT, LEFT,
          LEFT, PIN, RIGHT, RIGHT, LEFT, LEFT, ZERO, PIN, PIN,
          ZERO, ZERO, ZERO, PIN }
    },
    { "Valley",
        { ZERO, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, LEFT, RIGHT,
          ZERO, PIN, ZERO, LEFT, RIGHT, LEFT, PIN, LEFT, PIN,
          RIGHT, PIN, LEFT, ZERO }
    },
    { "Viaduct",
        { PIN, RIGHT, PIN, LEFT, PIN, ZERO, ZERO, PIN, RIGHT, ZERO,
          RIGHT, RIGHT, ZERO, RIGHT, PIN, ZERO, ZERO, PIN, LEFT,
          PIN, RIGHT, PIN, ZERO }
    },
    { "View",
        { ZERO, RIGHT, PIN, LEFT, PIN, RIGHT, ZERO, ZERO, RIGHT, PIN,
          LEFT, LEFT, RIGHT, RIGHT, PIN, LEFT, ZERO, ZERO, LEFT,
          PIN, RIGHT, PIN, LEFT }
    },
    { "Waterfall",
        { LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO,
          RIGHT, PIN, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT,
          PIN, LEFT, ZERO, RIGHT }
    },
    { "WindWheel",
        { PIN, RIGHT, RIGHT, PIN, ZERO, LEFT, PIN, RIGHT, RIGHT, PIN,
          ZERO, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, LEFT, PIN,
          RIGHT, RIGHT, PIN, ZERO }
    },
    { "Window",
        { PIN, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO, PIN, ZERO, PIN,
          ZERO, ZERO, PIN, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO,
          ZERO, ZERO }
    },
    { "WindowToTheWorld",
        { PIN, LEFT, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO,
          RIGHT, PIN, LEFT, ZERO, PIN, ZERO, ZERO, PIN, ZERO,
          ZERO, PIN, ZERO }
    },
    { "Windshield",
        { PIN, PIN, ZERO, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, ZERO,
          PIN, ZERO, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, ZERO,
          PIN, PIN, ZERO, PIN }
    },
    { "WingNut",
        { ZERO, ZERO, ZERO, ZERO, PIN, RIGHT, RIGHT, RIGHT, PIN, RIGHT,
          LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, RIGHT, RIGHT, PIN,
          ZERO, ZERO, ZERO, ZERO }
    },
    { "Wings2",
        { RIGHT, ZERO, PIN, ZERO, LEFT, PIN, RIGHT, PIN, RIGHT, LEFT,
          RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT, PIN, LEFT, PIN,
          RIGHT, ZERO, PIN, ZERO }
    },
    { "WithoutName",
        { PIN, RIGHT, PIN, RIGHT, RIGHT, PIN, LEFT, LEFT, PIN, ZERO,
          PIN, RIGHT, PIN, LEFT, PIN, ZERO, PIN, RIGHT, RIGHT,
          PIN, LEFT, LEFT, PIN }
    },
    { "Wolf",
        { ZERO, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO, PIN, ZERO, PIN,
          PIN, ZERO, PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO, ZERO,
          ZERO, PIN }
    },
    { "X",
        { LEFT, ZERO, ZERO, PIN, LEFT, RIGHT, RIGHT, PIN, LEFT, RIGHT,
          ZERO, PIN, PIN, ZERO, LEFT, RIGHT, PIN, LEFT, LEFT,
          RIGHT, PIN, ZERO, ZERO }
    }
/* end of models */
};

typedef struct {
	float curAngle;
	float destAngle;
} nodeAng;

int selected = 11;

nodeAng node[24];

int models = sizeof(model) / sizeof(model_t);
int m;
int curModel;

/* model morphing */
float morph_angular_velocity = MORPH_ANG_VELOCITY;

/* snake metrics */
int is_cyclic = 0;
int is_legal = 1;
int last_turn = -1;
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
int old_width, old_height;

/* font lsit number */
int font;

char * interactstr = "interactive";

/* option variables */
float explode = 0.1;
int wireframe = 0;
int shiny = 0;
int interactive = 0;
int paused = 0;
int fullscreen = 0;
int titles = 1;

int w = 0;

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
	glBegin(GL_LINE_STRIP);
	glVertex3fv(prism_v[0]);
	glVertex3fv(prism_v[1]);
	glVertex3fv(prism_v[2]);
	glVertex3fv(prism_v[0]);
	glVertex3fv(prism_v[3]);
	glVertex3fv(prism_v[4]);
	glVertex3fv(prism_v[5]);
	glVertex3fv(prism_v[3]);
	glEnd();
	glBegin(GL_LINES);
	glVertex3fv(prism_v[1]);
	glVertex3fv(prism_v[4]);
	glVertex3fv(prism_v[2]);
	glVertex3fv(prism_v[5]);
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
	glColor3f(1.0, 1.0, 0.0);
	{
		char * s;
		int i = 0;
		if (interactive)
			s = interactstr;
		else
			s = model[curModel].name;
		glRasterPos2f(0, height - 12);
		while (s[i] != 0)
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, s[i++]);
	}
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();
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

	/* translate centre to middle node */
	for (i = 11; i >= 0; i--) {
		ang = node[i].curAngle;
		glTranslatef(0.5, 0.5, 0.5);
		glRotatef(180+ang, -1.0, 0.0, 0.0);
		glTranslatef(-1.0-explode, 0.0, 0.0);
		glRotatef(90, 0.0, 0.0, 1.0);
		glTranslatef(-0.5, -0.5, -0.5);
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

		glTranslatef(0.5,0.5,0.5);		/* move to center */
		glRotatef(90, 0.0, 0.0, -1.0);		/* reorient  */
		glTranslatef(1.0 + explode, 0.0, 0.0);	/* move to new pos. */
		glRotatef(180 + ang, 1.0, 0.0, 0.0);	/* pivot to new angle */
		glTranslatef(-0.5,-0.5,-0.5);		/* return from center */
	}

	/* clear up the matrix stack */
	for (i = 0; i < 24; i++)
		glPopMatrix();
	

	if (titles)
		draw_title();

	glFlush();
	glutSwapBuffers();
}

/* wot gets called when the winder is resized */
void reshape(int w, int h) {
	glViewport(0, 0, w, h);
	gluPerspective(60.0, w/(float)h, 0.05, 100.0);
	width = w;
	height = h;
}

/* Returns the new dst_dir for the given src_dir and dst_dir */
int cross_product(src_dir, dst_dir) {
	return 	X_MASK*(GETSCALAR(src_dir,Y_MASK) * GETSCALAR(dst_dir,Z_MASK) -
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
			case (int) ZERO:
				dstDir = -prevSrcDir;
				break;
			case (int) PIN:
				dstDir = prevSrcDir;
				break;
			case (int) RIGHT:
			case (int) LEFT:
				dstDir = cross_product(prevSrcDir, prevDstDir);
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

	/* determine if the snake is cyclic */
	is_cyclic = (dstDir == Y_MASK && x == 12 && y == 11 && z == 12);

	/* determine last_turn */
	last_turn = -1;
	if (is_cyclic)
		switch (srcDir) {
			case -Z_MASK: last_turn = ZERO;  break;
			case Z_MASK:  last_turn = PIN;   break;
			case X_MASK:  last_turn = LEFT;  break;
			case -X_MASK: last_turn = RIGHT; break;
		}

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

/* Is a morph currently in progress? */
int morphing = 0;
	
/* Start morph process to this model */
void start_morph(int modelIndex, int immediate) {
	int i;
	float max_angle;

	max_angle = 0.0;
	for (i = 0; i < 23; i++) {
		node[i].destAngle = model[modelIndex].node[i];
		if (immediate)
			node[i].curAngle = model[modelIndex].node[i];
		if (fabs(node[i].destAngle - node[i].curAngle) > max_angle)
			max_angle = fabs(node[i].destAngle - node[i].curAngle);
	}

	calc_snake_metrics();

	set_colours(max_angle);

	curModel = modelIndex;
	morphing = 1;
}

void special(int key, int x, int y) {
	int i;
	float tmp;
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
				tmp = fmod(node[selected].destAngle+LEFT,360);
				node[selected].destAngle = tmp;
				morphing = 1;
				break;
			case GLUT_KEY_RIGHT:
				tmp = fmod(node[selected].destAngle+RIGHT,360);
				node[selected].destAngle = tmp;
				morphing = 1;
				break;
			case GLUT_KEY_HOME:
				for (i = 0; i < 24; i++)
					node[i].destAngle = ZERO;
				morphing = 1;
				break;
			default:
				unknown_key = 1;
				break;
		}
	}
	calc_snake_metrics();
	set_colours(fabs(node[selected].destAngle - node[selected].curAngle));
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
			interactive = 1 - interactive;
			glutPostRedisplay();
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
			printf("    { \"noname\", /* %s */\n        { ",
			       model[curModel].name);
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
			printf(" }\n    },\n");
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
	int still_morphing;

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
			((long) (last_iteration.time-last_morph.time) * 1000L);
		if ((morf_msec > MODEL_STATIC_TIME) && !interactive) {
			memcpy(&last_morph, &last_iteration, 
				sizeof(struct timeb));
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

		/* work out the maximum angle for this iteration */
		iter_angle_max = 90.0 * (morph_angular_velocity/1000.0) * iter_msec;

		still_morphing = 0;
		for (i = 0; i < 24; i++) {
			float curAngle = node[i].curAngle;
			float destAngle = node[i].destAngle;
			if (curAngle != destAngle) {
				still_morphing = 1;
				if (fabs(curAngle-destAngle) <= iter_angle_max)
					curAngle = destAngle;
				else if (fmod(curAngle-destAngle+360,360) > 180)
					curAngle = fmod(curAngle + 
							iter_angle_max, 360);
				else
					curAngle = fmod(curAngle + 360 - 
							iter_angle_max, 360);
			}
		}

		if (!still_morphing)
			morphing = 0;

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
	
	atexit(unmain);
	
	glutSwapBuffers();
	glutMainLoop();

	return 0;
}
