/* glsnake.c - OpenGL imitation of Rubik's Snake
 *
 * (c) 2001-2005 Jamie Wilkinson <jaq@spacepants.org>
 * (c) 2001-2003 Andrew Bennetts <andrew@puzzling.org>
 * (c) 2001-2006 Peter Aylett <aylett@gmail.com>
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

#ifdef WIN32
#include <windows.h>
#define isnan _isnan
#define inline __inline
#define random rand
#define ATTRIBUTE_UNUSED
#endif /* WIN32 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* HAVE_GLUT defined if we're building a standalone glsnake,
 * and not defined if we're building as an xscreensaver hack */
#ifdef HAVE_GLUT
#include <GL/glut.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

/* angles */
#define ZERO 0.0
#define LEFT 90.0
#define PIN 180.0
#define RIGHT 270.0

#ifdef HAVE_GETTIMEOFDAY
#ifdef GETTIMEOFDAY_TWO_ARGS

#include <sys/time.h>
#include <time.h>
typedef struct timeval snaketime;
#define GETSECS(t) ((t).tv_sec)
#define GETMSECS(t) ((t).tv_usec / 1000)

#else /* !GETTIMEOFDAY_TWO_ARGS */

#include <sys/time.h>
#include <time.h>
typedef struct timeval snaketime;
#define GETSECS(t) ((t).tv_sec)
#define GETMSECS(t) ((t).tv_usec / 1000)

#endif /* GETTIMEOFDAY_TWO_ARGS */

#else /* !HAVE_GETTIMEOFDAY */
#ifdef HAVE_FTIME

#include <sys/timeb.h>
typedef struct timeb snaketime;
#define GETSECS(t) ((long)(t).time)
#define GETMSECS(t) ((t).millitm)
#endif /* HAVE_FTIME */
#endif /* HAVE_GETTIMEOFDAY */

#include <math.h>

#ifndef M_SQRT1_2 /* Win32 doesn't have this constant  */
#define M_SQRT1_2 0.70710678118654752440084436210485
#endif

#ifndef ATTRIBUTE_UNUSED
#define ATTRIBUTE_UNUSED __attribute__((__unused__))
#endif

#define NODE_COUNT 24

#ifdef HAVE_GLUT
#define DEF_YANGVEL 0.10
#define DEF_ZANGVEL 0.14
#define DEF_EXPLODE 0.03
#define DEF_ANGVEL 1.0
#define DEF_STATICTIME 5000
#define DEF_ALTCOLOUR 0
#define DEF_TITLES 1
#define DEF_INTERACTIVE 0
#define DEF_ZOOM 25.0
#define DEF_WIREFRAME 0
#define DEF_TRANSPARENT 1
#else
/* xscreensaver options doobies prefer strings */
#define DEF_YANGVEL "0.10"
#define DEF_ZANGVEL "0.14"
#define DEF_EXPLODE "0.03"
#define DEF_ANGVEL "1.0"
#define DEF_STATICTIME "5000"
#define DEF_ALTCOLOUR "False"
#define DEF_TITLES "True"
#define DEF_INTERACTIVE "False"
#define DEF_ZOOM "25.0"
#define DEF_WIREFRAME "False"
#define DEF_TRANSPARENT "True"
#endif

/* static variables */
#ifndef HAVE_GLUT
#include <X11/Intrinsic.h>
#else
/* xscreensaver boolean type */
#define Bool int
#endif

static GLfloat explode;
static long statictime;
static GLfloat yspin = 60.0;
static GLfloat zspin = -45.0;
static GLfloat yangvel;
static GLfloat zangvel;
static Bool altcolour;
static Bool titles;
static Bool interactive;
static Bool wireframe;
static Bool transparent;
static GLfloat zoom;
static GLfloat angvel;

#ifndef HAVE_GLUT
/* xscreensaver setup */
extern XtAppContext app;

#define PROGCLASS "glsnake"
#define HACK_INIT glsnake_init
#define HACK_DRAW glsnake_display
#define HACK_RESHAPE glsnake_reshape
#define sws_opts xlockmore_opts

/* xscreensaver defaults */
#define DEFAULTS                                   \
  "*delay:          30000                      \n" \
  "*count:          30                         \n" \
  "*showFPS:        False                      \n" \
  "*labelfont:   -*-times-bold-r-normal-*-180-*\n"

#undef countof
#define countof(x) (sizeof((x)) / sizeof((*x)))

#include "xlockmore.h"
#include "glxfonts.h"

static XrmOptionDescRec opts[] = {
    {"-explode", ".explode", XrmoptionSepArg, DEF_EXPLODE},
    {"-angvel", ".angvel", XrmoptionSepArg, DEF_ANGVEL},
    {"-statictime", ".statictime", XrmoptionSepArg, DEF_STATICTIME},
    {"-yangvel", ".yangvel", XrmoptionSepArg, DEF_YANGVEL},
    {"-zangvel", ".zangvel", XrmoptionSepArg, DEF_ZANGVEL},
    {"-altcolour", ".altcolour", XrmoptionNoArg, (caddr_t) "True"},
    {"-no-altcolour", ".altcolour", XrmoptionNoArg, (caddr_t) "False"},
    {"-titles", ".titles", XrmoptionNoArg, (caddr_t) "True"},
    {"-no-titles", ".titles", XrmoptionNoArg, (caddr_t) "False"},
    {"-zoom", ".zoom", XrmoptionSepArg, DEF_ZOOM},
    {"-wireframe", ".wireframe", XrmoptionNoArg, (caddr_t) "true"},
    {"-no-wireframe", ".wireframe", XrmoptionNoArg, (caddr_t) "false"},
    {"-transparent", ".transparent", XrmoptionNoArg, (caddr_t) "true"},
    {"-no-transparent", ".transparent", XrmoptionNoArg, (caddr_t) "false"},
};

static argtype vars[] = {
    {&explode, "explode", "Explode", DEF_EXPLODE, t_Float},
    {&angvel, "angvel", "Angular Velocity", DEF_ANGVEL, t_Float},
    {&statictime, "statictime", "Static Time", DEF_STATICTIME, t_Int},
    {&yangvel, "yangvel", "Angular Velocity about Y axis", DEF_YANGVEL,
     t_Float},
    {&zangvel, "zangvel", "Angular Velocity about X axis", DEF_ZANGVEL,
     t_Float},
    {&interactive, "interactive", "Interactive", DEF_INTERACTIVE, t_Bool},
    {&altcolour, "altcolour", "Alternate Colour Scheme", DEF_ALTCOLOUR, t_Bool},
    {&titles, "titles", "Titles", DEF_TITLES, t_Bool},
    {&zoom, "zoom", "Zoom", DEF_ZOOM, t_Float},
    {&wireframe, "wireframe", "Wireframe", DEF_WIREFRAME, t_Bool},
    {&transparent, "transparent", "Transparent!", DEF_TRANSPARENT, t_Bool},
};

ModeSpecOpt sws_opts = {(int)countof(opts), opts, (int)countof(vars), vars,
                        NULL};
#endif

struct glsnake_shape {
  float node[NODE_COUNT];
};

struct model_s {
  const char *name;
  struct glsnake_shape shape;
};

#ifdef HAVE_GLUT
/* Define a ring buffer to store previous snake shapes.  The 'u' key will go
 * back to the previously stored state. */
#define UNDO_LENGTH 100
struct glsnake_shape undo_ring_buffer[UNDO_LENGTH];
int undo_ring_start;
int undo_ring_end;
#endif

typedef int (*morph_func_t)(long);
typedef float (*morph_percent_func_t)(void);

struct glsnake_cfg {
#ifndef HAVE_GLUT
  GLXContext *glx_context;
  XFontStruct *font;
  GLuint font_list;
#else
  /* font list number */
  int font;
#endif

  /* window id */
  int window;

  /* is a morph in progress? */
  int morphing;

  /* functions for this morph */
  morph_func_t morph;
  morph_percent_func_t morph_percent;

  /* has the model been paused? */
  int paused;

  /* snake metrics */
  int is_cyclic;
  int is_legal;
  float last_turn;
  int debug;

  /* the current shape of the model */
  struct glsnake_shape shape;

  /* the shapes we are morphing from and morphing to */
  struct model_s prev_model_s;
  struct model_s next_model_s;

  /* currently selected node for interactive mode */
  int selected;

  /* if next_model_s is from the preset model array, this is the index into
   * that array, otherwise -1. */
  int preset_index;

  /* model morphing */
  int new_morph;

  /* colours */
  float colour[2][4];
  int next_colour;
  int prev_colour;

  /* timing variables */
  snaketime last_iteration;
  snaketime last_morph;

  /* window size */
  int width, height;
  int old_width, old_height;

  /* the id of the display lists for drawing a node */
  GLuint node_solid, node_wire;

  /* is the window fullscreen? */
  int fullscreen;
};

#define COLOUR_CYCLIC 0
#define COLOUR_ACYCLIC 1
#define COLOUR_INVALID 2
#define COLOUR_AUTHENTIC 3
#define COLOUR_ORIGLOGO 4
#define COLOUR_SPOOKY 5

static float colour[][2][4] = {
    /* cyclic - green */
    {{0.4, 0.8, 0.2, 0.6}, {1.0, 1.0, 1.0, 0.6}},
    /* acyclic - blue */
    {{0.3, 0.1, 0.9, 0.6}, {1.0, 1.0, 1.0, 0.6}},
    /* invalid - grey */
    {{0.3, 0.1, 0.9, 0.6}, {1.0, 1.0, 1.0, 0.6}},
    /* authentic - purple and green */
    {{0.38, 0.0, 0.55, 0.7}, {0.0, 0.5, 0.34, 0.7}},
    /* old "authentic" colours from the logo */
    {{171 / 255.0, 0, 1.0, 1.0}, {46 / 255.0, 205 / 255.0, 227 / 255.0, 1.0}},
    /* Spooky */
    {{1.0, 0x99 / 255.0, 0.0, 1.0}, {0.1, 0.1, 0.1, 1.0}},
};

/* Dearest reader,
 *
 *   If the models contained within cause offence, you can do one of two
 * things:
 *
 * 1) Complain about it in bug reports and web forums, sharing war stories
 *    of how you were humiliated when you were giving a career-making
 *    presentation to your peers, or company board, and the screensaver
 *    kicked in, or
 *
 * 2) Send a patch to the authors.
 *
 * Contemptuously yours,
 *
 *   Jamie
 */
static struct model_s model[] = {
#define STRAIGHT_MODEL 0
    {"straight",
     {{ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO,
       ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO,
       ZERO}}},
    /* the models in the Rubik's snake manual */
    {"ball",
     {{RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT,
       LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT,
       LEFT, ZERO}}},
#define START_MODEL 2
    {"snow",
     {{RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT,
       RIGHT, LEFT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT,
       LEFT, ZERO}}},
    {"propellor",
     {{ZERO, ZERO, ZERO, RIGHT, LEFT, RIGHT, ZERO, LEFT, ZERO, ZERO, ZERO,
       RIGHT, LEFT, RIGHT, ZERO, LEFT, ZERO, ZERO, ZERO, RIGHT, LEFT, RIGHT,
       ZERO, LEFT}}},
    {"flamingo",
     {{ZERO, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, PIN, RIGHT, RIGHT, PIN, RIGHT,
       LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, RIGHT, ZERO, ZERO, ZERO, PIN,
       ZERO}}},
    {"cat",
     {{ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, PIN,
       PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO}}},
    {"rooster",
     {{ZERO, ZERO, PIN, PIN, ZERO, LEFT, ZERO, LEFT, RIGHT, PIN, RIGHT, ZERO,
       PIN, PIN, ZERO, RIGHT, PIN, RIGHT, LEFT, ZERO, LEFT, ZERO, PIN, ZERO}}},
    /* These models were taken from Andrew and Peter's original snake.c
     * as well as some newer ones made up by Jamie, Andrew and Peter. */
    {"half balls",
     {{LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, LEFT,
       LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT,
       LEFT, ZERO}}},
    {"zigzag1",
     {{RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT,
       LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT,
       LEFT, ZERO}}},
    {"zigzag2",
     {{PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN,
       ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO}}},
    {"zigzag3",
     {{PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN,
       LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN, ZERO}}},
    {"caterpillar",
     {{RIGHT, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, LEFT, PIN,
       RIGHT, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, LEFT,
       ZERO}}},
    {"bow",
     {{RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT,
       RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT,
       LEFT, ZERO}}},
    {"turtle",
     {{ZERO, RIGHT, LEFT, ZERO, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, LEFT,
       RIGHT, LEFT, LEFT, PIN, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT,
       RIGHT, ZERO}}},
    {"basket",
     {{RIGHT, PIN, ZERO, ZERO, PIN, LEFT, ZERO, LEFT, LEFT, ZERO, LEFT, PIN,
       ZERO, ZERO, PIN, RIGHT, PIN, LEFT, PIN, ZERO, ZERO, PIN, LEFT, ZERO}}},
    {"thing",
     {{PIN, RIGHT, LEFT, RIGHT, RIGHT, LEFT, PIN, LEFT, RIGHT, LEFT, LEFT,
       RIGHT, PIN, RIGHT, LEFT, RIGHT, RIGHT, LEFT, PIN, LEFT, RIGHT, LEFT,
       LEFT, ZERO}}},
    {"hexagon",
     {{ZERO, ZERO, ZERO, ZERO, LEFT, ZERO, ZERO, RIGHT, ZERO, ZERO, ZERO, ZERO,
       LEFT, ZERO, ZERO, RIGHT, ZERO, ZERO, ZERO, ZERO, LEFT, ZERO, ZERO,
       RIGHT}}},
    {"tri1",
     {{ZERO, ZERO, LEFT, RIGHT, ZERO, LEFT, ZERO, RIGHT, ZERO, ZERO, LEFT,
       RIGHT, ZERO, LEFT, ZERO, RIGHT, ZERO, ZERO, LEFT, RIGHT, ZERO, LEFT,
       ZERO, RIGHT}}},
    {"triangle",
     {{ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, LEFT, RIGHT, ZERO, ZERO, ZERO, ZERO,
       ZERO, ZERO, LEFT, RIGHT, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, LEFT,
       RIGHT}}},
    {"flower",
     {{ZERO, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, LEFT, PIN, RIGHT, RIGHT, PIN,
       ZERO, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, LEFT, PIN, RIGHT, RIGHT,
       PIN}}},
    {"crucifix",
     {{ZERO, PIN, PIN, ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN, ZERO, PIN, PIN,
       ZERO, PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO, ZERO, ZERO, PIN}}},
    {"kayak",
     {{PIN, RIGHT, LEFT, PIN, LEFT, PIN, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO,
       ZERO, ZERO, ZERO, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, PIN,
       RIGHT}}},
    {"bird",
     {{ZERO, ZERO, ZERO, ZERO, RIGHT, RIGHT, ZERO, LEFT, PIN, RIGHT, ZERO,
       RIGHT, ZERO, RIGHT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, LEFT, ZERO, PIN,
       ZERO}}},
    {"seal",
     {{RIGHT, LEFT, LEFT, PIN, RIGHT, LEFT, ZERO, PIN, PIN, ZERO, LEFT, ZERO,
       LEFT, PIN, RIGHT, ZERO, LEFT, LEFT, LEFT, PIN, RIGHT, RIGHT, LEFT,
       ZERO}}},
    {"dog",
     {{ZERO, ZERO, ZERO, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO, PIN, ZERO, PIN,
       PIN, ZERO, ZERO, ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN, ZERO}}},
    {"frog",
     {{RIGHT, RIGHT, LEFT, LEFT, RIGHT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, ZERO,
       LEFT, ZERO, LEFT, PIN, RIGHT, ZERO, LEFT, LEFT, RIGHT, LEFT, LEFT,
       ZERO}}},
    {"quavers",
     {{LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, ZERO, ZERO, ZERO, RIGHT, ZERO,
       ZERO, LEFT, RIGHT, ZERO, ZERO, ZERO, LEFT, LEFT, RIGHT, LEFT, RIGHT,
       RIGHT, ZERO}}},
    {"fly",
     {{LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, ZERO, PIN, ZERO, ZERO, LEFT, PIN,
       RIGHT, ZERO, ZERO, PIN, ZERO, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT,
       ZERO}}},
    {"puppy",
     {{ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO, RIGHT,
       RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, ZERO,
       ZERO}}},
    {"stars",
     {{LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, ZERO, ZERO,
       ZERO, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT,
       ZERO}}},
    {"mountains",
     {{RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN,
       RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, LEFT, PIN, LEFT, PIN, LEFT, PIN}}},
    {"quad1",
     {{RIGHT, PIN, RIGHT, RIGHT, RIGHT, PIN, LEFT, LEFT, LEFT, PIN, LEFT, PIN,
       RIGHT, PIN, RIGHT, RIGHT, RIGHT, PIN, LEFT, LEFT, LEFT, PIN, LEFT,
       PIN}}},
    {"quad2",
     {{ZERO, PIN, RIGHT, RIGHT, RIGHT, PIN, LEFT, LEFT, LEFT, PIN, ZERO, PIN,
       ZERO, PIN, RIGHT, RIGHT, RIGHT, PIN, LEFT, LEFT, LEFT, PIN, ZERO, PIN}}},
    {"glasses",
     {{ZERO, PIN, ZERO, RIGHT, RIGHT, PIN, LEFT, LEFT, ZERO, PIN, ZERO, PIN,
       ZERO, PIN, ZERO, RIGHT, RIGHT, PIN, LEFT, LEFT, ZERO, PIN, ZERO, PIN}}},
    {"em",
     {{ZERO, PIN, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, PIN, ZERO, PIN,
       ZERO, PIN, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, PIN, ZERO, PIN}}},
    {"quad3",
     {{ZERO, RIGHT, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, LEFT, ZERO, PIN,
       ZERO, RIGHT, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, LEFT, ZERO,
       PIN}}},
    {"vee",
     {{ZERO, ZERO, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, ZERO, ZERO, PIN,
       ZERO, ZERO, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, ZERO, ZERO, PIN}}},
    {"square",
     {{ZERO, ZERO, ZERO, RIGHT, RIGHT, PIN, LEFT, LEFT, ZERO, ZERO, ZERO, PIN,
       ZERO, ZERO, ZERO, RIGHT, RIGHT, PIN, LEFT, LEFT, ZERO, ZERO, ZERO,
       PIN}}},
    {"eagle",
     {{RIGHT, ZERO, ZERO, RIGHT, RIGHT, PIN, LEFT, LEFT, ZERO, ZERO, LEFT, PIN,
       RIGHT, ZERO, ZERO, RIGHT, RIGHT, PIN, LEFT, LEFT, ZERO, ZERO, LEFT,
       PIN}}},
    {"volcano",
     {{RIGHT, ZERO, LEFT, RIGHT, RIGHT, PIN, LEFT, LEFT, RIGHT, ZERO, LEFT, PIN,
       RIGHT, ZERO, LEFT, RIGHT, RIGHT, PIN, LEFT, LEFT, RIGHT, ZERO, LEFT,
       PIN}}},
    {"saddle",
     {{RIGHT, ZERO, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, ZERO, LEFT, PIN,
       RIGHT, ZERO, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, ZERO, LEFT,
       PIN}}},
    {"c3d",
     {{ZERO, ZERO, RIGHT, ZERO, ZERO, PIN, ZERO, ZERO, LEFT, ZERO, ZERO, PIN,
       ZERO, ZERO, RIGHT, ZERO, ZERO, PIN, ZERO, ZERO, LEFT, ZERO, ZERO, PIN}}},
    {"block",
     {{ZERO, ZERO, PIN, PIN, ZERO, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT,
       PIN, LEFT, PIN, RIGHT, ZERO, ZERO, PIN, ZERO, ZERO, LEFT, PIN, RIGHT}}},
    {"duck",
     {{LEFT, PIN, LEFT, PIN, ZERO, PIN, PIN, ZERO, PIN, ZERO, LEFT, PIN, RIGHT,
       ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO, LEFT, PIN, LEFT, ZERO}}},
    {"prayer",
     {{RIGHT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, ZERO, ZERO, ZERO, RIGHT,
       PIN, LEFT, ZERO, ZERO, ZERO, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, LEFT,
       PIN}}},
    {"giraffe",
     {{ZERO, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, ZERO, RIGHT, RIGHT,
       RIGHT, PIN, LEFT, RIGHT, ZERO, PIN, ZERO, LEFT, RIGHT, PIN, LEFT, LEFT,
       LEFT}}},
    {"tie fighter",
     {{PIN, LEFT, RIGHT, LEFT, LEFT, PIN, RIGHT, ZERO, RIGHT, LEFT, ZERO, PIN,
       LEFT, LEFT, RIGHT, RIGHT, RIGHT, PIN, LEFT, ZERO, LEFT, RIGHT, ZERO,
       ZERO}}},
    {"Strong Arms",
     {{PIN, PIN, ZERO, ZERO, PIN, ZERO, ZERO, RIGHT, ZERO, RIGHT, RIGHT, PIN,
       RIGHT, RIGHT, ZERO, RIGHT, ZERO, ZERO, PIN, ZERO, ZERO, PIN, PIN,
       ZERO}}},

    /* the following modesl were created during the slug/compsoc codefest
     * febrray 2003 */
    {"cool looking gegl",
     {{PIN, PIN, ZERO, ZERO, RIGHT, ZERO, ZERO, PIN, PIN, ZERO, LEFT, ZERO,
       ZERO, PIN, ZERO, PIN, PIN, ZERO, LEFT, RIGHT, PIN, ZERO, ZERO, ZERO}}},
    {"knuckledusters",
     {{ZERO, ZERO, ZERO, ZERO, PIN, RIGHT, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO,
       RIGHT, RIGHT, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, RIGHT, ZERO}}},
#ifndef WIN32
    {"k's turd",
     {{RIGHT, RIGHT, PIN, RIGHT, LEFT, RIGHT, PIN, RIGHT, LEFT, RIGHT, PIN,
       RIGHT, LEFT, RIGHT, PIN, RIGHT, LEFT, RIGHT, PIN, RIGHT, LEFT, RIGHT,
       PIN, ZERO}}},
#endif
    {"lightsabre",
     {{ZERO, ZERO, ZERO, ZERO, ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO, ZERO,
       ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO,
       ZERO}}},
    {"not a stairway",
     {{LEFT, ZERO, RIGHT, LEFT, RIGHT, ZERO, LEFT, RIGHT, LEFT, ZERO, RIGHT,
       LEFT, RIGHT, ZERO, LEFT, RIGHT, LEFT, ZERO, RIGHT, LEFT, RIGHT, ZERO,
       LEFT, ZERO}}},
    {"not very good (but accurate) gegl",
     {{ZERO, PIN, PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO, LEFT, ZERO, PIN, PIN,
       ZERO, RIGHT, LEFT, ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN, ZERO}}},
    {"box",
     {{ZERO, ZERO, ZERO, ZERO, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, PIN,
       ZERO, ZERO, ZERO, ZERO, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO}}},
    {"kissy box",
     {{PIN, ZERO, ZERO, ZERO, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, PIN,
       ZERO, ZERO, ZERO, ZERO, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, PIN, ZERO}}},
#ifndef WIN32
    {"erect penis", /* thanks benno */
     {{PIN, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO,
       ZERO, ZERO, RIGHT, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO}}},
    {"flaccid penis",
     {{PIN, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO,
       ZERO, ZERO, RIGHT, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO}}},
    {"vagina",
     {{RIGHT, ZERO, ZERO, ZERO, RIGHT, ZERO, ZERO, PIN, ZERO, ZERO, LEFT, ZERO,
       ZERO, ZERO, LEFT, ZERO, LEFT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, ZERO}}},
#endif
    {"mask",
     {{ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, ZERO, PIN, ZERO, ZERO,
       PIN, ZERO, PIN, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO, ZERO, ZERO}}},
    {"poles or columns or something",
     {{LEFT, RIGHT, LEFT, ZERO, ZERO, ZERO, PIN, PIN, ZERO, ZERO, ZERO, LEFT,
       RIGHT, LEFT, ZERO, ZERO, ZERO, PIN, PIN, ZERO, ZERO, ZERO, LEFT, ZERO}}},
    {"crooked v",
     {{ZERO, LEFT, ZERO, ZERO, ZERO, ZERO, PIN, PIN, ZERO, ZERO, ZERO, LEFT,
       ZERO, LEFT, ZERO, ZERO, ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO, ZERO}}},
    {"dog leg",
     {{ZERO, LEFT, ZERO, ZERO, ZERO, ZERO, PIN, PIN, ZERO, ZERO, ZERO, LEFT,
       ZERO, RIGHT, ZERO, ZERO, ZERO, ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO}}},
    {"scrubby",
     {{ZERO, ZERO, ZERO, ZERO, ZERO, LEFT, ZERO, ZERO, ZERO, ZERO, LEFT, RIGHT,
       ZERO, ZERO, ZERO, ZERO, LEFT, RIGHT, ZERO, ZERO, LEFT, PIN, ZERO,
       ZERO}}},
    {"voltron's eyes",
     {{ZERO, ZERO, PIN, RIGHT, ZERO, LEFT, ZERO, ZERO, RIGHT, ZERO, LEFT, PIN,
       ZERO, ZERO, PIN, ZERO, LEFT, ZERO, RIGHT, LEFT, ZERO, RIGHT, ZERO,
       ZERO}}},
    {"flying toaster",
     {{PIN, ZERO, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, RIGHT,
       ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO}}},
    {"dubbya",
     {{PIN, ZERO, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, ZERO, ZERO,
       PIN, PIN, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO}}},
    {"tap handle",
     {{PIN, ZERO, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, LEFT, ZERO,
       PIN, PIN, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO}}},
    {"wingnut",
     {{PIN, ZERO, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, PIN, ZERO,
       PIN, PIN, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO}}},
    {"tight twist",
     {{RIGHT, ZERO, ZERO, LEFT, ZERO, LEFT, RIGHT, ZERO, RIGHT, LEFT, RIGHT,
       PIN, RIGHT, LEFT, RIGHT, ZERO, RIGHT, LEFT, ZERO, LEFT, ZERO, ZERO,
       RIGHT, ZERO}}},
    {"double helix",
     {{RIGHT, ZERO, RIGHT, ZERO, RIGHT, ZERO, RIGHT, ZERO, RIGHT, ZERO, RIGHT,
       ZERO, RIGHT, LEFT, RIGHT, PIN, ZERO, RIGHT, ZERO, RIGHT, ZERO, RIGHT,
       ZERO, ZERO}}},

    /* Models by stixpjr@gmail.com */
    {"begging dog",
      {{ZERO, RIGHT, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, RIGHT,
	LEFT, PIN, RIGHT, RIGHT, ZERO, LEFT, PIN, LEFT, RIGHT, PIN,
	RIGHT, LEFT, PIN, LEFT}}},
    {"swan",
      {{ZERO, PIN, ZERO, ZERO, ZERO, LEFT, ZERO, LEFT, ZERO, ZERO,
	RIGHT, PIN, LEFT, ZERO, ZERO, LEFT, PIN, RIGHT, ZERO, ZERO,
	LEFT, ZERO, LEFT}}},

/* These models come from the website at
 * http://www.geocities.com/stigeide/snake */
#if 0
    { "Abstract",
      { { RIGHT, LEFT, RIGHT, ZERO, PIN, ZERO, LEFT, RIGHT, LEFT, PIN, ZERO, ZERO, PIN, LEFT, RIGHT, LEFT, ZERO, PIN, ZERO, RIGHT, LEFT, RIGHT, ZERO, ZERO } }
    },
#endif
    {"toadstool",
     {{LEFT, RIGHT, ZERO, RIGHT, LEFT, ZERO, ZERO, RIGHT, LEFT, PIN, RIGHT,
       RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, PIN, RIGHT, LEFT,
       PIN, ZERO}}},
    {"AlanH2",
     {{LEFT, RIGHT, ZERO, RIGHT, LEFT, ZERO, ZERO, RIGHT, LEFT, PIN, RIGHT,
       RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT,
       RIGHT, ZERO}}},
    {"AlanH3",
     {{LEFT, RIGHT, ZERO, RIGHT, LEFT, ZERO, ZERO, RIGHT, LEFT, PIN, RIGHT,
       RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT, RIGHT, LEFT, PIN, LEFT, RIGHT,
       PIN, ZERO}}},
    {"AlanH4",
     {{ZERO, ZERO, PIN, LEFT, RIGHT, LEFT, ZERO, RIGHT, LEFT, RIGHT, ZERO, PIN,
       ZERO, LEFT, RIGHT, LEFT, ZERO, RIGHT, LEFT, RIGHT, PIN, ZERO, ZERO,
       ZERO}}},
    {"Alien",
     {{RIGHT, LEFT, RIGHT, PIN, ZERO, ZERO, PIN, RIGHT, LEFT, RIGHT, ZERO, PIN,
       PIN, ZERO, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT, ZERO, PIN, PIN, ZERO}}},
    {"Angel",
     {{ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT,
       RIGHT, RIGHT, LEFT, LEFT, LEFT, PIN, RIGHT, LEFT, ZERO, ZERO, RIGHT,
       LEFT, ZERO}}},
    {"AnotherFigure",
     {{LEFT, PIN, RIGHT, ZERO, ZERO, PIN, RIGHT, LEFT, LEFT, PIN, RIGHT, LEFT,
       ZERO, PIN, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, LEFT, PIN, ZERO,
       ZERO}}},
    {"Ball",
     {{LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT, LEFT,
       RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT,
       RIGHT, ZERO}}},
    {"Basket",
     {{ZERO, RIGHT, RIGHT, ZERO, RIGHT, RIGHT, ZERO, RIGHT, LEFT, ZERO, LEFT,
       LEFT, PIN, RIGHT, LEFT, ZERO, LEFT, RIGHT, PIN, LEFT, LEFT, ZERO, LEFT,
       ZERO}}},
    {"Beetle",
     {{PIN, LEFT, RIGHT, ZERO, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT,
       RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, ZERO, LEFT, RIGHT, PIN,
       RIGHT, ZERO}}},
    {"bone",
     {{PIN, PIN, LEFT, ZERO, PIN, PIN, ZERO, LEFT, ZERO, ZERO, ZERO, ZERO, ZERO,
       ZERO, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, RIGHT, PIN, PIN, ZERO}}},
    {"Bow",
     {{LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT,
       RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT,
       RIGHT, ZERO}}},
    {"bra",
     {{RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, LEFT, LEFT,
       RIGHT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, LEFT,
       LEFT, ZERO}}},
    {"bronchosaurus",
     {{ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO, PIN, ZERO, PIN, PIN,
       ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, PIN, ZERO}}},
    {"Cactus",
     {{PIN, LEFT, ZERO, PIN, PIN, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, PIN,
       RIGHT, PIN, LEFT, ZERO, ZERO, RIGHT, PIN, LEFT, PIN, ZERO, ZERO, ZERO}}},
    {"Camel",
     {{RIGHT, ZERO, PIN, RIGHT, PIN, RIGHT, ZERO, RIGHT, PIN, RIGHT, LEFT, PIN,
       LEFT, RIGHT, PIN, RIGHT, ZERO, RIGHT, PIN, RIGHT, ZERO, ZERO, LEFT,
       ZERO}}},
    {"Candlestick",
     {{LEFT, PIN, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, PIN, RIGHT, PIN,
       LEFT, PIN, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, PIN, RIGHT,
       ZERO}}},
    {"Cat",
     {{ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, PIN,
       PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO}}},
    {"Cave",
     {{RIGHT, ZERO, ZERO, PIN, LEFT, ZERO, PIN, PIN, ZERO, RIGHT, LEFT, PIN,
       RIGHT, RIGHT, LEFT, LEFT, PIN, RIGHT, RIGHT, LEFT, PIN, ZERO, ZERO,
       ZERO}}},
    {"Chains",
     {{PIN, ZERO, ZERO, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, ZERO,
       PIN, ZERO, ZERO, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, ZERO}}},
    {"Chair",
     {{RIGHT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, RIGHT, ZERO, ZERO, PIN, PIN,
       ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, ZERO, LEFT, RIGHT, LEFT, LEFT,
       ZERO}}},
    {"Chick",
     {{RIGHT, RIGHT, RIGHT, PIN, LEFT, PIN, LEFT, PIN, RIGHT, RIGHT, RIGHT, PIN,
       LEFT, LEFT, LEFT, PIN, RIGHT, PIN, RIGHT, PIN, LEFT, LEFT, LEFT, ZERO}}},
    {"Clockwise",
     {{RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT,
       RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT,
       RIGHT, RIGHT, RIGHT, ZERO}}},
    {"cobra",
     {{ZERO, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT,
       LEFT, LEFT, LEFT, ZERO, LEFT, RIGHT, ZERO, ZERO, PIN, ZERO, ZERO, RIGHT,
       ZERO}}},
#if 0
    { "Cobra2",
        { { LEFT, ZERO, PIN, ZERO, PIN, LEFT, ZERO, PIN, ZERO, LEFT, LEFT, PIN, RIGHT, RIGHT, ZERO, PIN, ZERO, RIGHT, PIN, ZERO, PIN, ZERO, RIGHT , ZERO } }
    },
#endif
    {"Cobra3",
     {{ZERO, LEFT, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN, ZERO,
       ZERO, LEFT, ZERO, ZERO, ZERO, PIN, ZERO, ZERO, ZERO, LEFT, ZERO}}},
    {"Compact1",
     {{ZERO, ZERO, PIN, ZERO, ZERO, LEFT, PIN, RIGHT, PIN, LEFT, PIN, LEFT, PIN,
       RIGHT, PIN, LEFT, ZERO, PIN, PIN, ZERO, ZERO, LEFT, PIN, ZERO}}},
    {"Compact2",
     {{LEFT, PIN, RIGHT, ZERO, ZERO, PIN, PIN, ZERO, RIGHT, PIN, LEFT, ZERO,
       ZERO, RIGHT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, ZERO, ZERO, ZERO,
       ZERO}}},
    {"Compact3",
     {{ZERO, PIN, ZERO, PIN, PIN, ZERO, LEFT, PIN, RIGHT, ZERO, PIN, PIN, ZERO,
       PIN, ZERO, PIN, PIN, ZERO, LEFT, PIN, RIGHT, ZERO, PIN, ZERO}}},
    {"Compact4",
     {{PIN, RIGHT, ZERO, ZERO, PIN, ZERO, ZERO, PIN, PIN, ZERO, PIN, RIGHT, PIN,
       LEFT, PIN, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO, ZERO, ZERO}}},
    {"Compact5",
     {{LEFT, ZERO, LEFT, PIN, RIGHT, PIN, LEFT, PIN, LEFT, PIN, RIGHT, PIN,
       RIGHT, PIN, LEFT, PIN, RIGHT, ZERO, RIGHT, PIN, RIGHT, PIN, LEFT,
       ZERO}}},
    {"Contact",
     {{PIN, ZERO, ZERO, PIN, LEFT, LEFT, PIN, LEFT, RIGHT, RIGHT, PIN, LEFT,
       LEFT, RIGHT, PIN, RIGHT, RIGHT, PIN, ZERO, ZERO, PIN, RIGHT, PIN,
       ZERO}}},
    {"Contact2",
     {{RIGHT, PIN, ZERO, LEFT, LEFT, PIN, RIGHT, RIGHT, ZERO, PIN, LEFT, PIN,
       RIGHT, PIN, ZERO, LEFT, LEFT, PIN, RIGHT, RIGHT, ZERO, PIN, LEFT,
       ZERO}}},
    {"Cook",
     {{ZERO, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, RIGHT, LEFT, PIN, LEFT, ZERO,
       PIN, PIN, ZERO, LEFT, PIN, LEFT, RIGHT, ZERO, RIGHT, ZERO, PIN, ZERO}}},
    {"Counterclockwise",
     {{LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT,
       LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT,
       ZERO}}},
    {"Cradle",
     {{LEFT, LEFT, ZERO, PIN, LEFT, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT,
       RIGHT, LEFT, RIGHT, PIN, ZERO, RIGHT, RIGHT, LEFT, LEFT, ZERO, ZERO,
       RIGHT, ZERO}}},
    {"Crankshaft",
     {{ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, PIN, LEFT, LEFT, PIN, RIGHT, LEFT,
       ZERO, PIN, LEFT, PIN, RIGHT, ZERO, ZERO, ZERO, PIN, RIGHT, ZERO}}},
    {"Cross",
     {{ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO,
       ZERO, ZERO, PIN, ZERO, PIN, PIN, ZERO, PIN, ZERO, PIN, ZERO}}},
    {"Cross2",
     {{ZERO, ZERO, PIN, PIN, ZERO, LEFT, ZERO, ZERO, PIN, PIN, ZERO, RIGHT,
       ZERO, ZERO, PIN, PIN, ZERO, LEFT, ZERO, ZERO, PIN, PIN, ZERO, ZERO}}},
    {"Cross3",
     {{ZERO, ZERO, PIN, PIN, ZERO, LEFT, ZERO, ZERO, PIN, PIN, ZERO, RIGHT,
       ZERO, ZERO, PIN, PIN, ZERO, LEFT, ZERO, ZERO, PIN, PIN, ZERO, ZERO}}},
    {"CrossVersion1",
     {{PIN, ZERO, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, PIN, LEFT, PIN,
       RIGHT, ZERO, PIN, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, RIGHT, PIN,
       ZERO}}},
    {"CrossVersion2",
     {{RIGHT, LEFT, PIN, LEFT, LEFT, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, PIN,
       LEFT, LEFT, PIN, RIGHT, LEFT, ZERO, LEFT, LEFT, PIN, LEFT, RIGHT,
       ZERO}}},
    {"Crown",
     {{LEFT, ZERO, PIN, ZERO, RIGHT, ZERO, ZERO, LEFT, ZERO, PIN, ZERO, RIGHT,
       LEFT, ZERO, PIN, ZERO, RIGHT, ZERO, ZERO, LEFT, ZERO, PIN, ZERO, ZERO}}},
    {"DNAStrand",
     {{RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN,
       RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT,
       ZERO}}},
    {"Diamond",
     {{ZERO, RIGHT, ZERO, ZERO, LEFT, ZERO, ZERO, RIGHT, PIN, LEFT, LEFT, RIGHT,
       LEFT, RIGHT, RIGHT, PIN, LEFT, ZERO, ZERO, RIGHT, ZERO, ZERO, LEFT,
       ZERO}}},
    {"Dog",
     {{RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, LEFT, RIGHT, RIGHT, LEFT, RIGHT,
       LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, ZERO, LEFT,
       RIGHT, ZERO}}},
    {"DogFace",
     {{ZERO, ZERO, PIN, PIN, ZERO, LEFT, LEFT, RIGHT, PIN, ZERO, PIN, PIN, ZERO,
       PIN, LEFT, RIGHT, RIGHT, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO}}},
    {"DoublePeak",
     {{ZERO, ZERO, PIN, ZERO, ZERO, RIGHT, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT,
       LEFT, LEFT, ZERO, PIN, ZERO, RIGHT, RIGHT, LEFT, PIN, LEFT, RIGHT,
       ZERO}}},
    {"DoubleRoof",
     {{ZERO, LEFT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT, LEFT,
       RIGHT, RIGHT, LEFT, LEFT, ZERO, LEFT, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT,
       ZERO}}},
    {"txoboggan",
     {{ZERO, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, ZERO, PIN, PIN, ZERO,
       ZERO, ZERO, ZERO, LEFT, PIN, RIGHT, ZERO, ZERO, ZERO, ZERO, PIN, ZERO}}},
    {"Doubled",
     {{LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, RIGHT, LEFT, ZERO,
       LEFT, PIN, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, ZERO}}},
    {"Doubled1",
     {{LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, ZERO, RIGHT, ZERO,
       RIGHT, ZERO, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT,
       ZERO}}},
    {"Doubled2",
     {{LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, LEFT, RIGHT, ZERO,
       RIGHT, LEFT, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT,
       ZERO}}},
    {"DumblingSpoon",
     {{PIN, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, LEFT, ZERO, ZERO, LEFT, RIGHT,
       ZERO, ZERO, LEFT, RIGHT, ZERO, ZERO, RIGHT, ZERO, ZERO, ZERO, ZERO,
       ZERO}}},
    {"Embrace",
     {{PIN, ZERO, ZERO, PIN, RIGHT, PIN, LEFT, PIN, ZERO, RIGHT, PIN, RIGHT,
       PIN, LEFT, PIN, LEFT, ZERO, PIN, RIGHT, PIN, LEFT, PIN, ZERO, ZERO}}},
    {"EndlessBelt",
     {{ZERO, RIGHT, LEFT, ZERO, ZERO, ZERO, LEFT, RIGHT, ZERO, PIN, RIGHT, LEFT,
       ZERO, LEFT, RIGHT, LEFT, PIN, LEFT, RIGHT, LEFT, ZERO, LEFT, RIGHT,
       ZERO}}},
    {"Entrance",
     {{LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT,
       RIGHT, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, LEFT, LEFT,
       RIGHT, ZERO}}},
    {"Esthetic",
     {{LEFT, LEFT, PIN, RIGHT, RIGHT, ZERO, LEFT, PIN, RIGHT, PIN, LEFT, PIN,
       LEFT, PIN, RIGHT, PIN, LEFT, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT,
       ZERO}}},
    {"Explosion",
     {{RIGHT, RIGHT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT, RIGHT, RIGHT,
       LEFT, RIGHT, LEFT, LEFT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT,
       LEFT, ZERO}}},
    {"F-ZeroXCar",
     {{RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, PIN, RIGHT, LEFT, ZERO, ZERO,
       RIGHT, LEFT, ZERO, ZERO, LEFT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, RIGHT,
       ZERO}}},
    {"Face",
     {{ZERO, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN,
       RIGHT, LEFT, PIN, LEFT, PIN, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, ZERO}}},
    {"FaserGun",
     {{ZERO, ZERO, LEFT, RIGHT, PIN, RIGHT, ZERO, RIGHT, PIN, RIGHT, LEFT, PIN,
       LEFT, RIGHT, PIN, RIGHT, ZERO, RIGHT, PIN, RIGHT, RIGHT, ZERO, PIN,
       ZERO}}},
    {"FelixW",
     {{ZERO, RIGHT, ZERO, PIN, LEFT, ZERO, LEFT, RIGHT, ZERO, ZERO, RIGHT, PIN,
       LEFT, ZERO, ZERO, LEFT, RIGHT, ZERO, RIGHT, PIN, ZERO, LEFT, ZERO,
       ZERO}}},
    {"Flamingo",
     {{ZERO, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, PIN, LEFT, LEFT, PIN, LEFT,
       RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, LEFT, ZERO, ZERO, ZERO, PIN, ZERO}}},
    {"FlatOnTheTop",
     {{ZERO, PIN, PIN, ZERO, PIN, RIGHT, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT,
       PIN, LEFT, RIGHT, ZERO, RIGHT, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO}}},
    {"Fly",
     {{ZERO, LEFT, PIN, RIGHT, ZERO, PIN, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT,
       PIN, RIGHT, PIN, LEFT, PIN, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, ZERO}}},
    {"Fountain",
     {{LEFT, RIGHT, LEFT, RIGHT, RIGHT, PIN, LEFT, PIN, LEFT, RIGHT, RIGHT, PIN,
       LEFT, LEFT, RIGHT, RIGHT, PIN, LEFT, LEFT, RIGHT, PIN, RIGHT, PIN,
       ZERO}}},
    {"Frog",
     {{LEFT, LEFT, RIGHT, RIGHT, LEFT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, ZERO,
       RIGHT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, RIGHT, LEFT, RIGHT, RIGHT,
       ZERO}}},
    {"Frog2",
     {{LEFT, ZERO, LEFT, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, ZERO, RIGHT, PIN,
       LEFT, ZERO, ZERO, LEFT, RIGHT, PIN, LEFT, LEFT, RIGHT, ZERO, RIGHT,
       ZERO}}},
    {"Furby",
     {{PIN, ZERO, LEFT, PIN, RIGHT, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, ZERO,
       PIN, ZERO, RIGHT, PIN, LEFT, ZERO, PIN, ZERO, ZERO, PIN, ZERO}}},
    {"Gate",
     {{ZERO, ZERO, PIN, ZERO, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, LEFT, PIN,
       LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, RIGHT, ZERO, PIN, PIN, ZERO, ZERO}}},
    {"Ghost",
     {{LEFT, LEFT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT, RIGHT,
       RIGHT, PIN, LEFT, RIGHT, ZERO, ZERO, LEFT, RIGHT, ZERO, ZERO, LEFT,
       RIGHT, ZERO}}},
    {"Globus",
     {{RIGHT, LEFT, ZERO, PIN, LEFT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT,
       LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, PIN, ZERO, RIGHT, LEFT,
       ZERO, ZERO}}},
    {"Grotto",
     {{PIN, PIN, ZERO, LEFT, RIGHT, LEFT, ZERO, PIN, RIGHT, PIN, LEFT, ZERO,
       ZERO, ZERO, ZERO, RIGHT, PIN, LEFT, PIN, ZERO, RIGHT, LEFT, RIGHT,
       ZERO}}},
    {"H",
     {{PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO, PIN, PIN, ZERO, PIN, LEFT,
       ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO, PIN, PIN, ZERO, ZERO}}},
    {"HeadOfDevil",
     {{PIN, ZERO, RIGHT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, PIN, RIGHT, LEFT,
       PIN, LEFT, LEFT, PIN, RIGHT, RIGHT, PIN, RIGHT, LEFT, ZERO, ZERO,
       ZERO}}},
    {"Heart",
     {{RIGHT, ZERO, ZERO, ZERO, PIN, LEFT, PIN, LEFT, RIGHT, RIGHT, ZERO, PIN,
       ZERO, LEFT, LEFT, RIGHT, PIN, RIGHT, PIN, ZERO, ZERO, ZERO, LEFT,
       ZERO}}},
    {"Heart2",
     {{ZERO, PIN, ZERO, ZERO, LEFT, ZERO, LEFT, ZERO, ZERO, ZERO, ZERO, PIN,
       ZERO, ZERO, ZERO, ZERO, RIGHT, ZERO, RIGHT, ZERO, ZERO, PIN, ZERO,
       ZERO}}},
    {"Hexagon",
     {{ZERO, ZERO, ZERO, ZERO, LEFT, ZERO, ZERO, RIGHT, ZERO, ZERO, ZERO, ZERO,
       LEFT, ZERO, ZERO, RIGHT, ZERO, ZERO, ZERO, ZERO, LEFT, ZERO, ZERO,
       ZERO}}},
    {"HoleInTheMiddle1",
     {{ZERO, LEFT, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, LEFT, ZERO, LEFT, RIGHT,
       ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, RIGHT,
       ZERO}}},
    {"HoleInTheMiddle2",
     {{ZERO, LEFT, RIGHT, ZERO, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, RIGHT,
       LEFT, ZERO, LEFT, RIGHT, ZERO, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO,
       RIGHT, ZERO}}},
    {"HouseBoat",
     {{RIGHT, RIGHT, PIN, LEFT, LEFT, LEFT, PIN, RIGHT, RIGHT, RIGHT, PIN, LEFT,
       RIGHT, ZERO, LEFT, PIN, RIGHT, PIN, LEFT, PIN, LEFT, RIGHT, PIN, ZERO}}},
    {"HouseByHouse",
     {{LEFT, PIN, LEFT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN,
       LEFT, PIN, LEFT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, PIN, RIGHT, ZERO}}},
    {"Infinity",
     {{LEFT, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT,
       LEFT, LEFT, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, RIGHT, LEFT,
       LEFT, ZERO}}},
    {"Integral",
     {{RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT,
       LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT,
       LEFT, ZERO}}},
    {"Iron",
     {{ZERO, ZERO, ZERO, ZERO, PIN, RIGHT, ZERO, RIGHT, ZERO, ZERO, LEFT, PIN,
       RIGHT, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, RIGHT, ZERO, RIGHT,
       ZERO}}},
    {"just squares",
     {{RIGHT, RIGHT, LEFT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, LEFT, LEFT, PIN,
       RIGHT, RIGHT, LEFT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, LEFT, LEFT,
       ZERO}}},
    {"Kink",
     {{ZERO, PIN, PIN, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO, RIGHT, PIN, LEFT,
       ZERO, ZERO, PIN, PIN, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO}}},
    {"Knot",
     {{LEFT, LEFT, PIN, LEFT, ZERO, LEFT, RIGHT, LEFT, PIN, LEFT, LEFT, RIGHT,
       RIGHT, PIN, RIGHT, LEFT, RIGHT, ZERO, RIGHT, PIN, RIGHT, RIGHT, LEFT,
       ZERO}}},
    {"Leaf",
     {{ZERO, PIN, PIN, ZERO, ZERO, LEFT, ZERO, LEFT, ZERO, ZERO, PIN, ZERO,
       ZERO, RIGHT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO,
       ZERO}}},
    {"LeftAsRight",
     {{RIGHT, PIN, LEFT, RIGHT, LEFT, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, PIN,
       LEFT, LEFT, PIN, RIGHT, LEFT, ZERO, RIGHT, LEFT, RIGHT, PIN, LEFT,
       ZERO}}},
    {"Long-necked",
     {{PIN, ZERO, LEFT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, ZERO, PIN, ZERO,
       LEFT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, ZERO, PIN, PIN, ZERO, ZERO}}},
    {"lunar module",
     {{PIN, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT,
       RIGHT, LEFT, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, RIGHT, LEFT, ZERO,
       LEFT, ZERO}}},
    {"magnifying glass",
     {{ZERO, ZERO, PIN, ZERO, LEFT, ZERO, PIN, PIN, ZERO, ZERO, RIGHT, PIN,
       LEFT, ZERO, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN, ZERO, ZERO, ZERO}}},
    {"Mask",
     {{ZERO, ZERO, ZERO, RIGHT, ZERO, RIGHT, LEFT, ZERO, LEFT, PIN, ZERO, PIN,
       ZERO, ZERO, PIN, ZERO, PIN, RIGHT, ZERO, RIGHT, LEFT, ZERO, LEFT,
       ZERO}}},
    {"Microscope",
     {{PIN, PIN, ZERO, ZERO, PIN, ZERO, RIGHT, PIN, ZERO, ZERO, RIGHT, PIN,
       LEFT, ZERO, ZERO, PIN, LEFT, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO}}},
    {"Mirror",
     {{PIN, RIGHT, LEFT, ZERO, PIN, PIN, ZERO, ZERO, LEFT, RIGHT, ZERO, ZERO,
       PIN, ZERO, ZERO, LEFT, RIGHT, PIN, RIGHT, ZERO, PIN, PIN, ZERO, ZERO}}},
    {"MissPiggy",
     {{ZERO, LEFT, LEFT, PIN, RIGHT, ZERO, RIGHT, RIGHT, PIN, LEFT, LEFT, RIGHT,
       RIGHT, PIN, LEFT, LEFT, ZERO, LEFT, PIN, RIGHT, RIGHT, ZERO, RIGHT,
       ZERO}}},
    {"Mole",
     {{ZERO, RIGHT, ZERO, RIGHT, LEFT, RIGHT, PIN, ZERO, LEFT, PIN, RIGHT, ZERO,
       PIN, LEFT, RIGHT, LEFT, ZERO, LEFT, ZERO, RIGHT, RIGHT, PIN, LEFT,
       ZERO}}},
    {"Monk",
     {{LEFT, ZERO, PIN, PIN, ZERO, LEFT, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN,
       PIN, ZERO, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, ZERO}}},
    {"Mountain",
     {{ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, LEFT, PIN,
       LEFT, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO,
       ZERO}}},
    {"mountains",
     {{ZERO, PIN, ZERO, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN,
       LEFT, PIN, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, ZERO, PIN, ZERO, ZERO}}},
    {"MouseWithoutTail",
     {{ZERO, PIN, PIN, ZERO, LEFT, ZERO, PIN, PIN, ZERO, ZERO, RIGHT, PIN, LEFT,
       ZERO, ZERO, PIN, PIN, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, ZERO}}},
    {"mushroom",
     {{PIN, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, ZERO,
       LEFT, PIN, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, LEFT, ZERO, PIN, ZERO}}},
    {"necklace",
     {{ZERO, ZERO, LEFT, ZERO, ZERO, ZERO, LEFT, ZERO, ZERO, ZERO, ZERO, PIN,
       ZERO, ZERO, ZERO, ZERO, RIGHT, ZERO, ZERO, ZERO, RIGHT, ZERO, ZERO,
       ZERO}}},
    {"NestledAgainst",
     {{LEFT, ZERO, PIN, LEFT, LEFT, RIGHT, RIGHT, PIN, ZERO, RIGHT, PIN, LEFT,
       ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, LEFT,
       ZERO}}},
    {"NoClue",
     {{ZERO, RIGHT, PIN, LEFT, LEFT, LEFT, ZERO, LEFT, PIN, RIGHT, RIGHT, PIN,
       LEFT, LEFT, PIN, RIGHT, ZERO, RIGHT, RIGHT, RIGHT, PIN, LEFT, ZERO,
       ZERO}}},
    {"Noname",
     {{LEFT, PIN, RIGHT, PIN, RIGHT, ZERO, PIN, ZERO, ZERO, PIN, PIN, ZERO, PIN,
       PIN, ZERO, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, LEFT, ZERO}}},
    {"Obelisk",
     {{PIN, ZERO, ZERO, ZERO, PIN, RIGHT, PIN, LEFT, PIN, LEFT, PIN, LEFT,
       RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, LEFT, PIN, ZERO, ZERO, ZERO, ZERO}}},
    {"Ostrich",
     {{ZERO, ZERO, PIN, PIN, ZERO, LEFT, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO,
       RIGHT, ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, PIN, ZERO}}},
    {"Ostrich2",
     {{PIN, PIN, ZERO, PIN, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT,
       RIGHT, LEFT, LEFT, RIGHT, PIN, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO}}},
    {"pair of glasses",
     {{ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO, LEFT, ZERO, PIN, ZERO,
       RIGHT, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO}}},
    {"Parrot",
     {{ZERO, ZERO, ZERO, ZERO, RIGHT, RIGHT, ZERO, LEFT, PIN, RIGHT, ZERO,
       RIGHT, ZERO, RIGHT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, LEFT, ZERO, PIN,
       ZERO}}},
#ifndef WIN32
    {"Penis",
     {{PIN, PIN, RIGHT, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO, RIGHT, PIN, LEFT,
       ZERO, ZERO, PIN, ZERO, PIN, PIN, ZERO, LEFT, PIN, PIN, ZERO}}},
#endif
    {"PictureCommingSoon",
     {{LEFT, LEFT, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, PIN, RIGHT, LEFT, PIN,
       LEFT, RIGHT, PIN, RIGHT, RIGHT, PIN, RIGHT, LEFT, ZERO, RIGHT, RIGHT,
       ZERO}}},
    {"Pitti",
     {{LEFT, PIN, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO, RIGHT, PIN,
       LEFT, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO, PIN, RIGHT, ZERO}}},
    {"Plait",
     {{LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, RIGHT, LEFT,
       RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT,
       LEFT, ZERO}}},
    {"Platform",
     {{RIGHT, PIN, ZERO, ZERO, ZERO, ZERO, PIN, ZERO, ZERO, PIN, PIN, ZERO, PIN,
       LEFT, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO}}},
    {"PodRacer",
     {{ZERO, PIN, ZERO, PIN, RIGHT, PIN, ZERO, RIGHT, PIN, LEFT, LEFT, PIN,
       RIGHT, LEFT, ZERO, PIN, PIN, ZERO, ZERO, LEFT, ZERO, PIN, LEFT, ZERO}}},
#if 0
    { "Pokemon",
        { { LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, ZERO } }
    },
#endif
    {"Prawn",
     {{RIGHT, PIN, ZERO, PIN, RIGHT, ZERO, PIN, PIN, ZERO, ZERO, LEFT, PIN,
       RIGHT, ZERO, ZERO, PIN, PIN, ZERO, LEFT, PIN, ZERO, PIN, LEFT, ZERO}}},
    {"Propeller",
     {{ZERO, ZERO, ZERO, RIGHT, ZERO, LEFT, RIGHT, LEFT, ZERO, ZERO, ZERO,
       RIGHT, ZERO, LEFT, RIGHT, LEFT, ZERO, ZERO, ZERO, RIGHT, ZERO, LEFT,
       RIGHT, ZERO}}},
    {"Pyramid",
     {{ZERO, LEFT, PIN, RIGHT, ZERO, LEFT, PIN, RIGHT, ZERO, LEFT, PIN, RIGHT,
       ZERO, PIN, RIGHT, LEFT, LEFT, LEFT, PIN, RIGHT, RIGHT, RIGHT, LEFT,
       ZERO}}},
    {"QuarterbackTiltedAndReadyToHut",
     {{PIN, ZERO, RIGHT, RIGHT, LEFT, RIGHT, PIN, RIGHT, LEFT, RIGHT, ZERO, PIN,
       ZERO, LEFT, RIGHT, LEFT, PIN, LEFT, RIGHT, LEFT, LEFT, ZERO, PIN,
       ZERO}}},
    {"Ra",
     {{PIN, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT,
       LEFT, ZERO, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT,
       LEFT, ZERO}}},
    {"Rattlesnake",
     {{LEFT, ZERO, LEFT, ZERO, LEFT, ZERO, LEFT, LEFT, ZERO, LEFT, ZERO, LEFT,
       ZERO, LEFT, RIGHT, ZERO, PIN, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT,
       ZERO}}},
    {"Revelation",
     {{ZERO, ZERO, ZERO, PIN, ZERO, ZERO, PIN, RIGHT, LEFT, LEFT, LEFT, RIGHT,
       RIGHT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, LEFT, PIN, ZERO, ZERO, PIN,
       ZERO}}},
    {"Revolution1",
     {{LEFT, LEFT, PIN, RIGHT, ZERO, PIN, ZERO, LEFT, PIN, RIGHT, RIGHT, PIN,
       LEFT, LEFT, PIN, RIGHT, ZERO, PIN, ZERO, LEFT, PIN, RIGHT, RIGHT,
       ZERO}}},
    {"Ribbon",
     {{RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, PIN, ZERO, PIN, PIN, ZERO, PIN,
       ZERO, PIN, PIN, ZERO, PIN, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT,
       ZERO}}},
    {"Rocket",
     {{RIGHT, ZERO, LEFT, PIN, RIGHT, ZERO, RIGHT, ZERO, LEFT, ZERO, RIGHT, PIN,
       LEFT, ZERO, RIGHT, ZERO, LEFT, ZERO, LEFT, PIN, RIGHT, ZERO, LEFT,
       ZERO}}},
    {"Roofed",
     {{ZERO, LEFT, PIN, RIGHT, ZERO, PIN, LEFT, ZERO, PIN, ZERO, RIGHT, PIN,
       ZERO, LEFT, PIN, RIGHT, ZERO, PIN, LEFT, ZERO, PIN, ZERO, RIGHT, ZERO}}},
    {"Roofs",
     {{PIN, PIN, RIGHT, ZERO, LEFT, PIN, RIGHT, PIN, LEFT, PIN, LEFT, PIN,
       RIGHT, PIN, RIGHT, PIN, LEFT, PIN, RIGHT, ZERO, LEFT, PIN, PIN, ZERO}}},
    {"RowHouses",
     {{RIGHT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, PIN, LEFT, PIN, LEFT, PIN,
       RIGHT, PIN, RIGHT, PIN, LEFT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, ZERO}}},
    {"Sculpture",
     {{RIGHT, LEFT, PIN, ZERO, ZERO, ZERO, LEFT, RIGHT, LEFT, PIN, ZERO, ZERO,
       PIN, LEFT, RIGHT, LEFT, ZERO, ZERO, ZERO, PIN, LEFT, RIGHT, LEFT,
       ZERO}}},
    {"Seal",
     {{LEFT, LEFT, LEFT, PIN, RIGHT, RIGHT, RIGHT, ZERO, LEFT, PIN, RIGHT, ZERO,
       LEFT, LEFT, LEFT, PIN, RIGHT, LEFT, ZERO, PIN, PIN, ZERO, LEFT, ZERO}}},
    {"Seal2",
     {{RIGHT, PIN, ZERO, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, RIGHT,
       PIN, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, ZERO, LEFT, RIGHT, ZERO, ZERO,
       ZERO}}},
    {"Sheep",
     {{RIGHT, LEFT, LEFT, RIGHT, RIGHT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT,
       RIGHT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, LEFT, LEFT, LEFT, RIGHT,
       LEFT, ZERO}}},
    {"Shelter",
     {{LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT, ZERO,
       ZERO, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO, ZERO, ZERO, RIGHT,
       ZERO}}},
    {"Ship",
     {{PIN, RIGHT, LEFT, LEFT, LEFT, LEFT, PIN, RIGHT, RIGHT, RIGHT, RIGHT,
       LEFT, ZERO, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, LEFT, ZERO, PIN, PIN,
       ZERO}}},
    {"Shpongle",
     {{LEFT, RIGHT, ZERO, RIGHT, LEFT, RIGHT, ZERO, RIGHT, LEFT, RIGHT, ZERO,
       RIGHT, LEFT, RIGHT, ZERO, RIGHT, LEFT, RIGHT, ZERO, RIGHT, LEFT, RIGHT,
       ZERO, ZERO}}},
    {"Slide",
     {{LEFT, RIGHT, LEFT, RIGHT, ZERO, LEFT, RIGHT, LEFT, PIN, ZERO, ZERO, PIN,
       ZERO, ZERO, PIN, RIGHT, LEFT, ZERO, ZERO, RIGHT, LEFT, RIGHT, LEFT,
       ZERO}}},
    {"SmallShip",
     {{ZERO, LEFT, RIGHT, ZERO, RIGHT, LEFT, ZERO, LEFT, RIGHT, ZERO, LEFT,
       RIGHT, ZERO, LEFT, RIGHT, ZERO, RIGHT, LEFT, ZERO, LEFT, RIGHT, ZERO,
       LEFT, ZERO}}},
    {"SnakeReadyToStrike",
     {{LEFT, ZERO, LEFT, ZERO, LEFT, ZERO, LEFT, RIGHT, ZERO, RIGHT, ZERO,
       RIGHT, ZERO, LEFT, ZERO, ZERO, ZERO, PIN, ZERO, ZERO, ZERO, ZERO, LEFT,
       ZERO}}},
    {"Snakes14",
     {{RIGHT, RIGHT, PIN, ZERO, RIGHT, LEFT, RIGHT, ZERO, ZERO, ZERO, RIGHT,
       PIN, LEFT, PIN, ZERO, PIN, LEFT, PIN, RIGHT, ZERO, ZERO, LEFT, RIGHT,
       ZERO}}},
    {"Snakes15",
     {{ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, PIN, LEFT, LEFT, PIN, RIGHT, LEFT,
       ZERO, PIN, LEFT, PIN, RIGHT, ZERO, ZERO, ZERO, PIN, RIGHT, ZERO}}},
    {"Snakes18",
     {{PIN, PIN, LEFT, PIN, LEFT, PIN, RIGHT, ZERO, RIGHT, PIN, RIGHT, ZERO,
       RIGHT, PIN, LEFT, PIN, RIGHT, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO}}},
    {"Snowflake",
     {{LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT,
       RIGHT, RIGHT, RIGHT, RIGHT, LEFT, LEFT, LEFT, LEFT, RIGHT, RIGHT, RIGHT,
       RIGHT, ZERO}}},
    {"Snowman",
     {{ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO, PIN,
       PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, ZERO}}},
    {"Source",
     {{PIN, RIGHT, ZERO, PIN, ZERO, LEFT, PIN, RIGHT, PIN, LEFT, LEFT, RIGHT,
       LEFT, RIGHT, RIGHT, PIN, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, PIN,
       ZERO}}},
    {"Spaceship",
     {{PIN, PIN, RIGHT, RIGHT, PIN, RIGHT, LEFT, PIN, LEFT, PIN, RIGHT, PIN,
       RIGHT, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, PIN, RIGHT, PIN, PIN, ZERO}}},
    {"Spaceship2",
     {{PIN, PIN, LEFT, PIN, LEFT, PIN, RIGHT, ZERO, PIN, PIN, ZERO, LEFT, PIN,
       RIGHT, ZERO, PIN, ZERO, LEFT, PIN, LEFT, LEFT, PIN, PIN, ZERO}}},
    {"Speedboat",
     {{LEFT, ZERO, ZERO, LEFT, PIN, RIGHT, ZERO, ZERO, LEFT, ZERO, ZERO, PIN,
       ZERO, ZERO, RIGHT, ZERO, ZERO, LEFT, PIN, RIGHT, ZERO, ZERO, RIGHT,
       ZERO}}},
    {"Speedboat2",
     {{PIN, RIGHT, LEFT, LEFT, RIGHT, RIGHT, RIGHT, ZERO, LEFT, PIN, RIGHT,
       ZERO, LEFT, LEFT, LEFT, RIGHT, RIGHT, LEFT, PIN, ZERO, RIGHT, PIN, LEFT,
       ZERO}}},
    {"Spider",
     {{RIGHT, RIGHT, ZERO, ZERO, LEFT, RIGHT, LEFT, PIN, ZERO, LEFT, ZERO, PIN,
       PIN, ZERO, RIGHT, ZERO, PIN, RIGHT, LEFT, RIGHT, ZERO, ZERO, LEFT,
       ZERO}}},
    {"Spitzbergen",
     {{PIN, LEFT, ZERO, RIGHT, RIGHT, LEFT, PIN, ZERO, LEFT, PIN, RIGHT, RIGHT,
       PIN, LEFT, LEFT, PIN, RIGHT, ZERO, PIN, RIGHT, LEFT, LEFT, ZERO, ZERO}}},
    {"Square",
     {{ZERO, ZERO, LEFT, LEFT, PIN, RIGHT, RIGHT, ZERO, ZERO, LEFT, LEFT, PIN,
       RIGHT, RIGHT, ZERO, ZERO, LEFT, LEFT, PIN, RIGHT, RIGHT, ZERO, ZERO,
       ZERO}}},
    {"SquareHole",
     {{PIN, ZERO, PIN, ZERO, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO, PIN, ZERO,
       ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO, PIN, ZERO}}},
    {"Stage",
     {{RIGHT, ZERO, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, LEFT, RIGHT, PIN, RIGHT,
       LEFT, PIN, LEFT, RIGHT, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, ZERO,
       ZERO}}},
    {"Stairs",
     {{ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO,
       PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, ZERO}}},
    {"Stairs2",
     {{ZERO, PIN, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO, PIN, ZERO,
       PIN, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO, ZERO}}},
    {"Straight",
     {{ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO,
       ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO,
       ZERO}}},
    {"Swan",
     {{ZERO, PIN, ZERO, PIN, LEFT, LEFT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT,
       LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, PIN, LEFT, PIN, LEFT, RIGHT, ZERO}}},
    {"Swan2",
     {{PIN, ZERO, PIN, RIGHT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, RIGHT,
       LEFT, RIGHT, RIGHT, RIGHT, PIN, ZERO, ZERO, ZERO, ZERO, ZERO, PIN, PIN,
       ZERO}}},
    {"Swan3",
     {{PIN, PIN, ZERO, ZERO, ZERO, RIGHT, ZERO, RIGHT, ZERO, ZERO, LEFT, PIN,
       RIGHT, ZERO, ZERO, RIGHT, PIN, LEFT, ZERO, ZERO, RIGHT, ZERO, RIGHT,
       ZERO}}},
    {"Symbol",
     {{RIGHT, RIGHT, PIN, ZERO, PIN, PIN, ZERO, PIN, LEFT, LEFT, RIGHT, LEFT,
       RIGHT, RIGHT, PIN, ZERO, PIN, PIN, ZERO, PIN, LEFT, LEFT, RIGHT, ZERO}}},
    {"Symmetry",
     {{RIGHT, ZERO, LEFT, RIGHT, LEFT, ZERO, LEFT, RIGHT, LEFT, ZERO, RIGHT,
       PIN, LEFT, ZERO, RIGHT, LEFT, RIGHT, ZERO, RIGHT, LEFT, RIGHT, ZERO,
       LEFT, ZERO}}},
    {"Symmetry2",
     {{ZERO, PIN, LEFT, LEFT, PIN, ZERO, ZERO, LEFT, PIN, RIGHT, PIN, LEFT,
       LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, PIN, LEFT, ZERO}}},
    {"TableFireworks",
     {{ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO,
       RIGHT, LEFT, PIN, RIGHT, RIGHT, RIGHT, PIN, RIGHT, LEFT, ZERO, RIGHT,
       PIN, ZERO}}},
    {"Tapering",
     {{ZERO, ZERO, RIGHT, LEFT, PIN, LEFT, ZERO, PIN, PIN, ZERO, LEFT, PIN,
       RIGHT, ZERO, PIN, PIN, ZERO, RIGHT, PIN, RIGHT, LEFT, ZERO, ZERO,
       ZERO}}},
    {"TaperingTurned",
     {{ZERO, ZERO, RIGHT, LEFT, PIN, LEFT, ZERO, PIN, PIN, ZERO, LEFT, ZERO,
       RIGHT, ZERO, PIN, PIN, ZERO, RIGHT, PIN, RIGHT, LEFT, ZERO, ZERO,
       ZERO}}},
    {"TeaLightStick",
     {{RIGHT, ZERO, PIN, PIN, ZERO, LEFT, RIGHT, PIN, LEFT, LEFT, RIGHT, RIGHT,
       PIN, LEFT, LEFT, RIGHT, RIGHT, PIN, LEFT, LEFT, RIGHT, RIGHT, PIN,
       ZERO}}},
    {"thighmaster",
     {{RIGHT, ZERO, ZERO, RIGHT, LEFT, ZERO, ZERO, RIGHT, LEFT, ZERO, ZERO,
       LEFT, RIGHT, ZERO, ZERO, RIGHT, LEFT, ZERO, ZERO, RIGHT, LEFT, ZERO,
       ZERO, ZERO}}},
    {"Terraces",
     {{RIGHT, LEFT, ZERO, RIGHT, LEFT, PIN, LEFT, LEFT, PIN, LEFT, RIGHT, RIGHT,
       RIGHT, LEFT, LEFT, LEFT, RIGHT, PIN, RIGHT, RIGHT, PIN, RIGHT, LEFT,
       ZERO}}},
    {"Terrier",
     {{PIN, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO, PIN,
       ZERO, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO, ZERO}}},
    {"Three-Legged",
     {{RIGHT, ZERO, LEFT, RIGHT, ZERO, LEFT, PIN, RIGHT, ZERO, RIGHT, ZERO, PIN,
       ZERO, LEFT, ZERO, LEFT, PIN, RIGHT, ZERO, LEFT, RIGHT, ZERO, LEFT,
       ZERO}}},
    {"ThreePeaks",
     {{RIGHT, ZERO, ZERO, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, RIGHT, PIN,
       LEFT, LEFT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, ZERO, ZERO, LEFT, ZERO}}},
    {"ToTheFront",
     {{ZERO, PIN, RIGHT, LEFT, LEFT, LEFT, PIN, RIGHT, LEFT, ZERO, PIN, PIN,
       ZERO, LEFT, LEFT, PIN, ZERO, LEFT, RIGHT, ZERO, PIN, ZERO, LEFT, ZERO}}},
    {"Top",
     {{PIN, LEFT, LEFT, PIN, LEFT, ZERO, ZERO, RIGHT, LEFT, PIN, RIGHT, RIGHT,
       LEFT, RIGHT, LEFT, LEFT, PIN, RIGHT, PIN, RIGHT, RIGHT, PIN, ZERO,
       ZERO}}},
    {"Transport",
     {{PIN, ZERO, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, PIN,
       PIN, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO, ZERO, ZERO, ZERO}}},
    {"Triangle",
     {{ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, RIGHT, LEFT, ZERO, ZERO, ZERO, ZERO,
       ZERO, ZERO, RIGHT, LEFT, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, RIGHT,
       ZERO}}},
    {"Tripple",
     {{PIN, ZERO, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, PIN, ZERO, PIN, LEFT, PIN,
       RIGHT, PIN, ZERO, PIN, LEFT, PIN, LEFT, PIN, RIGHT, PIN, ZERO}}},
#if 0
    { "Turtle",
        { { RIGHT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, LEFT, PIN, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, ZERO, LEFT, RIGHT, ZERO, ZERO } }
    },
#endif
    {"Twins",
     {{ZERO, PIN, ZERO, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, PIN, ZERO, ZERO,
       PIN, LEFT, PIN, LEFT, RIGHT, PIN, RIGHT, ZERO, PIN, ZERO, ZERO, ZERO}}},
    {"TwoSlants",
     {{ZERO, PIN, ZERO, ZERO, PIN, PIN, ZERO, PIN, ZERO, RIGHT, PIN, RIGHT,
       LEFT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, ZERO, ZERO, RIGHT, PIN, ZERO}}},
    {"TwoWings",
     {{PIN, LEFT, ZERO, RIGHT, ZERO, PIN, PIN, ZERO, PIN, PIN, ZERO, PIN, PIN,
       ZERO, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, LEFT, ZERO, ZERO}}},
    {"UFO",
     {{LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, LEFT,
       PIN, LEFT, LEFT, LEFT, RIGHT, LEFT, RIGHT, RIGHT, LEFT, RIGHT, LEFT,
       LEFT, ZERO}}},
    {"USS Enterprise",
     {{LEFT, PIN, RIGHT, PIN, RIGHT, LEFT, ZERO, PIN, PIN, ZERO, RIGHT, LEFT,
       ZERO, PIN, PIN, ZERO, RIGHT, LEFT, PIN, LEFT, PIN, RIGHT, ZERO, ZERO}}},
    {"UpAndDown",
     {{ZERO, PIN, ZERO, PIN, ZERO, PIN, LEFT, PIN, RIGHT, PIN, ZERO, PIN, ZERO,
       PIN, ZERO, PIN, ZERO, PIN, LEFT, PIN, RIGHT, PIN, ZERO, ZERO}}},
    {"Upright",
     {{ZERO, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, PIN, ZERO, ZERO, LEFT, PIN,
       RIGHT, ZERO, ZERO, PIN, RIGHT, RIGHT, LEFT, RIGHT, LEFT, LEFT, ZERO,
       ZERO}}},
    {"Upside-down",
     {{PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO, RIGHT, RIGHT, LEFT, LEFT, PIN,
       RIGHT, RIGHT, LEFT, LEFT, ZERO, PIN, PIN, ZERO, ZERO, ZERO, PIN, ZERO}}},
    {"Valley",
     {{ZERO, RIGHT, PIN, LEFT, PIN, RIGHT, PIN, RIGHT, LEFT, RIGHT, ZERO, PIN,
       ZERO, LEFT, RIGHT, LEFT, PIN, LEFT, PIN, RIGHT, PIN, LEFT, ZERO, ZERO}}},
    {"Viaduct",
     {{PIN, RIGHT, PIN, LEFT, PIN, ZERO, ZERO, PIN, RIGHT, ZERO, RIGHT, RIGHT,
       ZERO, RIGHT, PIN, ZERO, ZERO, PIN, LEFT, PIN, RIGHT, PIN, ZERO, ZERO}}},
    {"View",
     {{ZERO, RIGHT, PIN, LEFT, PIN, RIGHT, ZERO, ZERO, RIGHT, PIN, LEFT, LEFT,
       RIGHT, RIGHT, PIN, LEFT, ZERO, ZERO, LEFT, PIN, RIGHT, PIN, LEFT,
       ZERO}}},
    {"Waterfall",
     {{LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, PIN,
       LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT, PIN, LEFT, ZERO, RIGHT,
       ZERO}}},
    {"windwheel",
     {{PIN, RIGHT, RIGHT, PIN, ZERO, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO, LEFT,
       PIN, RIGHT, RIGHT, PIN, ZERO, LEFT, PIN, RIGHT, RIGHT, PIN, ZERO,
       ZERO}}},
    {"Window",
     {{PIN, ZERO, PIN, PIN, ZERO, ZERO, PIN, ZERO, PIN, ZERO, PIN, ZERO, ZERO,
       PIN, ZERO, PIN, ZERO, PIN, PIN, ZERO, ZERO, ZERO, ZERO, ZERO}}},
    {"WindowToTheWorld",
     {{PIN, LEFT, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO, RIGHT, PIN,
       LEFT, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO, PIN, ZERO, ZERO}}},
    {"Windshield",
     {{PIN, PIN, ZERO, RIGHT, PIN, LEFT, LEFT, PIN, RIGHT, ZERO, PIN, ZERO,
       LEFT, PIN, RIGHT, RIGHT, PIN, LEFT, ZERO, PIN, PIN, ZERO, PIN, ZERO}}},
    {"WingNut",
     {{ZERO, ZERO, ZERO, ZERO, PIN, RIGHT, RIGHT, RIGHT, PIN, RIGHT, LEFT, PIN,
       LEFT, RIGHT, PIN, RIGHT, RIGHT, RIGHT, PIN, ZERO, ZERO, ZERO, ZERO,
       ZERO}}},
    {"Wings2",
     {{RIGHT, ZERO, PIN, ZERO, LEFT, PIN, RIGHT, PIN, RIGHT, LEFT, RIGHT, RIGHT,
       LEFT, LEFT, RIGHT, LEFT, PIN, LEFT, PIN, RIGHT, ZERO, PIN, ZERO, ZERO}}},
    {"WithoutName",
     {{PIN, RIGHT, PIN, RIGHT, RIGHT, PIN, LEFT, LEFT, PIN, ZERO, PIN, RIGHT,
       PIN, LEFT, PIN, ZERO, PIN, RIGHT, RIGHT, PIN, LEFT, LEFT, PIN, ZERO}}},
    {"Wolf",
     {{ZERO, ZERO, PIN, PIN, ZERO, PIN, ZERO, ZERO, PIN, ZERO, PIN, PIN, ZERO,
       PIN, ZERO, ZERO, ZERO, PIN, PIN, ZERO, ZERO, ZERO, PIN, ZERO}}},
    {"X",
     {{LEFT, ZERO, ZERO, PIN, LEFT, RIGHT, RIGHT, PIN, LEFT, RIGHT, ZERO, PIN,
       PIN, ZERO, LEFT, RIGHT, PIN, LEFT, LEFT, RIGHT, PIN, ZERO, ZERO, ZERO}}},
};

static size_t models = sizeof(model) / sizeof(struct model_s);

#define VOFFSET 0.045

#define X_MASK 1
#define Y_MASK 2
#define Z_MASK 4

/* the connecting string that holds the snake together */
#define MAGICAL_RED_STRING 0

#define GETSCALAR(vec, mask) ((vec) == (mask) ? 1 : ((vec) == -(mask) ? -1 : 0))

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#define RAND(n) ((random() & 0x7fffffff) % ((long)(n)))
#define RANDSIGN() ((random() & 1) ? 1 : -1)

/* the triangular prism what makes up the basic unit */
static float solid_prism_v[][3] = {
    /* first corner, bottom left front */
    {VOFFSET, VOFFSET, 1.0},
    {VOFFSET, 0.00, 1.0 - VOFFSET},
    {0.00, VOFFSET, 1.0 - VOFFSET},
    /* second corner, rear */
    {VOFFSET, VOFFSET, 0.00},
    {VOFFSET, 0.00, VOFFSET},
    {0.00, VOFFSET, VOFFSET},
    /* third, right front */
    {1.0 - VOFFSET / M_SQRT1_2, VOFFSET, 1.0},
    {1.0 - VOFFSET / M_SQRT1_2, 0.0, 1.0 - VOFFSET},
    {1.0 - VOFFSET * M_SQRT1_2, VOFFSET, 1.0 - VOFFSET},
    /* fourth, right rear */
    {1.0 - VOFFSET / M_SQRT1_2, VOFFSET, 0.0},
    {1.0 - VOFFSET / M_SQRT1_2, 0.0, VOFFSET},
    {1.0 - VOFFSET * M_SQRT1_2, VOFFSET, VOFFSET},
    /* fifth, upper front */
    {VOFFSET, 1.0 - VOFFSET / M_SQRT1_2, 1.0},
    {VOFFSET / M_SQRT1_2, 1.0 - VOFFSET * M_SQRT1_2, 1.0 - VOFFSET},
    {0.0, 1.0 - VOFFSET / M_SQRT1_2, 1.0 - VOFFSET},
    /* sixth, upper rear */
    {VOFFSET, 1.0 - VOFFSET / M_SQRT1_2, 0.0},
    {VOFFSET / M_SQRT1_2, 1.0 - VOFFSET * M_SQRT1_2, VOFFSET},
    {0.0, 1.0 - VOFFSET / M_SQRT1_2, VOFFSET}};

static float solid_prism_n[][3] = {/* corners */
                                   {-VOFFSET, -VOFFSET, VOFFSET},
                                   {VOFFSET, -VOFFSET, VOFFSET},
                                   {-VOFFSET, VOFFSET, VOFFSET},
                                   {-VOFFSET, -VOFFSET, -VOFFSET},
                                   {VOFFSET, -VOFFSET, -VOFFSET},
                                   {-VOFFSET, VOFFSET, -VOFFSET},
                                   /* edges */
                                   {-VOFFSET, 0.0, VOFFSET},
                                   {0.0, -VOFFSET, VOFFSET},
                                   {VOFFSET, VOFFSET, VOFFSET},
                                   {-VOFFSET, 0.0, -VOFFSET},
                                   {0.0, -VOFFSET, -VOFFSET},
                                   {VOFFSET, VOFFSET, -VOFFSET},
                                   {-VOFFSET, -VOFFSET, 0.0},
                                   {VOFFSET, -VOFFSET, 0.0},
                                   {-VOFFSET, VOFFSET, 0.0},
                                   /* faces */
                                   {0.0, 0.0, 1.0},
                                   {0.0, -1.0, 0.0},
                                   {M_SQRT1_2, M_SQRT1_2, 0.0},
                                   {-1.0, 0.0, 0.0},
                                   {0.0, 0.0, -1.0}};

static float wire_prism_v[][3] = {{0.0, 0.0, 1.0},
                                  {1.0, 0.0, 1.0},
                                  {0.0, 1.0, 1.0},
                                  {0.0, 0.0, 0.0},
                                  {1.0, 0.0, 0.0},
                                  {0.0, 1.0, 0.0}};

#if 0
/* this isn't used! */
static float wire_prism_n[][3] = {{ 0.0, 0.0, 1.0},
			   { 0.0,-1.0, 0.0},
			   { M_SQRT1_2, M_SQRT1_2, 0.0},
			   {-1.0, 0.0, 0.0},
			   { 0.0, 0.0,-1.0}};
#endif /* 0 */

static struct glsnake_cfg *glc = NULL;
#ifdef HAVE_GLUT
#define bp glc
#endif

typedef float (*morphFunc)(long);

#ifdef HAVE_GLUT
/* forward definitions for GLUT functions */
static void calc_rotation();
static inline void ui_mousedrag();
#endif

static GLfloat white_light[] = {1.0, 1.0, 1.0, 1.0};
static GLfloat yellow_light[] = {1.0, 1.0, 0.0, 0.8};
static GLfloat lmodel_ambient[] = {0.2, 0.2, 0.2, 1.0};
static GLfloat mat_specular[] = {0.1, 0.1, 0.1, 1.0};
static GLfloat mat_shininess[] = {20.0};

void gl_init(
#ifndef HAVE_GLUT
    ModeInfo *mi
#endif
    ) {
  float light_pos[][3] = {{0.0, 10.0, 20.0}, {0.0, 20.0, -1.0}};
  float light_dir[][3] = {{0.0, -10.0, -20.0}, {0.0, -20.0, 1.0}};

  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);
  glCullFace(GL_BACK);
  /*glEnable(GL_CULL_FACE);*/
  glEnable(GL_NORMALIZE);
  if (transparent) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
  }

  if (!wireframe) {
    /*glColor4f(1.0, 1.0, 1.0, 1.0);*/
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos[0]);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_dir[0]);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
/*glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);*/
#if 1
    glLightfv(GL_LIGHT1, GL_POSITION, light_pos[1]);
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, light_dir[1]);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, white_light);
    glLightfv(GL_LIGHT1, GL_SPECULAR, white_light);
#endif
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    /*glEnable(GL_COLOR_MATERIAL);*/
  }
}

static void gettime(snaketime *t) {
#ifdef HAVE_GETTIMEOFDAY
#ifdef GETTIMEOFDAY_TWO_ARGS
  struct timezone tzp;
  gettimeofday(t, &tzp);
#else  /* !GETTIMEOFDAY_TWO_ARGS */
  gettimeofday(t);
#endif /* !GETTIMEOFDAY_TWO_ARGS */
#else  /* !HAVE_GETTIMEOFDAY */
#ifdef HAVE_FTIME
  ftime(t);
#endif /* HAVE_FTIME */
#endif /* !HAVE_GETTIMEOFDAY */
}

static void start_morph(unsigned int model_index, int immediate);
static void start_morph_shape(struct glsnake_shape *shape, int immediate);
static float morph_percent(void);
static int morph_all_at_once(long iter_msec);
static int morph_one_at_a_time(long iter_msec);
static float morph_percent_one_at_a_time(void);

struct morph_method_t {
  morph_func_t morph;
  morph_percent_func_t morph_percent;
};

static struct morph_method_t morph_methods[] = {
    /* rotate all joints at the same time */
    {&morph_all_at_once, &morph_percent},
    /* rotate joints one at a time */
    {&morph_one_at_a_time, &morph_percent_one_at_a_time}};
static size_t MORPH_METHOD_COUNT =
    sizeof(morph_methods) / sizeof(struct morph_method_t);

/* wot initialises it */
void glsnake_init(
#ifndef HAVE_GLUT
    ModeInfo *mi
#endif
    ) {
#ifndef HAVE_GLUT
  struct glsnake_cfg *bp;

  /* set up the conf struct and glx contexts */
  if (!glc) {
    glc = (struct glsnake_cfg *)calloc(MI_NUM_SCREENS(mi),
                                       sizeof(struct glsnake_cfg));
    if (!glc) {
      fprintf(stderr, "%s: out of memory\n", progname);
      exit(1);
    }
  }
  bp = &glc[MI_SCREEN(mi)];

  if ((bp->glx_context = init_GL(mi)) != NULL) {
    gl_init(mi);
    glsnake_reshape(mi, MI_WIDTH(mi), MI_HEIGHT(mi));
  }
#else
  gl_init();
#endif

  /* initialise conf struct */
  memset(&bp->shape.node, 0, sizeof(float) * NODE_COUNT);

  bp->selected = 11;
  bp->is_cyclic = 0;
  bp->is_legal = 1;
  bp->last_turn = -1;
  bp->morphing = 0;
  bp->paused = 0;
  bp->new_morph = 0;
  bp->morph = morph_one_at_a_time;
  bp->morph_percent = morph_percent_one_at_a_time;

  gettime(&bp->last_iteration);
  memcpy(&bp->last_morph, &bp->last_iteration, sizeof(bp->last_morph));

  bp->prev_colour = bp->next_colour = COLOUR_ACYCLIC;
  start_morph(START_MODEL, 1);

/* set up a font for the labels */
#ifndef HAVE_GLUT
  if (titles) load_font(mi->dpy, "labelfont", &bp->font, &bp->font_list);
#endif

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

#ifdef HAVE_GLUT
  /* initialise the rotation */
  calc_rotation();
#endif
}

static void draw_title(
#ifndef HAVE_GLUT
    ModeInfo *mi
#endif
    ) {
#ifndef HAVE_GLUT
  struct glsnake_cfg *bp = &glc[MI_SCREEN(mi)];
#endif

  /* draw some text */
  glPushAttrib((GLbitfield)GL_TRANSFORM_BIT | GL_ENABLE_BIT);
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  if (transparent) {
    glDisable(GL_BLEND);
  }
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
#ifdef HAVE_GLUT
  gluOrtho2D((GLdouble)0., (GLdouble)glc->width, (GLdouble)0.,
             (GLdouble)glc->height);
#else
  gluOrtho2D((GLdouble)0., (GLdouble)mi->xgwa.width, (GLdouble)0.,
             (GLdouble)mi->xgwa.height);
#endif
  glColor4f(1.0, 1.0, 1.0, 1.0);
  {
    char interactstr[] = "interactive";
    const char *s;
#ifdef HAVE_GLUT
    int w;
#endif

    if (interactive)
      s = interactstr;
    else
      s = glc->next_model_s.name;

#ifdef HAVE_GLUT
    {
      unsigned int i = 0;

      w = glutBitmapLength(GLUT_BITMAP_HELVETICA_12, (const unsigned char *)s);
      glRasterPos2f((GLfloat)(glc->width - w - 3), 4.0);
      while (s[i] != '\0')
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, s[i++]);
    }
#else
    print_gl_string(mi->dpy, bp->font, bp->font_list, mi->xgwa.width,
                    mi->xgwa.height, 10.0, (float)mi->xgwa.height - 10.0, s);
#endif
  }
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glPopAttrib();
}

/* apply the matrix to the origin and stick it in vec */
static void matmult_origin(float rotmat[16], float vec[4]) {
#if 1
  vec[0] = 0.5 * rotmat[0] + 0.5 * rotmat[4] + 0.5 * rotmat[8] + 1 * rotmat[12];
  vec[1] = 0.5 * rotmat[1] + 0.5 * rotmat[5] + 0.5 * rotmat[9] + 1 * rotmat[13];
  vec[2] =
      0.5 * rotmat[2] + 0.5 * rotmat[6] + 0.5 * rotmat[10] + 1 * rotmat[14];
  vec[3] =
      0.5 * rotmat[3] + 0.5 * rotmat[7] + 0.5 * rotmat[11] + 1 * rotmat[15];
#else
  vec[0] = 0 * rotmat[0] + 0 * rotmat[1] + 0 * rotmat[2] + 1 * rotmat[3];
  vec[1] = 0 * rotmat[4] + 0 * rotmat[5] + 0 * rotmat[6] + 1 * rotmat[7];
  vec[2] = 0 * rotmat[8] + 0 * rotmat[9] + 0 * rotmat[10] + 1 * rotmat[11];
  vec[3] = 0 * rotmat[12] + 0 * rotmat[13] + 0 * rotmat[14] + 1 * rotmat[15];
#endif
  vec[0] /= vec[3];
  vec[1] /= vec[3];
  vec[2] /= vec[3];
  vec[3] = 1.0;
}

/* wot gets called when the winder is resized */
void glsnake_reshape(
#ifndef HAVE_GLUT
    ModeInfo *mi,
#endif
    int w, int h) {
  glViewport(0, 0, (GLint)w, (GLint)h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(zoom, (GLdouble)w / (GLdouble)h, 0.05, 100.0);
  gluLookAt(0.0, 0.0, 20.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
  glMatrixMode(GL_MODELVIEW);
  /*gluLookAt(0.0, 0.0, 20.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);*/
  glLoadIdentity();
#ifdef HAVE_GLUT
  glc->width = w;
  glc->height = h;
#endif
}

/* Returns the new dst_dir for the given src_dir and dst_dir */
static int cross_product(int src_dir, int dst_dir) {
  return X_MASK * (GETSCALAR(src_dir, Y_MASK) * GETSCALAR(dst_dir, Z_MASK) -
                   GETSCALAR(src_dir, Z_MASK) * GETSCALAR(dst_dir, Y_MASK)) +
         Y_MASK * (GETSCALAR(src_dir, Z_MASK) * GETSCALAR(dst_dir, X_MASK) -
                   GETSCALAR(src_dir, X_MASK) * GETSCALAR(dst_dir, Z_MASK)) +
         Z_MASK * (GETSCALAR(src_dir, X_MASK) * GETSCALAR(dst_dir, Y_MASK) -
                   GETSCALAR(src_dir, Y_MASK) * GETSCALAR(dst_dir, X_MASK));
}

/* calculate orthogonal snake metrics
 *  is_legal  = true if model does not pass through itself
 *  is_cyclic = true if last node connects back to first node
 *  last_turn = for cyclic snakes, specifes what the 24th turn would be
 */
static void calc_snake_metrics_shape(struct glsnake_shape *shape);
static void calc_snake_metrics_model_s(struct model_s *mdl);
static void calc_snake_metrics(void) {
  calc_snake_metrics_model_s(&glc->next_model_s);
}

static void calc_snake_metrics_model_s(struct model_s *mdl) {
  calc_snake_metrics_shape(&mdl->shape);
}

static void calc_snake_metrics_shape(struct glsnake_shape *shape) {
  int srcDir, dstDir;
  int i, x, y, z;
  int prevSrcDir = -Y_MASK;
  int prevDstDir = Z_MASK;
  int grid[25][25][25];

  /* zero the grid */
  memset(&grid, 0, sizeof(int) * 25 * 25 * 25);

  glc->is_legal = 1;
  x = y = z = 12;

  /* trace path of snake - and keep record for is_legal */
  for (i = 0; i < NODE_COUNT - 1; i++) {
    /*int ang_card;*/ /* cardinal direction of node angle */
    /* establish new state vars */
    srcDir = -prevDstDir;
    x += GETSCALAR(prevDstDir, X_MASK);
    y += GETSCALAR(prevDstDir, Y_MASK);
    z += GETSCALAR(prevDstDir, Z_MASK);

    switch ((int)shape->node[i]) {
      case (int)(ZERO):
        dstDir = -prevSrcDir;
        break;
      case (int)(PIN):
        dstDir = prevSrcDir;
        break;
      case (int)(RIGHT):
      case (int)(LEFT):
        dstDir = cross_product(prevSrcDir, prevDstDir);
        if (fabs(shape->node[i] - RIGHT) < FLT_EPSILON) dstDir = -dstDir;
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
  if (glc->is_cyclic) switch (srcDir) {
      case -Z_MASK:
        glc->last_turn = ZERO;
        break;
      case Z_MASK:
        glc->last_turn = PIN;
        break;
      case X_MASK:
        glc->last_turn = LEFT;
        break;
      case -X_MASK:
        glc->last_turn = RIGHT;
        break;
    }
}

int spooky(void) {
  time_t t;
  struct tm *tm_p;
  if ((t = time(NULL)) == -1) {
    return 0;
  }
  if ((tm_p = localtime(&t)) == NULL) {
    return 0;
  }
  return ((tm_p->tm_mon == 9 && tm_p->tm_mday == 31));
}

/* Work out how far through the current morph we are.  Used by morph_colour. */
static float morph_percent(void) {
  float retval;
  int i;

  /* extend this function later with a case statement for each of the
   * morph schemes */

  /* when morphing all nodes at once, the longest morph will be the node
   * that needs to rotate 180 degrees.  For each node, work out how far it
   * has to go, and store the maximum rotation and current largest angular
   * difference, returning the angular difference over the maximum. */
  {
    float rot_max = 0.0, ang_diff_max = 0.0;

    for (i = 0; i < NODE_COUNT - 1; i++) {
      float rot, ang_diff;

      /* work out the maximum rotation this node has to go through
       * from the previous to the next model, taking into account that
       * the snake always morphs through the smaller angle */
      rot = fabs(glc->prev_model_s.shape.node[i] -
                 glc->next_model_s.shape.node[i]);
      if (rot > 180.0) rot = 180.0 - rot;
      /* work out the difference between the current position and the
       * target */
      ang_diff = fabs(glc->shape.node[i] - glc->next_model_s.shape.node[i]);
      if (ang_diff > 180.0) ang_diff = 180.0 - ang_diff;
      /* if it's the biggest so far, record it */
      if (rot > rot_max) rot_max = rot;
      if (ang_diff > ang_diff_max) ang_diff_max = ang_diff;
    }

    /* ang_diff / rot approaches 0, we want the complement */
    retval = 1.0 - (ang_diff_max / rot_max);
/* protect against naan */

/* Apparently some systems (Solaris) don't have isinf() */
#undef isinf
#define isinf(x) (((x) > 999999999999.9) || ((x) < -999999999999.9))

    if (isnan(retval) || isinf(retval)) retval = 1.0;
  }
  /*printf("morph_pct = %f\n", retval);*/
  return retval;
}

static void morph_colour(void) {
  float percent, compct; /* complement of percentage */

  percent = glc->morph_percent();
  compct = 1.0 - percent;

  glc->colour[0][0] = colour[glc->prev_colour][0][0] * compct +
                      colour[glc->next_colour][0][0] * percent;
  glc->colour[0][1] = colour[glc->prev_colour][0][1] * compct +
                      colour[glc->next_colour][0][1] * percent;
  glc->colour[0][2] = colour[glc->prev_colour][0][2] * compct +
                      colour[glc->next_colour][0][2] * percent;
  glc->colour[0][3] = colour[glc->prev_colour][0][3] * compct +
                      colour[glc->next_colour][0][3] * percent;

  glc->colour[1][0] = colour[glc->prev_colour][1][0] * compct +
                      colour[glc->next_colour][1][0] * percent;
  glc->colour[1][1] = colour[glc->prev_colour][1][1] * compct +
                      colour[glc->next_colour][1][1] * percent;
  glc->colour[1][2] = colour[glc->prev_colour][1][2] * compct +
                      colour[glc->next_colour][1][2] * percent;
  glc->colour[1][3] = colour[glc->prev_colour][1][3] * compct +
                      colour[glc->next_colour][1][3] * percent;
}

/* Start morph process to this model */
static void start_morph(unsigned int model_index, int immediate) {
  start_morph_shape(&model[model_index].shape, immediate);
  glc->next_model_s.name = model[model_index].name;
  glc->preset_index = model_index;
}

static void start_morph_shape(struct glsnake_shape *shape, int immediate) {
  /* if immediate, don't bother morphing, go straight to the next model */
  if (immediate) {
    int i;

    for (i = 0; i < NODE_COUNT; i++) glc->shape.node[i] = shape->node[i];
  }

  memcpy(&glc->prev_model_s, &glc->next_model_s, sizeof(struct model_s));
  memcpy(&glc->next_model_s.shape, shape, sizeof(struct glsnake_shape));
  glc->next_model_s.name = "(XXX)";
  glc->prev_colour = glc->next_colour;

  calc_snake_metrics();
  if (!glc->is_legal)
    glc->next_colour = COLOUR_INVALID;
  else if (altcolour) {
    if (spooky()) {
      glc->next_colour = COLOUR_SPOOKY;
    } else {
      glc->next_colour = COLOUR_AUTHENTIC;
    }
  } else if (glc->is_cyclic)
    glc->next_colour = COLOUR_CYCLIC;
  else
    glc->next_colour = COLOUR_ACYCLIC;

  if (immediate) {
    glc->colour[0][0] = colour[glc->next_colour][0][0];
    glc->colour[0][1] = colour[glc->next_colour][0][1];
    glc->colour[0][2] = colour[glc->next_colour][0][2];
    glc->colour[0][3] = colour[glc->next_colour][0][3];
    glc->colour[1][0] = colour[glc->next_colour][1][0];
    glc->colour[1][1] = colour[glc->next_colour][1][1];
    glc->colour[1][2] = colour[glc->next_colour][1][2];
    glc->colour[1][3] = colour[glc->next_colour][1][3];
  } else {
    /* Randomly select the next morph method */
    int morph_method = RAND(MORPH_METHOD_COUNT);
    glc->morph = morph_methods[morph_method].morph;
    glc->morph_percent = morph_methods[morph_method].morph_percent;
    /* Signal to the morph method that this is a new morph */
    glc->new_morph = 1;
  }
  glc->morphing = 1;

  morph_colour();
}

#if 0
/* HOLY SHIT THIS FUNCTION ISN'T EVEN USED */
/* Returns morph progress */
static float morph(long iter_msec) {
    /* work out the maximum angle for this iteration */
    int still_morphing;
    float iter_angle_max, largest_diff, largest_progress;
    int i;

    if (glc->new_morph)
	glc->new_morph = 0;
	
    iter_angle_max = 90.0 * (angvel/1000.0) * iter_msec;
	
    still_morphing = 0;
    largest_diff = largest_progress = 0.0;
    for (i = 0; i < NODE_COUNT; i++) {
	float curAngle = glc->node[i];
	float destAngle = model[glc->next_model].node[i];
	if (curAngle != destAngle) {
	    still_morphing = 1;
	    if (fabs(curAngle-destAngle) <= iter_angle_max)
		glc->node[i] = destAngle;
	    else if (fmod(curAngle-destAngle+360,360) > 180)
		glc->node[i] = fmod(curAngle + iter_angle_max, 360);
	    else
		glc->node[i] = fmod(curAngle+360 - iter_angle_max, 360);
	    largest_diff = MAX(largest_diff, fabs(destAngle-glc->node[i]));
	    largest_progress = MAX(largest_diff, fabs(glc->node[i] - model[glc->prev_model].node[i]));
	}
    }
	
    return MIN(largest_diff / largest_progress, 1.0);
}
#endif /* 0 */

/* Allocate a new undo entry in undo_ring_buffer, and return its index. */
int push_undo_entry() {
  /* calculate new ring buffer indices */
  undo_ring_end++;
  undo_ring_end %= UNDO_LENGTH;
  if (undo_ring_end == undo_ring_start) {
    /* We have a full buffer.  We're about to overwrite the oldest entry,
     * so increment the start pointer. */
    undo_ring_start++;
    undo_ring_start %= UNDO_LENGTH;
  }
  return undo_ring_end;
}

/* Deallocate the most recent undo entry, and return its index.
 *
 * Make sure you use or copy the entry before doing another push, or it may be
 * overwritten!
 *
 * Returns -1 if there are no entries to pop. */
int pop_undo_entry() {
  int entry;
  if (undo_ring_start == undo_ring_end) return -1;
  entry = undo_ring_end--;
  undo_ring_end %= UNDO_LENGTH;
  return entry;
}

/* Store the current snake shape */
void save_snake_state() {
  struct glsnake_shape *undo_shape = &undo_ring_buffer[push_undo_entry()];
  /* By "current snake shape", we mean the shape we are currently
   * transitioning to, rather than the currently displayed shape.  i.e., if we
   * are mid-transition, we don't capture the half-transitioned shape, but the
   * destination shape. */
  memcpy(undo_shape, &glc->next_model_s.shape, sizeof(struct glsnake_shape));
}

#ifdef HAVE_GLUT
static void glsnake_idle();

static void restore_idle(int v ATTRIBUTE_UNUSED) { glutIdleFunc(glsnake_idle); }
#endif

static void quick_sleep(void) {
#ifdef HAVE_GLUT
  /* By using glutTimerFunc we can keep responding to
   * mouse and keyboard events, unlike using something like
   * usleep. */
  glutIdleFunc(NULL);
  glutTimerFunc(5, restore_idle, 0);
#else
  usleep(1);
#endif
}

/* returns a flag indicating if any rotation happened */
int rotate_joint(int current_node, float iter_angle_max) {
  struct glsnake_shape *shape = &(glc->shape);
  float cur_angle = shape->node[current_node];
  float dest_angle = glc->next_model_s.shape.node[current_node];
  int rotated = 0;

  if (cur_angle != dest_angle) {
    rotated = 1;
  }
  if (fabs(cur_angle - dest_angle) <= iter_angle_max)
    shape->node[current_node] = dest_angle;
  else if (fmod(cur_angle - dest_angle + 360, 360) > 180)
    shape->node[current_node] = fmod(cur_angle + iter_angle_max, 360);
  else
    shape->node[current_node] = fmod(cur_angle + 360 - iter_angle_max, 360);

  return rotated;
}

/* returns a flag indicating if this morph is complete */
static int morph_all_at_once(long iter_msec) {
  int i, still_morphing = 0;
  /* work out the maximum angle we could turn this node in this
   * timeslice, iter_msec milliseconds long */
  float iter_angle_max = 90.0 * (angvel / 1000.0) * iter_msec;

  for (i = 0; i < NODE_COUNT; i++) {
    int rotated = rotate_joint(i, iter_angle_max);
    if (rotated) {
      still_morphing = 1;
    }
  }
  return still_morphing;
}

static int morph_one_at_time_current_node = 0;

static int morph_one_at_a_time(long iter_msec) {
  int current_node = morph_one_at_time_current_node;
  struct glsnake_shape *shape = &(glc->shape);
  /* work out the maximum angle we could turn this node in this
   * timeslice, iter_msec milliseconds long */
  float iter_angle_max = 90.0 * (angvel / 1000.0) * iter_msec;

  if (glc->new_morph) {
    current_node = morph_one_at_time_current_node = 0;
    glc->new_morph = 0;
  }

  /* find the next angle (possibly the current one) to rotate */
  while (shape->node[current_node] ==
         glc->next_model_s.shape.node[current_node]) {
    current_node++;
    if (current_node == NODE_COUNT) {
      /* all joints are at their destination, so we're done morphing */
      morph_one_at_time_current_node = 0;
      return 0;
    }
  }
  morph_one_at_time_current_node = current_node;

  rotate_joint(current_node, iter_angle_max);

  return 1;
}

static float morph_percent_one_at_a_time(void) {
  return morph_one_at_time_current_node / NODE_COUNT;
}

void glsnake_idle(
#ifndef HAVE_GLUT
    struct glsnake_cfg *bp
#endif
    ) {
  /* time since last iteration */
  long iter_msec;
  /* time since the beginning of last morph */
  long morf_msec;
  snaketime current_time;
  int still_morphing;

  /* Do nothing to the model if we are paused */
  if (glc->paused) {
    /* Avoid busy waiting when nothing is changing */
    quick_sleep();
#ifdef HAVE_GLUT
    glutPostRedisplay();
#endif
    return;
  }

  /* <spiv> Well, ftime gives time with millisecond resolution.
   * <spiv> (or worse, perhaps... who knows what the OS will do)
   * <spiv> So if no discernable amount of time has passed:
   * <spiv>   a) There's no point updating the screen, because
   *             it would be the same
   * <spiv>   b) The code will divide by zero
   */
  gettime(&current_time);

  iter_msec =
      (long)GETMSECS(current_time) - GETMSECS(glc->last_iteration) +
      ((long)GETSECS(current_time) - GETSECS(glc->last_iteration)) * 1000L;

  if (iter_msec) {
    /* save the current time */
    memcpy(&glc->last_iteration, &current_time, sizeof(snaketime));

    /* work out if we have to switch models */
    morf_msec =
        GETMSECS(glc->last_iteration) - GETMSECS(glc->last_morph) +
        ((long)(GETSECS(glc->last_iteration) - GETSECS(glc->last_morph)) *
         1000L);

    if ((morf_msec > statictime) && !interactive && !glc->morphing) {
      /*printf("starting morph\n");*/
      memcpy(&glc->last_morph, &(glc->last_iteration), sizeof(glc->last_morph));
      start_morph(RAND(models), 0);
    }

    if (interactive && !glc->morphing) {
      quick_sleep();
      return;
    }

    /*	if (!glc->dragging && !glc->interactive) { */
    if (!interactive) {
      yspin += 360 / ((1000 / yangvel) / iter_msec);
      zspin += 360 / ((1000 / zangvel) / iter_msec);
      /*
      yspin += 360 * (yangvel/1000.0) * iter_msec;
      zspin += 360 * (zangvel/1000.0) * iter_msec;
      */

      /*printf("yspin: %f, zspin: %f\n", yspin, zspin);*/
    }

    still_morphing = glc->morph(iter_msec);

    if (!still_morphing) {
      glc->morphing = 0;
    }

    /* colour cycling */
    morph_colour();

#ifdef HAVE_GLUT
    glutPostRedisplay();
#endif
  } else {
    /* We are going too fast, so we may as well let the
     * cpu relax a little by sleeping for a millisecond. */
    quick_sleep();
  }
}

/* wot draws it */
void glsnake_display(
#ifndef HAVE_GLUT
    ModeInfo *mi
#endif
    ) {
#ifndef HAVE_GLUT
  struct glsnake_cfg *bp = &glc[MI_SCREEN(mi)];
  Display *dpy = MI_DISPLAY(mi);
  Window window = MI_WINDOW(mi);
#endif
  int i;
  float ang;
  float positions[NODE_COUNT][4]; /* origin points for each node */
  float com[4];                   /* it's the CENTRE of MASS */

#ifndef HAVE_GLUT
  if (!bp->glx_context) return;
#endif

  /* clear the buffer */
  glClear((GLbitfield)GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* go into the modelview stack */
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  /* get the centre of each node, by moving through the snake and
   * performing the rotations, then grabbing the matrix at each point
   * and applying it to the origin */
  glPushMatrix();

#ifdef HAVE_GLUT
  /* apply the mouse drag rotation */
  ui_mousedrag();
#endif

  /* apply the continuous rotation */
  glRotatef(yspin, 0.0, 1.0, 0.0);
  glRotatef(zspin, 0.0, 0.0, 1.0);

  com[0] = 0.0;
  com[1] = 0.0;
  com[2] = 0.0;
  com[3] = 0.0;
  for (i = 0; i < NODE_COUNT; i++) {
    float rotmat[16];

    ang = glc->shape.node[i];

    /*printf("ang = %f\n", ang);*/

    glTranslatef(0.5, 0.5, 0.5);           /* move to center */
    glRotatef(90.0, 0.0, 0.0, -1.0);       /* reorient  */
    glTranslatef(1.0 + explode, 0.0, 0.0); /* move to new pos. */
    glRotatef(180.0 + ang, 1.0, 0.0, 0.0); /* pivot to new angle */
    glTranslatef(-0.5, -0.5, -0.5);        /* return from center */

    glGetFloatv(GL_MODELVIEW_MATRIX, rotmat);

    matmult_origin(rotmat, positions[i]);

    /*printf("positions %f %f %f %f\n", positions[i][0], positions[i][1],
     * positions[i][2], positions[i][3]);*/

    com[0] += positions[i][0];
    com[1] += positions[i][1];
    com[2] += positions[i][2];
    com[3] += positions[i][3];
  }
  glPopMatrix();
  com[0] /= NODE_COUNT;
  com[1] /= NODE_COUNT;
  com[2] /= NODE_COUNT;
  com[3] /= NODE_COUNT;

  com[0] /= com[3];
  com[1] /= com[3];
  com[2] /= com[3];

/*printf("com: %f, %f, %f, %f\n", com[0], com[1], com[2], com[3]);*/

#if MAGICAL_RED_STRING
  glPushMatrix();
  glTranslatef(-com[0], -com[1], -com[2]);

  glDisable(GL_LIGHTING);
  glColor4f(1.0, 0.0, 0.0, 1.0);
  glBegin(GL_LINE_STRIP);
  for (i = 0; i < NODE_COUNT - 1; i++) {
    glVertex3fv(positions[i]);
  }
  glEnd();
  glEnable(GL_LIGHTING);
  /*glTranslatef(com[0], com[1], com[2]);*/
  glPopMatrix();
#endif

  glPushMatrix();
  glTranslatef(-com[0], -com[1], -com[2]);

#ifdef HAVE_GLUT
  /* apply the mouse drag rotation */
  ui_mousedrag();
#endif

  /* apply the continuous rotation */
  glRotatef(yspin, 0.0, 1.0, 0.0);
  glRotatef(zspin, 0.0, 0.0, 1.0);

  /* now draw each node along the snake -- this is quite ugly :p */
  for (i = 0; i < NODE_COUNT; i++) {
    /* choose a colour for this node */
    if ((i == glc->selected || i == glc->selected + 1) && interactive)
      if (wireframe) {
        glColor4fv(yellow_light);
      } else {
        glMaterialfv(GL_FRONT, GL_DIFFUSE, yellow_light);
      }
    else {
      if (wireframe) {
        glColor4fv(glc->colour[(i + 1) % 2]);
      } else {
        glMaterialfv(GL_FRONT, GL_AMBIENT, glc->colour[(i + 1) % 2]);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, glc->colour[(i + 1) % 2]);
        /*glMaterialfv(GL_FRONT, GL_SPECULAR, glc->colour[(i+1)%2]);*/
      }
    }

    /* draw the node */
    if (wireframe)
      glCallList(glc->node_wire);
    else
      glCallList(glc->node_solid);

    /* now work out where to draw the next one */

    /* Interpolate between models */
    ang = glc->shape.node[i];

    glTranslatef(0.5, 0.5, 0.5);           /* move to center */
    glRotatef(90.0, 0.0, 0.0, -1.0);       /* reorient  */
    glTranslatef(1.0 + explode, 0.0, 0.0); /* move to new pos. */
    glRotatef(180.0 + ang, 1.0, 0.0, 0.0); /* pivot to new angle */
    glTranslatef(-0.5, -0.5, -0.5);        /* return from center */
  }

  glPopMatrix();

  if (titles)
#ifdef HAVE_GLUT
    draw_title();
#else
    draw_title(mi);
#endif

#ifndef HAVE_GLUT
  glsnake_idle(bp);
#endif

  glFlush();
#ifdef HAVE_GLUT
  glutSwapBuffers();
#else
  glXSwapBuffers(dpy, window);
#endif
}

#ifdef HAVE_GLUT
/* anything that needs to be cleaned up goes here */
static void unmain() {
  glutDestroyWindow(glc->window);
  free(glc);
}

static void ui_init(int *, char **);

int main(int argc, char **argv) {
  glc = malloc(sizeof(struct glsnake_cfg));
  memset(glc, 0, sizeof(struct glsnake_cfg));

  glc->width = 640;
  glc->height = 480;

  ui_init(&argc, argv);

  gettime(&glc->last_iteration);
  memcpy(&glc->last_morph, &glc->last_iteration, sizeof(snaketime));
  srand((unsigned int)GETSECS(glc->last_iteration));

  glc->prev_colour = glc->next_colour =
      spooky() ? COLOUR_SPOOKY : COLOUR_ACYCLIC;

  glsnake_init();

  atexit(unmain);
  glutSwapBuffers();
  glutMainLoop();

  return 0;
}
#endif

/*
 * GLUT FUNCTIONS
 */

#ifdef HAVE_GLUT

/* trackball quaternions */
static float cumquat[4] = {0.0, 0.0, 0.0, 0.0},
             oldquat[4] = {0.0, 0.0, 0.0, 0.1};

/* rotation matrix */
static float rotation[16];

/* mouse drag vectors: start and end */
static float mouse_start[3], mouse_end[3];

/* dragging boolean */
static int dragging = 0;

/* this function calculates the rotation matrix based on the quaternions
 * generated from the mouse drag vectors */
static void calc_rotation() {
  double Nq, s;
  double xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;

  /* this bit ripped from Shoemake's quaternion notes from SIGGRAPH */
  Nq = cumquat[0] * cumquat[0] + cumquat[1] * cumquat[1] +
       cumquat[2] * cumquat[2] + cumquat[3] * cumquat[3];
  s = (Nq > 0.0) ? (2.0 / Nq) : 0.0;
  xs = cumquat[0] * s;
  ys = cumquat[1] * s;
  zs = cumquat[2] * s;
  wx = cumquat[3] * xs;
  wy = cumquat[3] * ys;
  wz = cumquat[3] * zs;
  xx = cumquat[0] * xs;
  xy = cumquat[0] * ys;
  xz = cumquat[0] * zs;
  yy = cumquat[1] * ys;
  yz = cumquat[1] * zs;
  zz = cumquat[2] * zs;

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

static void ui_mousedrag() { glMultMatrixf(rotation); }

static void ui_keyboard(unsigned char c, int x ATTRIBUTE_UNUSED,
                        int y ATTRIBUTE_UNUSED) {
  switch (c) {
    case 27: /* ESC */
    case 'q':
      exit(0);
      break;
    case 'e':
      explode += DEF_EXPLODE;
      glutPostRedisplay();
      break;
    case 'E':
      explode -= DEF_EXPLODE;
      if (explode < 0.0) explode = 0.0;
      glutPostRedisplay();
      break;
    case '.':
      /* next model */
      save_snake_state();
      glc->preset_index++;
      glc->preset_index %= models;
      start_morph(glc->preset_index, 0);

      /* Reset last_morph time */
      gettime(&glc->last_morph);
      break;
    case ',':
      /* previous model */
      save_snake_state();
      glc->preset_index = (glc->preset_index + (int)models - 1) % (int)models;
      start_morph(glc->preset_index, 0);

      /* Reset glc->last_morph time */
      gettime(&glc->last_morph);
      break;
    case '+':
      angvel += DEF_ANGVEL;
      break;
    case '-':
      if (angvel > DEF_ANGVEL) angvel -= DEF_ANGVEL;
      break;
    case 'i':
      if (interactive) {
        /* Reset last_iteration and last_morph time */
        gettime(&glc->last_iteration);
        gettime(&glc->last_morph);
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
    case 'a':
      transparent = 1 - transparent;
      if (transparent) {
        glEnable(GL_BLEND);
      } else {
        glDisable(GL_BLEND);
      }
      break;
    case 'p':
      if (glc->paused) {
        /* unpausing, reset last_iteration and last_morph time */
        gettime(&glc->last_iteration);
        gettime(&glc->last_morph);
      }
      glc->paused = 1 - glc->paused;
      break;
    case 'd':
      /* dump the current model so we can add it! */
      printf("# %s\nnoname:\t", glc->next_model_s.name);
      {
        int i;
        struct glsnake_shape *shape = &(glc->shape);

        for (i = 0; i < NODE_COUNT; i++) {
          if (shape->node[i] == ZERO)
            printf("Z");
          else if (shape->node[i] == LEFT)
            printf("L");
          else if (shape->node[i] == PIN)
            printf("P");
          else if (shape->node[i] == RIGHT)
            printf("R");
          /*
            else
            printf("%f", node[i].curAngle);
          */
          if (i < NODE_COUNT - 1) printf(" ");
        }
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
        glutPositionWindow(50, 50);
      }
      break;
    case 't':
      titles = 1 - titles;
      if (interactive || glc->paused) glutPostRedisplay();
      break;
    case 'c':
      altcolour = 1 - altcolour;
      break;
    case 'z':
      zoom += 1.0;
      glsnake_reshape(glc->width, glc->height);
      break;
    case 'Z':
      zoom -= 1.0;
      glsnake_reshape(glc->width, glc->height);
      break;
    case 'u': {
      int undo_idx = pop_undo_entry();
      if (undo_idx != -1) {
        memcpy(&glc->next_model_s.shape, &undo_ring_buffer[undo_idx],
               sizeof(struct glsnake_shape));
        glc->morphing = glc->new_morph = 1;
      }
    } break;
    default:
      break;
  }
}

static void ui_special(int key, int x ATTRIBUTE_UNUSED,
                       int y ATTRIBUTE_UNUSED) {
  float *destAngle = &(glc->next_model_s.shape.node[glc->selected]);
  int unknown_key = 0;

  if (interactive) {
    switch (key) {
      case GLUT_KEY_UP:
        glc->selected = (glc->selected + (NODE_COUNT - 2)) % (NODE_COUNT - 1);
        break;
      case GLUT_KEY_DOWN:
        glc->selected = (glc->selected + 1) % (NODE_COUNT - 1);
        break;
      case GLUT_KEY_LEFT:
        save_snake_state();
        *destAngle = fmod(*destAngle + (LEFT), 360);
        glc->morphing = glc->new_morph = 1;
        break;
      case GLUT_KEY_RIGHT:
        save_snake_state();
        *destAngle = fmod(*destAngle + (RIGHT), 360);
        glc->morphing = glc->new_morph = 1;
        break;
      case GLUT_KEY_HOME:
        save_snake_state();
        start_morph(STRAIGHT_MODEL, 0);
        break;
      default:
        unknown_key = 1;
        break;
    }
  }

  calc_snake_metrics();

  if (!unknown_key) glutPostRedisplay();
}

static inline void ui_mouse(int button, int state, int x, int y) {
  if (button == 0) {
    switch (state) {
      case GLUT_DOWN:
        dragging = 1;
        mouse_start[0] =
            M_SQRT1_2 * (x - (glc->width / 2.0)) / (glc->width / 2.0);
        mouse_start[1] =
            M_SQRT1_2 * ((glc->height / 2.0) - y) / (glc->height / 2.0);
        mouse_start[2] = sqrt((double)(1 - (mouse_start[0] * mouse_start[0] +
                                            mouse_start[1] * mouse_start[1])));
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

static void ui_motion(int x, int y) {
  double norm;
  float q[4];

  if (dragging) {
    /* construct the motion end vector from the x,y position on the
     * window */
    mouse_end[0] = M_SQRT1_2 * (x - (glc->width / 2.0)) / (glc->width / 2.0);
    mouse_end[1] = M_SQRT1_2 * ((glc->height / 2.0) - y) / (glc->height / 2.0);
    /* calculate the normal of the vector... */
    norm = mouse_end[0] * mouse_end[0] + mouse_end[1] * mouse_end[1];
    /* check if norm is outside the sphere and wraparound if necessary */
    if (norm > 1.0) {
      mouse_end[0] = -mouse_end[0];
      mouse_end[1] = -mouse_end[1];
      mouse_end[2] = sqrt(norm - 1);
    } else {
      /* the z value comes from projecting onto an elliptical spheroid */
      mouse_end[2] = sqrt(1 - norm);
    }

    /* now here, build a quaternion from mouse_start and mouse_end */
    q[0] = mouse_start[1] * mouse_end[2] - mouse_start[2] * mouse_end[1];
    q[1] = mouse_start[2] * mouse_end[0] - mouse_start[0] * mouse_end[2];
    q[2] = mouse_start[0] * mouse_end[1] - mouse_start[1] * mouse_end[0];
    q[3] = mouse_start[0] * mouse_end[0] + mouse_start[1] * mouse_end[1] +
           mouse_start[2] * mouse_end[2];

    /* new rotation is the product of the new one and the old one */
    cumquat[0] = q[3] * oldquat[0] + q[0] * oldquat[3] + q[1] * oldquat[2] -
                 q[2] * oldquat[1];
    cumquat[1] = q[3] * oldquat[1] + q[1] * oldquat[3] + q[2] * oldquat[0] -
                 q[0] * oldquat[2];
    cumquat[2] = q[3] * oldquat[2] + q[2] * oldquat[3] + q[0] * oldquat[1] -
                 q[1] * oldquat[0];
    cumquat[3] = q[3] * oldquat[3] - q[0] * oldquat[0] - q[1] * oldquat[1] -
                 q[2] * oldquat[2];

    calc_rotation();
  }
  glutPostRedisplay();
}

static void ui_init(int *argc, char **argv) {
  glutInit(argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(glc->width, glc->height);
  glc->window = glutCreateWindow("glsnake");

  glutDisplayFunc(glsnake_display);
  glutReshapeFunc(glsnake_reshape);
  glutIdleFunc(glsnake_idle);
  glutKeyboardFunc(ui_keyboard);
  glutSpecialFunc(ui_special);
  glutMouseFunc(ui_mouse);
  glutMotionFunc(ui_motion);

  yangvel = DEF_YANGVEL;
  zangvel = DEF_ZANGVEL;
  explode = DEF_EXPLODE;
  angvel = DEF_ANGVEL;
  statictime = DEF_STATICTIME;
  altcolour = DEF_ALTCOLOUR;
  titles = DEF_TITLES;
  interactive = DEF_INTERACTIVE;
  zoom = DEF_ZOOM;
  wireframe = DEF_WIREFRAME;
  transparent = DEF_TRANSPARENT;
  undo_ring_start = 0;
  undo_ring_end = 0;
}
#endif /* HAVE_GLUT */
