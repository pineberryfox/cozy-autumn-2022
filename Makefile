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

main : main.o console.o framebuffer.o graphics.o levels.o lv00.o
main :
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(.TARGET) $(.ALLSRC:M*.o) $(LDLIBS)

console.o : console.c console.h framebuffer.h
framebuffer.o : framebuffer.c framebuffer.h
graphics.o : graphics.c console.h graphics.h
levels.o : levels.c levels.h
lv00.o : lv00.c
main.o : main.c console.h framebuffer.h graphics.h levels.h
