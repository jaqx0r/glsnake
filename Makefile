LDFLAGS=-lGL -lGLU -lglut -lm
CFLAGS=-Wall -pedantic

all: glsnake

ChangeLog:
	cvs2cl -S --no-wrap
