/* $Id: glsnake.c,v 1.8 2001/10/04 16:29:48 jaq Exp $
 * An OpenGL imitation of Rubik's Snake 
 * by Jamie Wilkinson, Andrew Bennetts and Peter Aylett
 * based on the Allegro snake.c by Peter Aylett and Andrew Bennetts
 *
 * Known issues:
 * - Z-fighting occurs in solid mode with the edges drawn
 * - z-fighting occurs when the nodes are close (explode distance == 0.0)
 * - nodes pass through themselves! :)
 */

#include <GL/glut.h>
#include <sys/timeb.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* angles */
#define ZERO	0.0
#define RIGHT   270.0
#define PIN     180.0
#define LEFT    90.0

#define X_MASK	1
#define Y_MASK	2
#define Z_MASK	4

#define ROTATION_RATE1		0.10		/* Rotations per second */
#define ROTATION_RATE2		0.14		/* Rotations per second */
#define EXPLODE_INCREMENT	0.05
#define MORPH_DELAY			3.0			/* Delay in seconds between morphs */
#define INIT_MORPH_RATE		1.0			/* Morphs per second */
#define MORPH_RATE_CHANGE	0.1

#define ABS(x)				((x) > 0 ? (x) : -(x))
#define FMOD(num,denom)		((num) >= (denom) ? (num) - (denom) : (num))	/* works only in our special case */
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
float prism_n[][3] = {{0.0,0.0,1.0},
	                  {0.0,-1.0,0.0},
			  {0.707,0.707,0.0},
			  {-1.0,0.0,0.0},
			  {0.0,0.0,-1.0}};

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

/* this model sucks */
/* but try watching a caterpillar do a transition to this! */
/*
float zigzag3_wrong[] = { PIN, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, PIN};
*/

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

/* Note: this is a 32 node model
float snowflake32[] = { RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT,
LEFT, LEFT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT, LEFT,
RIGHT, RIGHT, RIGHT};
*/

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

float * model[] = { straight, stars, thing, caterpillar, zigzag1, zigzag2, zigzag3,	// linear
	ball, half_balls,													// spherical						
	tri1, bow, snowflake, propellor, hexagon, triangle,						// triples
	c3d, saddle, volcano, eagle, square, vee, quad3, em, glasses, quad2, quad1, mountains,		// quads
	cat, dog, crucifix,													// flat
	block, flower, turtle, basket, kayak, bird, seal, frog, quavers, fly, puppy, duck }; // models
	

typedef struct {
	float curAngle;
	float destAngle;
} nodeAng;

int selected = 11;

nodeAng node[24];

int models = sizeof(model) / sizeof(float *);
int m;
int curModel;
float morph = 0.0;
float morphRate = INIT_MORPH_RATE;
int morphComplete = 1;			/* look at this to see if morph is in progress */

/* snake metrics */
int is_cyclic = 0;
int is_legal = 1;
int debug = 0;

/* rotation angle */
float rotang1 = 0.0;
float rotang2 = 0.0;

/* morph ratio (how far between model m and model m_next we are) */
//float morph = 0.0;
//float ma_morph = 0.0;

struct timeb last_iteration;
struct timeb last_morph;


/* option variables */
float explode = 0.05;
int wireframe = 0;
int shiny = 1;
int interactive = 0;

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
	/*glEnable(GL_LINE_SMOOTH); */
	/*glEnable(GL_POLYGON_SMOOTH );*/ 

	/* draw this dang thing */
	
	/* the origin */
/*	glPointSize(4.0);
	glBegin(GL_POINTS);
	glColor3f(1.0,1.0,0.0);
	glVertex3f(0.0,0.0,0.0);
	glEnd();
*/

	/* rotate and translate into snake space */
	glRotatef(45.0,-5.0,0.0,1.0);
	glTranslatef(-0.5,0.0,0.5);
	
	/* rotate the 0th junction */
	glTranslatef(0.5,0.0,0.5);
	glRotatef(rotang1, 0.0,1.0,0.0); 
	glRotatef(rotang2, 0.0,0.0,1.0); 
	glTranslatef(-0.5,0.0,-0.5);


	/* translate center to middle node */
	/* (disgusting hack by peter who knows naught of opengl) */

	for (i = 11; i >= 0; i--) {
		ang = node[i].curAngle;
		if (i % 2) {
			glTranslatef(0.5,0.0,0.5);
			glRotatef(-ang, 0.0, 1.0, 0.0);
			glTranslatef(-0.5,0.0,-0.5);
			glTranslatef(1.0,-explode,0.0);
		} else {
			glTranslatef(0.0,0.5,0.5);
			glRotatef(-ang, 1.0, 0.0, 0.0);
			glTranslatef(0.0,-0.5,-0.5);
			glTranslatef(-explode,1.0,0.0);
		}
		glRotatef(-180.0, 0.0,0.0,1.0);
	}

	/* now draw each node along the snake -- this is quite ugly :p */

	for (i = 0; i < 24; i++) {
		glPushMatrix();
		/* get node */
		if ((i == selected || i == selected+1) && interactive)
			glColor3f(1.0,1.0,0.0);		// yellow
		else if (!is_legal)
			glColor3f(0.5,0.5,0.5);		// grey
		else if (i % 2)
			if (is_cyclic)
				glColor3f(1.0,0.0,0.0);	// red
			else
				glColor3f(0.0,0.0,1.0);	// blue
		else
			glColor3f(1.0,1.0,1.0);		// white

		/* get call list */
		if (wireframe)
			glCallList(node_wire);
		else if (shiny)
			glCallList(node_shiny);
		else {
			glCallList(node_solid);
		}

		/* Interpolate between models */
		ang = node[i].curAngle;
		
		glRotatef(180.0, 0.0,0.0,1.0);
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
	for (i = 0; i < 24; i++) {
		glPopMatrix();
	}
	glFlush();
	/* glutSwapBuffers(); */
}

/* wot gets called when the winder is resized */
void reshape(int width, int height) {
	glViewport(0, 0, width, height);
	gluPerspective(60.0, width/(float)height, 0.05, 100.0);
}

/* Instantly set snake to this model */
void set_model(int modelIndex)
{
	int i;
	for (i=0; i<23; i++)
		node[i].destAngle = node[i].curAngle = model[modelIndex][i];
	curModel = modelIndex;
}

/* calculate snake metrics */
void snake_metrics() {
	int srcDir, dstDir;
	int i, x, y, z;
	int prevSrcDir = -Y_MASK;
	int prevDstDir = Z_MASK;
	int grid[25][25][25];

	for (x=0; x<25; x++)
		for (y=0; y<25; y++)
			for (z=0; z<25; z++)
				grid[x][y][z] = 0;

	is_legal = 1;
	x = y = z = 12;

	for (i=0; i<23; i++)
	{
		// establish new state vars
		srcDir = -prevDstDir;
		x += GETSCALAR(prevDstDir, X_MASK);
		y += GETSCALAR(prevDstDir, Y_MASK);
		z += GETSCALAR(prevDstDir, Z_MASK);

		switch ((int)node[i].destAngle)
		{
			case (int)(ZERO):
				dstDir = -prevSrcDir;
				break;
			case (int)(PIN):
				dstDir = prevSrcDir;
				break;
			case (int)(RIGHT):
			case (int)(LEFT):
				// think cross product
				dstDir =	X_MASK * (	GETSCALAR(prevSrcDir, Y_MASK) * GETSCALAR(prevDstDir, Z_MASK) -
										GETSCALAR(prevSrcDir, Z_MASK) * GETSCALAR(prevDstDir, Y_MASK) ) + 
							Y_MASK * (	GETSCALAR(prevSrcDir, Z_MASK) * GETSCALAR(prevDstDir, X_MASK) -
										GETSCALAR(prevSrcDir, X_MASK) * GETSCALAR(prevDstDir, Z_MASK) ) + 
							Z_MASK * (	GETSCALAR(prevSrcDir, X_MASK) * GETSCALAR(prevDstDir, Y_MASK) -
										GETSCALAR(prevSrcDir, Y_MASK) * GETSCALAR(prevDstDir, X_MASK) );
				if (node[i].destAngle == (int)(RIGHT))
					dstDir = -dstDir;
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

/* Start morph process to this model */
void start_morph(int modelIndex)
{
	int i;
	for (i=0; i<23; i++)
		node[i].destAngle = model[modelIndex][i];
	snake_metrics();
	curModel = modelIndex;
}

void keyboard_ex(int key, int x, int y) {
	int i;

	if (interactive)
		switch (key) {
			case GLUT_KEY_UP:
				selected = (selected + 22) % 23;
				break;
			case GLUT_KEY_DOWN:
				selected = (selected + 1) % 23;
				break;
			case GLUT_KEY_LEFT:
				node[selected].destAngle = FMOD(node[selected].destAngle + 90, 360);
				break;
			case GLUT_KEY_RIGHT:
				node[selected].destAngle = FMOD(node[selected].destAngle + 270, 360);
				break;
			case GLUT_KEY_HOME:
				for (i=0; i<24; i++)
					node[i].destAngle = 0;
				break;
		}
	snake_metrics();
}

void keyboard(unsigned char c, int x, int y) {
	switch (c) {
		case 'q':
			exit(0);
			break;
		case 'e':
			explode += EXPLODE_INCREMENT;
			break;
		case 'E':
			explode -= EXPLODE_INCREMENT;
			if (explode < 0.0) explode = 0.0;
			break;
		case '.':	// think right arrow
			/* Guard against changing models during a morph - no longer necessary*/
			curModel++;
			curModel %= models;
			start_morph( curModel );

			/* Reset last_morph time */
			ftime(&last_morph);			
			break;
		case ',':	// think left arrow
			/* Guard against changing models during a morph - no longer necessary*/
			curModel = (curModel + models - 1) % models;
			start_morph( curModel );

			/* Reset last_morph time */
			ftime(&last_morph);			
			break;
		case '+':
			morphRate += MORPH_RATE_CHANGE;
			break;
		case '-':
			if (morphRate > MORPH_RATE_CHANGE)
				morphRate -= MORPH_RATE_CHANGE;
			break;
		case 'i':
			interactive = 1 - interactive;
			break;
		case 's':
			if (wireframe)
				glEnable(GL_LIGHTING);
			wireframe = 0;
			shiny = 1 - shiny;
			break;
		case 'w':
			wireframe = 1 - wireframe;
			if (wireframe)
				glDisable(GL_LIGHTING);
			else
				glEnable(GL_LIGHTING);
			break;
		case 'd':
			debug = 1 - debug;
			break;
		default:
			break;
	}
}

/* "jwz?  no way man, he's my idle" -- me, 2001.  I forget the context :( */
void idol(void) {
        long i_sec, i_usec;             /* used for tracking how far through an iteration we are */
        long m_sec, m_usec;
		float maxmorph;
		int i;
		struct timeb current_time;

        ftime(&current_time);
		if (memcmp(&current_time, &last_iteration,sizeof(struct timeb))) {
			i_sec = (long)-last_iteration.time;
			i_usec = -last_iteration.millitm;
			memcpy(&last_iteration, &current_time, sizeof(struct timeb));
			i_sec += (long)last_iteration.time;
			i_usec += last_iteration.millitm;
			i_usec += i_sec*1000;

			rotang1 += 360/((1000/ROTATION_RATE1)/i_usec);
			rotang2 += 360/((1000/ROTATION_RATE2)/i_usec);

			maxmorph = 90/((1000/morphRate)/i_usec);

			morphComplete = 1;
			for (i=0; i<24; i++) {
				if (node[i].curAngle != node[i].destAngle) {
					morphComplete = 0;
					if (ABS(node[i].curAngle - node[i].destAngle) <= maxmorph)
						node[i].curAngle = node[i].destAngle;
					else if (FMOD(node[i].curAngle - node[i].destAngle + 360, 360) > 180)
						node[i].curAngle = FMOD(node[i].curAngle + maxmorph, 360);
					else
						node[i].curAngle = FMOD(node[i].curAngle + 360 - maxmorph, 360);
				}
			}

			/* do the morphing stuff here */
			m_sec = (long)(last_iteration.time - last_morph.time);
			m_usec = last_iteration.millitm - last_morph.millitm;
			m_usec += m_sec*1000;
			if (m_usec > (long)(MORPH_DELAY*1000)) {
					morph = (m_usec - MORPH_DELAY*1000) * (morphRate/1000);
					if (morph > 1.0) {
							morph = 0.0;
							memcpy(&last_morph, &last_iteration, sizeof(struct timeb));
							if (!interactive)
								start_morph( rand() % models );
					}
			}
	

			glutSwapBuffers();
			glutPostRedisplay();
		}
}

/* stick anything that needs to be shutdown properly here */
void unmain(void) {
	glutDestroyWindow(window);
}

int main(int argc, char ** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(800,600);
	window = glutCreateWindow("glsnake");
    ftime(&last_iteration);
    memcpy(&last_morph, &last_iteration, sizeof(struct timeb));
    srand((unsigned int)last_iteration.time);

	m = rand() % models;
	set_model(0);	
	
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(keyboard_ex);
	glutIdleFunc(idol);
	/*glutMotionFunc(exit); */
	/*glutPassiveMotionFunc(exit);*/ 
	init();
	glutFullScreen();
	atexit(unmain);
	glutSwapBuffers();
	glutMainLoop();
	return 0;
}
