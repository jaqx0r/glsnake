bin_PROGRAMS = glsnake

wflags = -W -Wall -Werror

glsnake_SOURCES = glsnake.c
glsnake_CFLAGS = $(wflags) @GLUT_CFLAGS@ @OPENGL_CFLAGS@ @XSCREENSAVER_CFLAGS@
glsnake_LDADD = @GLUT_LIBS@ @OPENGL_LIBS@

man_MANS = glsnake.6

pkgdata_DATA = data/models.glsnake data/ericandthomas.glsnake

data/ericandthomas.glsnake:
	tools/getmodels.sh > $@

pixmapdir = $(datadir)/glsnake/pixmaps

pixmap_DATA = pixmaps/glsnake_ball.png pixmaps/glsnake_ball.xpm \
	pixmaps/glsnake_h.png pixmaps/glsnake_h.xpm \
	pixmaps/glsnake_propeller.png pixmaps/glsnake_propeller.xpm \
	pixmaps/glsnake_snowflake.png pixmaps/glsnake_snowflake.xpm

EXTRA_DIST = glsnake.6 \
	tools/getmodels.sh tools/format_convert.py \
	data/models.glsnake data/ericandthomas.glsnake data/snake.c \
	pixmaps/glsnake_ball.png pixmaps/glsnake_ball.xpm \
	pixmaps/glsnake_h.png pixmaps/glsnake_h.xpm \
	pixmaps/glsnake_propeller.png pixmaps/glsnake_propeller.xpm \
	pixmaps/glsnake_snowflake.png pixmaps/glsnake_snowflake.xpm


