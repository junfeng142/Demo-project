
CC=/opt/mmiyoo/usr/bin/arm-linux-gnueabihf-gcc
SDL_CFLAGS = `/opt/mmiyoo/usr/arm-buildroot-linux-gnueabihf/sysroot/usr/bin/sdl-config --cflags`
SDL_LIBS = `/opt/mmiyoo/usr/arm-buildroot-linux-gnueabihf/sysroot/usr/bin/sdl-config --libs`
CFLAGS= -DDINGUX -march=armv7-a $(SDL_CFLAGS)
OBJS=	src/client.o src/common.o src/vm.o  src/sprites.o src/decode.o  src/animation.o \
	src/rooms.o  src/render.o src/main.o    src/music.o  src/debug.o  src/lzss.o  \
	src/sound.o  src/screen.o src/scale2x.o src/scale3x.o src/game2bin.o src/cd_iso.o      \
	src/scale800x480.o

LIBS=$(SDL_LIBS) -lSDL -lSDLmain -lSDL_mixer

hota: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -r $(OBJS)
