/* $Id: glsnake.c,v 1.3 2001/10/04 16:18:21 jaq Exp $
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
#include <sys/time.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* angles */
#define ZERO	0.0
#define RIGHT   -90.0
#define PIN     180.0
#define LEFT    90.0

#define ROTATION_RATE 0.25	  /* Rotations per second */
#define EXPLODE_INCREMENT 0.1
#define MORPH_DELAY 2.0		  /* Delay in seconds between morphs */
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

float ball[] = { RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT};

float cat[] = { ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO };

float zigzag1[] = { RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT };

float zigzag2[] = { PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN};

float zigzag3[] = { PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN };

/* this model sucks */
/* but try watching a caterpillar do a transition to this! */
/*
float zigzag3_wrong[] = { PIN, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, PIN};
*/

float caterpillar[] = { RIGHT, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, LEFT };

float bow[] = { RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT};

float snowflake[] = { RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT};

float turtle[] = { RIGHT, RIGHT, LEFT, ZERO, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, PIN, RIGHT, LEFT, ZERO, ZERO, LEFT, LEFT, LEFT, RIGHT };

float basket[] = { RIGHT, PIN, ZERO, ZERO, PIN, LEFT, ZERO, LEFT, LEFT, ZERO, LEFT, PIN, ZERO, ZERO, PIN, RIGHT, PIN, LEFT, PIN, ZERO, ZERO, PIN, LEFT };

float thing[] = { PIN, RIGHT, LEFT, RIGHT, RIGHT, LEFT, PIN, LEFT, RIGHT, LEFT, LEFT, RIGHT, PIN, RIGHT, LEFT, RIGHT, RIGHT, LEFT, PIN, LEFT, RIGHT, LEFT, LEFT };

/* Note: this is a 32 node model
float snowflake32[] = { RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT};
*/

float straight[] = { ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO,
	ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO,
	ZERO, ZERO };

float * model[] = { ball, cat, zigzag1, zigzag2, zigzag3, bow, snowflake, caterpillar, turtle, basket, thing, straight };

int models = sizeof(model) / sizeof(float *);
int m, m_next;

/* rotation angle */
float rotang = 0.0;

/* morph ratio (how far between model m and model m_next we are) */
float morph = 0.0;

/* option variables */
float explode = 0.1;
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
	
	/* draw this dang thing */
	
	/* the origin */
	glPointSize(4.0);
	glBegin(GL_POINTS);
	glColor3f(1.0,1.0,0.0);
	glVertex3f(0.0,0.0,0.0);
	glEnd();

	/* rotate and translate into snake space */
	glRotatef(45.0,0.0,0.0,1.0);
	glTranslatef(-0.5,0.0,0.5);
	
	/* rotate the 0th junction */
	glTranslatef(0.5,0.0,0.5);
	glRotatef(rotang, 0.0,1.0,0.0); 
	glTranslatef(-0.5,0.0,-0.5);

	/* now draw each node along the snake -- this is quite ugly :p */

	for (i = 0; i < 24; i++) {
		glPushMatrix();
		if (i % 2)
			glColor3f(0.0,0.0,1.0);
		else
			glColor3f(1.0,1.0,1.0);
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
			m = m_next;
			m_next = rand() % models;
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

struct timeval last_iteration;
struct timeval last_morph;

/* "jwz?  no way man, he's my idle" -- me, 2001.  I forget the context :( */
void idol(void) {
	long i_sec, i_usec;		/* used for tracking how far through an iteration we are */
	long m_sec, m_usec;
	i_sec = -last_iteration.tv_sec;
	i_usec = -last_iteration.tv_usec;
	gettimeofday(&last_iteration,NULL);
	i_sec += last_iteration.tv_sec;
	i_usec += last_iteration.tv_usec;
	i_usec += i_sec*1000000;

	rotang += 360/((1000000/ROTATION_RATE)/i_usec);
	
	/* do the morphing stuff here */
	m_sec = last_iteration.tv_sec - last_morph.tv_sec;
	m_usec = last_iteration.tv_usec - last_morph.tv_usec;
	m_usec += m_sec*1000000;
	if (m_usec > (long)(MORPH_DELAY*1000000)) {
		morph = (m_usec - MORPH_DELAY*1000000) * (MORPH_RATE/1000000);
		if (morph > 1.0) {
			morph = 0.0;
			m = m_next;
			m_next = rand() % models;
			memcpy(&last_morph, &last_iteration, sizeof(struct timeval));
		}
	}
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
	glutInitWindowSize(640, 480);
	window = glutCreateWindow("glsnake");
	gettimeofday(&last_iteration, NULL);
	memcpy(&last_morph, &last_iteration, sizeof(struct timeval));
	srand((unsigned int)last_iteration.tv_usec);
	m = rand() % models;
	m_next = rand() % models;
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(idol);
	init();
	atexit(unmain);
	glutSwapBuffers();
	glutMainLoop();
	return 0;
}
