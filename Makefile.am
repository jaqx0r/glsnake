## Process this file with automake to create Makefile.in

bin_PROGRAMS = glsnake

glsnake_SOURCES = glsnake.c
glsnake_CFLAGS = @GLUT_CFLAGS@ @OPENGL_CFLAGS@ @XSCREENSAVER_CFLAGS@
glsnake_LDADD = @GLUT_LIBS@ @OPENGL_LIBS@

man_MANS = glsnake.6

SUBDIRS = . tools data pixmaps

EXTRA_DIST = glsnake.6
