/* $Id: glsnake.c,v 1.6 2001/10/04 16:25:01 jaq Exp $
 * An OpenGL imitation of Rubik's Snake 
 * by Jamie Wilkinson and Andrew Bennetts
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
#define RIGHT   -90.0
#define PIN     180.0
#define LEFT    90.0

#define ROTATION_RATE1 0.10	  /* Rotations per second */
#define ROTATION_RATE2 0.14	  /* Rotations per second */
#define EXPLODE_INCREMENT 0.05
#define MORPH_DELAY 3.0		  /* Delay in seconds between morphs */
#define MORPH_RATE  0.25	  /* Morphs per second */

/* the id for the window we make */
int window;

/* the id of the display lists for drawing a node */
int node_solid, node_wire;

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

float hyperbola[] = { RIGHT, PIN, LEFT, ZERO, RIGHT, ZERO, LEFT, PIN, RIGHT,
	ZERO, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, ZERO, LEFT, PIN,
	RIGHT, ZERO, LEFT, ZERO };

float propellor[] = { ZERO, ZERO, ZERO, RIGHT, LEFT, RIGHT, ZERO, LEFT, ZERO,
	ZERO, ZERO, RIGHT, LEFT, RIGHT, ZERO, LEFT, ZERO, ZERO, ZERO, RIGHT,
	LEFT, RIGHT, ZERO, LEFT };

float hexagon[] = { ZERO, ZERO, ZERO, ZERO, LEFT, ZERO, ZERO, RIGHT, ZERO,
	ZERO, ZERO, ZERO, LEFT, ZERO, ZERO, RIGHT, ZERO, ZERO, ZERO, ZERO,
	LEFT, ZERO, ZERO, RIGHT };

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


float * model[] = { ball, cat, zigzag1, zigzag2, zigzag3, bow, snowflake,
	caterpillar, turtle, basket, thing, straight, hyperbola, propellor,
	hexagon, triangle, flower, crucifix, kayak, bird, seal, dog, frog,
	half_balls };


int models = sizeof(model) / sizeof(float *);
int m, m_next;

/* rotation angle */
float rotang1 = 0.0;
float rotang2 = 0.0;

/* morph ratio (how far between model m and model m_next we are) */
float morph = 0.0;
float ma_morph = 0.0;

struct timeb last_iteration;
struct timeb last_morph;


/* option variables */
float explode = 0.05;
int wireframe = 0;

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
	
	/* build a display list */
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
		ang = model[m][i] * (1.0 - morph) + model[m_next][i] * morph;
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
		if (i % 2)
			glColor3f(0.0,0.0,1.0);
		else
			glColor3f(1.0,1.0,1.0);
		/*glColor3b(rand() % 255,rand() % 255,rand() % 255); */
		/*glColor3b(i * 2341 % 255,i * 312 % 255,i * 5213 % 255); */ 

		if (wireframe)
			glCallList(node_wire);
		else {
			glCallList(node_solid);
			/*glColor3f(0.0,0.0,0.0);
			glCallList(node_wire);
			*/
		}

		/* Interpolate between models */
		ang = model[m][i] * (1.0 - morph) + model[m_next][i] * morph;
		
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
		case 'n':
			/* Guard against changing models during a morph */
			if (morph == 0.0) {		
				m = m_next;
				++m_next;
				m_next %= models;
				/* Reset last_morph time */
				ftime(&last_morph);
			}
			
			break;
		case 'w':
			wireframe = 1 - wireframe;
			if (wireframe)
				glDisable(GL_LIGHTING);
			else
				glEnable(GL_LIGHTING);
			break;
		default:
			break;
	}
}

/* "jwz?  no way man, he's my idle" -- me, 2001.  I forget the context :( */
void idol(void) {
        long i_sec, i_usec;             /* used for tracking how far through an iteration we are */
        long m_sec, m_usec;
        i_sec = (long)-last_iteration.time;
        i_usec = -last_iteration.millitm;
        ftime(&last_iteration);
        i_sec += (long)last_iteration.time;
        i_usec += last_iteration.millitm;
        i_usec += i_sec*1000;

        rotang1 += 360/((1000/ROTATION_RATE1)/i_usec);
        rotang2 += 360/((1000/ROTATION_RATE2)/i_usec);

        /* do the morphing stuff here */
        m_sec = (long)(last_iteration.time - last_morph.time);
        m_usec = last_iteration.millitm - last_morph.millitm;
        m_usec += m_sec*1000;
        if (m_usec > (long)(MORPH_DELAY*1000)) {
                morph = (m_usec - MORPH_DELAY*1000) * (MORPH_RATE/1000);
                if (morph > 1.0) {
                        morph = 0.0;
                        m = m_next;
                        m_next = rand() % models;
                        memcpy(&last_morph, &last_iteration, sizeof(struct timeb));
                }
        }

	/*
	 * Win32 hack, hopefully redundant now.
 
	rotang1 += 2; 
	rotang2 += 3; 
	
	ma_morph += 0.02;
	if (ma_morph > 2.0) {
		ma_morph = 0.0;
		m = m_next;
		m_next = rand() % models;
	}
	morph = 1.0 < ma_morph ? 1.0 : ma_morph; 
//	morph = 0.0;
	*/
	glutSwapBuffers();
	glutPostRedisplay();
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
	m_next = rand() % models;
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
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
