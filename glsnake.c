/* $Id: glsnake.c,v 1.1 2001/10/04 16:13:37 jaq Exp $
 * An OpenGL imitation of Rubik's Snake
 * based on Allegro code by Peter Aylett and Andrew Bennetts
 */

#include <GL/glut.h>

#if 0
#define NUM_NODES          24     /* number of snake QUAD-nodes */
#define NUM_VERTICES       6     /* a node has six corners */
#define NUM_FACES          5     /* eight QUADangles per node */
#define NODE_SPACING       14
#define NODE_HALF_LENGTH   7     /* Usually, the spacing is twice the half length */
#define RIGHT              (64<<16)
#define LEFT               (192<<16)
#define PIN                (128<<16)
#define NUM_MODELS         8
#endif
#define ZERO	0.0
#define RIGHT   -90.0
#define PIN     180.0
#define LEFT    90.0
/*
typedef struct VTX
{
   fixed x, y, z;
} VTX;
*/

/* the id for the window we make */
int window;

/* the id of the display lists for drawing a node */
int node_solid, node_wire;

/* the triangular prism what makes up the basic unit */
float prism_v[][3] = { { 0.0, 0.0, 1.0 }, 
					  { 1.0, 0.0, 1.0 },
					  { 0.0, 1.0, 1.0 },
					  { 0.0, 0.0, 0.0 },
					  { 1.0, 0.0, 0.0 },
					  { 0.0, 1.0, 0.0 }};

/* the actual models */

float ball[] = { RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT};

float cat[] = { ZERO, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO };

float zigzag1[] = { ZERO, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT };

float zigzag2[] = { ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN};

float zigzag3[] = { ZERO, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN };

 /* but try watching a caterpillar do a transition to this! */
float zigzag3_wrong[] = { ZERO, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, PIN};

float caterpillar[] = { ZERO, RIGHT, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, LEFT };

float bow[] = { ZERO, RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT};

float snowflake[] = { ZERO, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT};

float turtle[] = { ZERO, RIGHT, RIGHT, LEFT, ZERO, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, PIN, RIGHT, LEFT, ZERO, ZERO, LEFT, LEFT, LEFT, RIGHT };

//float bow[] =
//{ ZERO, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT };

float basket[] = { ZERO, RIGHT, PIN, ZERO, ZERO, PIN, LEFT, ZERO, LEFT, LEFT, ZERO, LEFT, PIN, ZERO, ZERO, PIN, RIGHT, PIN, LEFT, PIN, ZERO, ZERO, PIN, LEFT };


float thing[] = { ZERO, PIN, RIGHT, LEFT, RIGHT, RIGHT, LEFT, PIN, LEFT, RIGHT, LEFT, LEFT, RIGHT, PIN, RIGHT, LEFT, RIGHT, RIGHT, LEFT, PIN, LEFT, RIGHT, LEFT, LEFT };

 /* Note: this is a 32 node model */
float snowflake32[] = { ZERO, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT};

//int *models[] = {ball, cat, zigzag1, zigzag2, zigzag3, bow, snowflake, caterpillar, turtle, basket, thing};
/*int *models[] = {ball, cat, bow, snowflake, caterpillar, turtle, basket, thing};*/

float * model[] = { ball, cat, zigzag1, zigzag2, zigzag3, bow, snowflake, caterpillar, turtle, basket, thing };

int models = sizeof(model) / sizeof(float *);
int m = 0;

/*
float ang[] = {0.0,LEFT, RIGHT, 0.0, LEFT, RIGHT, ZERO, LEFT, RIGHT, ZERO, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT};
*/

/* rotation angle */
float rotang = 0.0;

/* option variables */
float explode = 0.0;
int wireframe = 0;


/* wot initialises it */
void init(void) {
	/*
	float light_pos[] = {0.0, 0.0, 1.0};
	float light_dir[] = {0.0, 0.0, -1.0};
	*/
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
	gluPerspective(45.0, 640/480.0, 0.05, 100.0);
	gluLookAt(0.0, 0.0, 20.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	glMatrixMode(GL_MODELVIEW);
	/* set up lighting */
	/*
	glColor3f(1.0, 1.0, 1.0);
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_dir);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	*/
	/* build a display list */
	node_solid = glGenLists(1);
	glNewList(node_solid, GL_COMPILE);
	glBegin(GL_TRIANGLES);
	glVertex3fv(prism_v[0]);
	glVertex3fv(prism_v[1]);
	glVertex3fv(prism_v[2]);

	glVertex3fv(prism_v[3]);
	glVertex3fv(prism_v[5]);
	glVertex3fv(prism_v[4]);
	glEnd();
	
	glBegin(GL_QUADS);
	glVertex3fv(prism_v[1]);
	glVertex3fv(prism_v[0]);
	glVertex3fv(prism_v[3]);
	glVertex3fv(prism_v[4]);
	
	glVertex3fv(prism_v[2]);
	glVertex3fv(prism_v[1]);
	glVertex3fv(prism_v[4]);
	glVertex3fv(prism_v[5]);
	
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
	
	/* clear the buffer */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	/* go into the modelview stack */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	/* draw this dang thing */

	/* origin */
	glColor3f(1.0,1.0,0.0);
	glPointSize(4.0);
	glBegin(GL_POINTS);
	glVertex3f(0.0, 0.0, 0.0);
	glEnd();

	glRotatef(rotang, 1.0,1.0,1.0);

	/* rotate and translate into snake space */

	/* glTranslatef(8.0,0.0,0.0); */
	glRotatef(45.0,0.0,0.0,1.0);

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
			glColor3f(0.0,0.0,0.0);
			glCallList(node_wire);
		}
		glRotatef(180.0, 0.0,0.0,1.0);
		if (i % 2) {
			glTranslatef(-1.0,explode,0.0);
			/* rotation of the joint */
			glTranslatef(0.5,0.0,0.5);
			glRotatef(model[m][i], 0.0, 1.0, 0.0);
			glTranslatef(-0.5,0.0,-0.5);
		} else {
			glTranslatef(explode,-1.0,0.0);
			/* rotation of the joint */
			glTranslatef(0.0,0.5,0.5);
			glRotatef(model[m][i], 1.0, 0.0, 0.0);
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
			/* mmm, hardcoded */
			explode = 0.2 - explode;
			break;
		case 'n':
			m++;
			if (m >= models)
				m = 0;
			break;
		case 'w':
			wireframe = 1 - wireframe;
			break;
		default:
			break;
	}
}

/* "jwz?  no way man, he's my idle" -- me, 2001.  I forget the context :( */
void idol(void) {
	rotang += 0.05;
	/* do the morphing stuff here */

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

#if 0
   /* color 0 = black */
   pal[0].r = pal[0].g = pal[0].b = 0;

   /* copy the desktop pallete */
   for (c=1; c<64; c++)
      pal[c] = desktop_pallete[c];

   /* make a red gradient */
   for (c=64; c<96; c++) {
      pal[c].r = (c-64)*2;
      pal[c].g = pal[c].b = 0;
   }

   /* make a green gradient */
   for (c=96; c<128; c++) {
      pal[c].g = (c-96)*2;
      pal[c].r = pal[c].b = 0;
   }

   /* set up a greyscale in the top half of the pallete */
   for (c=128; c<256; c++)
      pal[c].r = pal[c].g = pal[c].b = (c-128)/2;

      /* set up a bluescale in the top half of the pallete */
   for (c=0; c<128; c++) {
      pal[c].r = pal[c].g = 0; pal[c].b = (c-128)/2; }

   /* set the graphics mode */
   set_gfx_mode(GFX_AUTODETECT, 800, 600, 0, 0);


   set_pallete(pal);

   /* double buffer the animation */
   buffer = create_bitmap(SCREEN_W, SCREEN_H);

   /* set up the viewport for the perspective projection */
   set_projection_viewport(0, 0, SCREEN_W, SCREEN_H);

   /* initialise the bouncing shapes */
   init_shapes();

   last_retrace_count = retrace_count;

     for (a=0; a<24; a++) nodes[a].ang=model1[a];
      j = -32;

//      while (!keypressed() & !mouse_moved()) {
     while (!key[KEY_ESC]) {

         j++;
         clear(buffer);

         translate_shapes(12);
         draw_shapes(buffer);

         if (j == -32) {
            model2 = models[rand() % (NUM_MODELS - 1)];
         }
         else if (j == 160) {
            model1 = models[rand() % (NUM_MODELS - 1)];
         }


         if ((ABS(j)) <= 160 && (ABS(j)) >= 32)
            for (a=1; a<24; a++)
                nodes[a].ang = ((model1[a] * (128-(ABS(j)-32)))>>7)+((model2[a] * (ABS(j)-32))>>7);
                //nodes[a].ang + (1<<16);

         nodes[0].ang = nodes[0].ang + (3<<16);
         /*      nodes[1].ang = nodes[1].ang + (4<<16);
                 nodes[2].ang = nodes[2].ang + (5<<16);
                 nodes[3].ang = nodes[3].ang + (7<<16);
         */
         vsync();
         blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

         if (j == 192) j = -192;

           if (key[KEY_S] ) {
              save_bitmap("output.bmp", buffer, pal);
           }

         if (key[KEY_P] || key[KEY_S] ) {
            while (key[KEY_P]) {}
            while (!key[KEY_P]) {}
         }


      }
      destroy_bitmap(buffer);

      return 0;
}
#endif
