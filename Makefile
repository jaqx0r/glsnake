VERSION=0.7.6
LDFLAGS=-L/home/cs9018/lib-pc.i86.linux -L/usr/X11R6/lib -lGL -lGLU -lglut -lm -lX11 -lXmu -lXi
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
