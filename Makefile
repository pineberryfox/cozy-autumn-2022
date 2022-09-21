EXE=
CFLAGS+= -std=c99
CFLAGS+= -I$(.CURDIR)/stb
CFLAGS+= -DSTBI_ONLY_PNG
CFLAGS+= -DSTBI_MAX_DIMENSIONS=512
CFLAGS+= -Wall -Wextra
.ifndef(OS)
.OBJDIR : $(.CURDIR)/web
CC=emcc
CFLAGS+= -std=gnu99
CFLAGS+= -Oz -ffast-math -s USE_SDL=2
CFLAGS+= -fno-exceptions -flto
LDFLAGS+= --closure 1
LDFLAGS+= -s ENVIRONMENT=web
LDFLAGS+= -s EVAL_CTORS=2
LDFLAGS+= -s JS_MATH=1
LDFLAGS+= -s MALLOC=emmalloc
LDFLAGS+= -s EXPORTED_RUNTIME_METHODS=[allocate]
LDFLAGS+= --preload-file $(.CURDIR)/assets@
LDFLAGS+= --shell-file $(.CURDIR)/emcc-template.html
EXE=.html
.elif $(OS) == Darwin
MACOSX_DEPLOYMENT_TARGET=10.7
.export-env MACOSX_DEPLOYMENT_TARGET
CFLAGS+= -Oz
CFLAGS+= -fno-exceptions -flto
CFLAGS+= -I/Library/Frameworks/SDL2.framework/Headers
CFLAGS+= -arch arm64 -arch x86_64
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

main$(EXE) : main.o boids2d.o console.o eggboss.o entity.o
main$(EXE) : framebuffer.o graphics.o levels.o lv00.o player.o
.if $(CC) == emcc
main$(EXE) : emcc-template.html
.endif
main$(EXE) :
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(.TARGET) $(.ALLSRC:M*.o) $(LDLIBS)

boids2d.o : boids2d.c boids2d.h
console.o : console.c console.h framebuffer.h
eggboss.o : eggboss.c common.h console.h eggboss.h entity.h
eggboss.o : framebuffer.h levels.h player.h
entity.o : entity.c entity.h
framebuffer.o : framebuffer.c framebuffer.h
graphics.o : graphics.c console.h framebuffer.h graphics.h
levels.o : levels.c common.h console.h eggboss.h entity.h
levels.o : framebuffer.h graphics.h levels.h player.h
lv00.o : lv00.c
main.o : main.c boids2d.h common.h console.h entity.h framebuffer.h
main.o : graphics.h levels.h player.h
player.o : player.c common.h console.h entity.h framebuffer.h
player.o : levels.h player.h

web.zip : index.html
	zip -9XDj $(.TARGET) index.html main.data main.js main.wasm
index.html : main.html
	cp $(.ALLSRC) $(.TARGET)
