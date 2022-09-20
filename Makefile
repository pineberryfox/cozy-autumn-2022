EXE=
CFLAGS+= -I$(.CURDIR)/stb
.ifndef(OS)
.OBJDIR : $(.CURDIR)/web
CC=emcc
CFLAGS+= -O2 -s USE_SDL=2
LDFLAGS+= --preload-file $(.CURDIR)/assets@
LDFLAGS+= --shell-file $(.CURDIR)/emcc-template.html
EXE=.html
.elif $(OS) == Darwin
CFLAGS+= -Oz
CFLAGS+= -DSTBI_ONLY_PNG
CFLAGS+= -DSTBI_MAX_DIMENSIONS=512
CFLAGS+= -I/Library/Frameworks/SDL2.framework/Headers
CFLAGS+= -mmacosx-version-min=10.7 -arch arm64 -arch x86_64
CFLAGS+= -Wall
LDFLAGS+= -F/Library/Frameworks
LDLIBS+= -framework SDL2
LDLIBS+= -framework Cocoa
.endif

.MAIN : main$(EXE)
.SUFFIXES : .c .o .tmx

PATH := $(.CURDIR)/Tools:$(PATH)
.export-env PATH

.tmx.c:
	xpath -q -e '//layer[@name="Layout"]/data/text()' < "$(.IMPSRC)" \
	| extract-layout.awk "$(.TARGET:T:R)" > "$(.TARGET)"
.c.o:
	$(CC) $(CFLAGS) -o $(.TARGET) -c $(.IMPSRC)

main$(EXE) : main.o boids2d.o console.o framebuffer.o graphics.o
main$(EXE) : levels.o lv00.o player.o
.if $(CC) == emcc
main$(EXE) : emcc-template.html
.endif
main$(EXE) :
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(.TARGET) $(.ALLSRC:M*.o) $(LDLIBS)

boids2d.o : boids2d.c boids2d.h
console.o : console.c console.h framebuffer.h
framebuffer.o : framebuffer.c framebuffer.h
graphics.o : graphics.c console.h graphics.h
levels.o : levels.c common.h levels.h
lv00.o : lv00.c
main.o : main.c boids2d.h common.h console.h entity.h framebuffer.h
main.o : graphics.h levels.h player.h
player.o : player.c entity.h player.h

web.zip : index.html
	zip -9XDj $(.TARGET) index.html main.data main.js main.wasm
index.html : main.html
	cp $(.ALLSRC) $(.TARGET)
