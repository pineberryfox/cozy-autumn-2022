CFLAGS+= -DSTBI_ONLY_PNG
CFLAGS+= -DSTBI_MAX_DIMENSIONS=512
CFLAGS+= -I/Library/Frameworks/SDL2.framework/Headers
CFLAGS+= -I$(.CURDIR)/stb
CFLAGS+= -mmacosx-version-min=10.7 -arch arm64 -arch x86_64
CFLAGS+= -Wall
LDFLAGS+= -F/Library/Frameworks
LDLIBS+= -framework SDL2
LDLIBS+= -framework Cocoa

main : main.o framebuffer.o console.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(.TARGET) $(.ALLSRC:M*.o) $(LDLIBS)
