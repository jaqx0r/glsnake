## Process this file with automake to create Makefile.in

bin_PROGRAMS = glsnake
glsnake_SOURCES = glsnake.c loader.c model.h

SUBDIRS = . tools data

ChangeLog:
	cvs2cl -S --no-wrap

.PHONY: ChangeLog
