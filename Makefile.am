## Process this file with automake to create Makefile.in

bin_PROGRAMS = glsnake
glsnake_SOURCES = glsnake.c loader.c
noinst_HEADERS = model.h
man_MANS = glsnake.6

SUBDIRS = . tools data pixmaps

EXTRA_DIST = config/install-sh config/missing config/mkinstalldirs \
			 glsnake.6

ChangeLog:
	cvs2cl -S --no-wrap

.PHONY: ChangeLog
