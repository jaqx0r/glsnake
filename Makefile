VERSION=0.7.1
LDFLAGS=-L/usr/X11R6/lib -lGL -lGLU -lglut -lm -lXi -lXmu -lX11
CFLAGS=-O2 -Wall -pedantic

all: glsnake

install:
	install glsnake $(DESTDIR)/usr/bin
	-test ! -d $(DESTDIR)/usr/share/pixmaps/glsnake && mkdir $(DESTDIR)/usr/share/pixmaps/glsnake
	install -m 644 pixmaps/*.png $(DESTDIR)/usr/share/pixmaps/glsnake
	install -m 644 pixmaps/*.xpm $(DESTDIR)/usr/share/pixmaps/glsnake

uninstall:
	rm $(DESTDIR)/usr/bin/glsnake
	rm -rf $(DESTDIR)/usr/share/pixmaps/glsnake

ChangeLog:
	cvs2cl -S --no-wrap

clean:
	rm -f glsnake

.PHONY: ChangeLog clean
