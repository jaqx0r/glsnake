## Process this file with automake to create Makefile.in

bin_PROGRAMS = glsnake

glsnake_SOURCES = glsnake.c loader.c
glsnake_CFLAGS = @GLUT_CFLAGS@
glsnake_LDADD = @GLUT_LIBS@

noinst_HEADERS = model.h

man_MANS = glsnake.6

#CPPFLAGS = -DDATADIR=\""$(pkgdatadir)"\" -DDEBUG
AM_CPPFLAGS = -DDATADIR=\""$(pkgdatadir)"\"

SUBDIRS = . tools data pixmaps

EXTRA_DIST = config/install-sh config/missing config/mkinstalldirs \
			 glsnake.6

ChangeLog:
	cvs2cl -S --no-wrap

.PHONY: ChangeLog
