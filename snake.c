/* * * * * * * * * * * * * * * * * * * * * * * *
 *                                             *
 *     -S-     -N-     -A-     -K-     -E-     *
 *                                             *
 *     by Peter Aylett and Andrew Bennetts     *
 *                                             *
 * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdlib.h>
#include <stdio.h>

#include "allegro.h"


#define NUM_NODES          24     /* number of snake QUAD-nodes */
#define NUM_VERTICES       6     /* a node has six corners */
#define NUM_FACES          5     /* eight QUADangles per node */
#define NODE_SPACING       14
#define NODE_HALF_LENGTH   7     /* Usually, the spacing is twice the half length */
#define RIGHT              (64<<16)
#define LEFT               (192<<16)
#define PIN                (128<<16)
#define NUM_MODELS         8

typedef struct VTX
{
   fixed x, y, z;
} VTX;


typedef struct QUAD              /* three vertices makes a QUAD */
{
   VTX *vtxlist;
   int v1, v2, v3, v4;
   int node;
   int hide;
} QUAD;


typedef struct SHAPE             /* store position of a shape */
{
   fixed x, y, z;                /* x, y, z position */
   fixed rx, ry, rz;             /* rotations */
} SHAPE;

typedef struct NODE
{
   SHAPE shape;
   int col;
   fixed ang;
   MATRIX matrix;
} NODE;

VTX points[] =                   /* a node, joins facing down and back */
{
   /* vertices of the QUAD node */
   { -NODE_HALF_LENGTH << 16, -NODE_HALF_LENGTH << 16, -NODE_HALF_LENGTH << 16 },
   { -NODE_HALF_LENGTH << 16, -NODE_HALF_LENGTH << 16,  NODE_HALF_LENGTH << 16 },
   {  NODE_HALF_LENGTH << 16, -NODE_HALF_LENGTH << 16,  NODE_HALF_LENGTH << 16 },
   {  NODE_HALF_LENGTH << 16, -NODE_HALF_LENGTH << 16, -NODE_HALF_LENGTH << 16 },
   { -NODE_HALF_LENGTH << 16,  NODE_HALF_LENGTH << 16,  NODE_HALF_LENGTH << 16 },
   {  NODE_HALF_LENGTH << 16,  NODE_HALF_LENGTH << 16,  NODE_HALF_LENGTH << 16 }
};


int ball[] =
{ 0,LEFT ,LEFT ,RIGHT,LEFT ,RIGHT,RIGHT,LEFT ,
    RIGHT,LEFT ,LEFT ,RIGHT,RIGHT,LEFT ,LEFT ,RIGHT,
    LEFT ,RIGHT,RIGHT,LEFT ,RIGHT,LEFT ,LEFT ,RIGHT};

int cat[] =
{ 0,0, PIN,PIN,0,PIN,PIN,0,RIGHT,
    0,PIN,PIN,0,PIN,PIN,0,PIN,PIN,0,0,0,0,0,0 };

int zigzag1 [] =
{ 0, RIGHT, RIGHT, RIGHT, LEFT , LEFT , LEFT , RIGHT, RIGHT, RIGHT, LEFT , LEFT , LEFT ,
     RIGHT, RIGHT, RIGHT, LEFT , LEFT , LEFT , RIGHT, RIGHT, RIGHT, LEFT , LEFT };

int zigzag2 [] =
{ 0, PIN, 0, PIN, 0, PIN, 0, PIN, 0, PIN, 0, PIN, 0, PIN, 0, PIN, 0, PIN, 0, PIN, 0,
     PIN, 0, PIN};

int zigzag3 [] =
{ 0, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT,
     PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN };

int zigzag3_wrong [] =     /* but try watching a caterpillar do a transition to this! */
{ 0, PIN  , RIGHT, PIN, LEFT , PIN  , RIGHT, PIN, LEFT , PIN  , RIGHT, PIN, LEFT ,
     PIN  , RIGHT, PIN, LEFT , PIN  , RIGHT, PIN, LEFT , PIN  , RIGHT, PIN};

int caterpillar [] =
{ 0, RIGHT, RIGHT, PIN  , LEFT , LEFT , PIN  , RIGHT, RIGHT, PIN  , LEFT , LEFT , PIN  ,
     RIGHT, RIGHT, PIN  , LEFT , LEFT , PIN  , RIGHT, RIGHT, PIN  , LEFT , LEFT };

int bow [] =
{ 0, RIGHT, LEFT , RIGHT, RIGHT, RIGHT, LEFT , LEFT , LEFT ,
     RIGHT, LEFT , RIGHT, RIGHT, RIGHT, LEFT , LEFT , LEFT ,
     RIGHT, LEFT , RIGHT, RIGHT, RIGHT, LEFT};

int snowflake [] =
{ 0, RIGHT, RIGHT, RIGHT, RIGHT, LEFT , LEFT , LEFT , LEFT , RIGHT, RIGHT, RIGHT, RIGHT,
     LEFT , LEFT , LEFT , LEFT , RIGHT, RIGHT, RIGHT, RIGHT, LEFT , LEFT , LEFT};

int turtle [] =
{ 0, RIGHT,RIGHT,LEFT,0,0,RIGHT,LEFT,PIN,RIGHT,RIGHT,LEFT,RIGHT,LEFT,LEFT,PIN,RIGHT,LEFT,0,0,LEFT,LEFT,LEFT,RIGHT };

//int bow [] =
//{ 0, LEFT,LEFT,LEFT,RIGHT,LEFT,RIGHT,RIGHT,RIGHT,LEFT,LEFT,LEFT,RIGHT,LEFT,RIGHT,RIGHT,RIGHT,LEFT,LEFT,LEFT,RIGHT,LEFT,RIGHT,RIGHT };

int basket [] =
{ 0, RIGHT,PIN,0,0,PIN,LEFT,0,LEFT,LEFT,0,LEFT,PIN,0,0,PIN,RIGHT,PIN,LEFT,PIN,0,0,PIN,LEFT };


int thing [] =
{ 0, PIN, RIGHT, LEFT, RIGHT, RIGHT, LEFT,
  PIN, LEFT, RIGHT, LEFT, LEFT, RIGHT,
  PIN, RIGHT, LEFT, RIGHT, RIGHT, LEFT,
  PIN, LEFT, RIGHT, LEFT, LEFT };

     /* Note: this is a 32 node model */
int snowflake32 [] =
{ 0, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT , LEFT , LEFT , LEFT , LEFT,
     RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT , LEFT , LEFT , LEFT , LEFT, RIGHT, RIGHT, RIGHT};

/* Set which models to use initially */
int *model1 = ball;
int *model2 = thing;

//int *models[] = {ball, cat, zigzag1, zigzag2, zigzag3, bow, snowflake, caterpillar, turtle, basket, thing};
int *models[] = {ball, cat, bow, snowflake, caterpillar, turtle, basket, thing};

QUAD faces[] =                   /* group the vertices into polygons */
{
   { points, 0, 4, 1, 0 },   /*side*/
   { points, 2, 5, 3, 2 },   /*side*/
   { points, 0, 3, 5, 4 },  /* hyp */
   { points, 2, 1, 4, 5 }, /* base */
   { points, 0, 1, 2, 3 }  /* front */
//   { points, 0, 0, 0, 0 }
};


NODE nodes[NUM_NODES];        /* a list of shapes */


/* somewhere to put translated vertices */
VTX output_points[NUM_VERTICES * NUM_NODES];
QUAD output_faces[NUM_FACES * NUM_NODES];


/* initialise shape positions */
void init_shapes()
{
   int c;

   for (c=0; c<NUM_NODES; c++) {
      nodes[c].shape.x = 0;
      nodes[c].shape.y = 0;
      nodes[c].shape.z = 0;
      nodes[c].shape.rx = 0;
      nodes[c].shape.ry = 0;
      nodes[c].shape.rz = 0;
      nodes[c].ang = model1[c];
   }
}


/* translate shapes from 3d world space to 2d screen space */
void translate_shapes(int pivotNode)
{
   int c, d, hide;
   MATRIX rmatrix, tmatrix, qmatrix, matrix, smatrix;
   MATRIX prevmatrix, pivotmatrix;
   VTX *outpoint = output_points;
   QUAD *outface = output_faces;

   get_transformation_matrix(&prevmatrix,1<<16,0,90<<16,0,0,0,150<<16);

   get_translation_matrix(&tmatrix, 0,NODE_SPACING<<16,0);

   for (c=0; c<NUM_NODES; c++) {
      /* build a transformation matrix */
      get_y_rotate_matrix(&rmatrix,nodes[c].ang+(128<<16));
      matrix_mul(&tmatrix, &rmatrix, &qmatrix);
      get_x_rotate_matrix(&rmatrix,64<<16);
      matrix_mul(&qmatrix, &rmatrix, &smatrix);
      matrix_mul(&smatrix, &prevmatrix, &matrix);

      prevmatrix = matrix;
      /* output the vertices */
      hide = 0;
      for (d=0; d<NUM_VERTICES; d++) {
	       apply_matrix(&matrix, points[d].x, points[d].y, points[d].z, &outpoint[d].x, &outpoint[d].y, &outpoint[d].z);
	       persp_project(outpoint[d].x, outpoint[d].y, outpoint[d].z, &outpoint[d].x, &outpoint[d].y);
//         if (points[d].z > 0) hide = 1;
	   }

      /* output the faces */
      for (d=0; d<NUM_FACES; d++) {
	       outface[d] = faces[d];
          outface[d].vtxlist = outpoint;
          outface[d].node = c;
          outface[d].hide = hide;

      }

      outpoint += NUM_VERTICES;
      outface += NUM_FACES;
   }

}


/* draw a line (for wireframe display) */
void wire(BITMAP *b, VTX *v1, VTX *v2, int col)
{
//   int col = MID(128, 255 - fixtoi(v1->z+v2->z) / 16, 255);
   line(b, fixtoi(v1->x), fixtoi(v1->y), fixtoi(v2->x), fixtoi(v2->y), col);
}

/* draw a line (for wireframe display) */
int clockwise(VTX *v1, VTX *v2, VTX *v3)
{
   return( ((v1->x-v2->x)*(v3->x-v2->x)-(v1->y-v2->y)*(v3->y-v2->y)) > 0 );
}


/* draw a quad */

void quad(BITMAP *b, VTX *v1, VTX *v2, VTX *v3, VTX *v4, int num )
{
   int col;

   /* four vertices */
   V3D vtx1 = { v1->x, v1->y, v1->z, 0,      0,      0 };
   V3D vtx2 = { v2->x, v2->y, v2->z, 31<<16, 0,      0 };
   V3D vtx3 = { v3->x, v3->y, v3->z, 31<<16, 31<<16, 0 };
   V3D vtx4 = { v4->x, v4->y, v4->z, 0,      31<<16, 0 };

   /* cull backfaces */
   if (polygon_z_normal(&vtx1, &vtx2, &vtx3) < 0)
      return;

   /* set up the vertex color, differently for each rendering mode */
//   switch (mode) {

//      case POLYTYPE_FLAT:
	 col = MID(128, 255 - fixtoi(v1->z+v2->z) / 16, 255) - (128*(num%2));
	 vtx1.c = vtx2.c = vtx3.c = vtx4.c = col;
//	 break;

//      case POLYTYPE_GCOL:
//	 vtx1.c = 0xD0;
//	 vtx2.c = 0x80;
//	 vtx3.c = 0xB0;
//	 vtx4.c = 0xFF;
//	 break;

//      case POLYTYPE_ATEX_LIT:
//      case POLYTYPE_PTEX_LIT:
//	 vtx1.c = MID(0, 255 - fixtoi(v1->z) / 4, 255);
//	 vtx2.c = MID(0, 255 - fixtoi(v2->z) / 4, 255);
//	 vtx3.c = MID(0, 255 - fixtoi(v3->z) / 4, 255);
//	 vtx4.c = MID(0, 255 - fixtoi(v4->z) / 4, 255);
//	 break;
//   }

   /* draw the quad */
   quad3d(b, POLYTYPE_FLAT, NULL, &vtx1, &vtx2, &vtx3, &vtx4);
}


/* callback for qsort() */
int quad_cmp(const void *e1, const void *e2)
{
   QUAD *q1 = (QUAD *)e1;
   QUAD *q2 = (QUAD *)e2;

   fixed d1 = q1->vtxlist[q1->v1].z + q1->vtxlist[q1->v2].z +
	      q1->vtxlist[q1->v3].z + q1->vtxlist[q1->v4].z;

   fixed d2 = q2->vtxlist[q2->v1].z + q2->vtxlist[q2->v2].z +
	      q2->vtxlist[q2->v3].z + q2->vtxlist[q2->v4].z;

   return d2 - d1;
}


/* draw the shapes calculated by translate_shapes() */
void draw_shapes(BITMAP *b)
{
   int c;
   QUAD *face = output_faces;
   VTX *v1, *v2, *v3, *v4;

   /* depth sort */
   qsort(output_faces, NUM_FACES * NUM_NODES, sizeof(QUAD), quad_cmp);

   for (c=0; c < NUM_FACES * NUM_NODES; c++) {

      /* find the vertices used by the face */
      v1 = face->vtxlist + face->v1;
      v2 = face->vtxlist + face->v2;
      v3 = face->vtxlist + face->v3;
      v4 = face->vtxlist + face->v4;

/*      if (!clockwise(v1,v2,v3)) {

               wire(b, v1, v2,(int)(c/NUM_FACES)+1);
	            wire(b, v2, v3,(int)(c/NUM_FACES)+1);
               wire(b, v3, v4,(int)(c/NUM_FACES)+1);
               wire(b, v4, v1,(int)(c/NUM_FACES)+1);
               }
               */
       if (!face->hide) quad(b, v1,v2,v3,v4, face->node );

               wire(b, v1, v2,0);
	            wire(b, v2, v3,0);
               wire(b, v3, v4,0);
               wire(b, v4, v1,0);

      face++;
   }
}


void print_progress(int pos)
{
   if ((pos & 3) == 3) {
      printf("*");
      fflush(stdout);
   }
}

int mouse_moved ()
{
   int x, y;

   get_mouse_mickeys(&x,&y);
   return (x | y);

}

int main2()
{
   BITMAP *buffer;
   PALLETE pal;
   int c, w, h, a,j;
   int last_retrace_count;

   allegro_init();
   install_keyboard();
   install_mouse();
   install_timer();

   srand(time(NULL));

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

      while (!keypressed() & !mouse_moved()) {
//     while (!key[KEY_ESC]) {

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

/*         if (key[KEY_P] ) {
            while (key[KEY_P]) {}
            while (!key[KEY_P]) {}
         }
*/

      }
      destroy_bitmap(buffer);

      return 0;
}

int main()
{
   BITMAP *buffer;
   PALLETE pal;
   int c, w, h, a,j;
   int last_retrace_count;

   allegro_init();
   install_keyboard();
   install_mouse();
   install_timer();

   srand(time(NULL));

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

