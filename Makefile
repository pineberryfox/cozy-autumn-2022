.MAIN : main
.SUFFIXES : .c .o .tmx

PATH := $(.CURDIR)/Tools:$(PATH)
.export-env PATH

CFLAGS+= -DSTBI_ONLY_PNG
CFLAGS+= -DSTBI_MAX_DIMENSIONS=512
CFLAGS+= -I/Library/Frameworks/SDL2.framework/Headers
CFLAGS+= -I$(.CURDIR)/stb
CFLAGS+= -mmacosx-version-min=10.7 -arch arm64
CFLAGS+= -Wall
LDFLAGS+= -F/Library/Frameworks
LDLIBS+= -framework SDL2
LDLIBS+= -framework Cocoa

.tmx.c:
	xpath -q -e '//layer[@name="Layout"]/data/text()' < "$(.IMPSRC)" \
	| extract-layout.awk \
	| bin2c "$(.TARGET:T:R)" > "$(.TARGET)"

main : main.o boids2d.o console.o framebuffer.o graphics.o
main : levels.o lv00.o player.o
main :
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(.TARGET) $(.ALLSRC:M*.o) $(LDLIBS)

boids2d.o : boids2d.c boids2d.h
console.o : console.c console.h framebuffer.h
framebuffer.o : framebuffer.c framebuffer.h
graphics.o : graphics.c console.h graphics.h
levels.o : levels.c levels.h
lv00.o : lv00.c
main.o : main.c boids2d.h console.h entity.h framebuffer.h
main.o : graphics.h levels.h player.h
player.o : player.c entity.h player.h
