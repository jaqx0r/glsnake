LDFLAGS=-lGL -lGLU -lglut -lm
CFLAGS=-O2 -Wall -pedantic

all: glsnake

ChangeLog:
	cvs2cl -S --no-wrap

clean:
	rm -f glsnake

.PHONY: ChangeLog clean
