LDFLAGS=-lGL -lGLU -lglut -lm
CFLAGS=-Wall -pedantic

all: glsnake

ChangeLog:
	cvs2cl -S --no-wrap

clean:
	rm -f glsnake ChangeLog

.PHONY: ChangeLog clean
