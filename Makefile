# ZMenu VERSION
VERSION = 0.2

PKG_CONFIG = pkg-config

# includes and libs
LIBS = -L/usr/lib -lc -lcrypt -L${X11LIB} -lX11 -lXft\
			 `$(PKG_CONFIG) --libs freetype2`

# flags
CFLAGS = -g -Wall -Wextra -Wpedantic `$(PKG_CONFIG) --cflags freetype2`
LDFLAGS = -lX11 -lXft `$(PKG_CONFIG) --libs freetype2`

zmenu:
	gcc -c -s ${CFLAGS} zmenu.c
	gcc -o zmenu zmenu.o ${LDFLAGS}

clean:
	rm -rf zmenu
