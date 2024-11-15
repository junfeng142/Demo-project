# credits @GXB

TARGET  = gngeo
CHAINPREFIX     ?= /opt/FunKey-sdk-2.3.0
CROSS_COMPILE   ?= /opt/FunKey-sdk-2.3.0/bin/arm-funkey-linux-gnueabihf-
CC      = $(CROSS_COMPILE)gcc
CXX     = $(CROSS_COMPILE)g++
LD      = $(CROSS_COMPILE)gcc
STRIP   = $(CROSS_COMPILE)strip
PKG_CONFIG ?= $(CHAINPREFIX)/usr/bin/pkg-config
PKGS = sdl SDL_image zlib
PKGS_CFLAGS = $(shell $(PKG_CONFIG) --cflags $(PKGS))
PKGS_LIBS = $(shell $(PKG_CONFIG) --libs $(PKGS))
CFLAGS = $(PKGS_CFLAGS)
#CFLAGS += -ggdb
CFLAGS += -D_GNU_SOURCE=1 -D_REENTRANT -DARM
CFLAGS += -O3 -fstrength-reduce -frerun-loop-opt -funroll-loops -ffast-math -fexpensive-optimizations -fomit-frame-pointer -fno-strict-aliasing
LDFLAGS = $(PKGS_LIBS)
LDFLAGS += -lm
OBJS    = \
  src/messages.o \
  src/drv.o \
  src/conf.o \
  src/mame_layer.o \
  src/cyclone.o \
  src/neocrypt.o \
  src/drz80.o \
  src/state.o \
  src/frame_skip.o \
  src/sound.o \
  src/video.o \
  src/memory.o \
  src/ym2610.o \
  src/neoboot.o \
  src/list.o \
  src/cyclone_interf.o \
  src/pd4990a.o \
  src/ym2610_interf.o \
  src/menu.o \
  src/gnutil.o \
  src/unzip.o \
  src/drz80_interf.o \
  src/screen.o \
  src/interp.o \
  src/emu.o \
  src/event.o \
  src/main.o \
  src/video_arm.o \
  src/roms.o \
  src/timer.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(OBJS) -o $@ $(LDFLAGS)
	$(STRIP) $(TARGET)

%.o: %.c 
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s 
	$(CXX) $(ASFLAGS) $(CXXFLAGS) -c $< -o $@

%.o: %.S 
	$(CXX) $(ASFLAGS) $(CXXFLAGS) -c $< -o $@

release: all
	@mkdir -p assets
	cp icon.png assets/
	cp miyoo-sample_gngeorc assets/
	cp aliases.txt assets/
	cp gngeo.dat/gngeo_data.zip assets/
	@mkdir -p dist
	cp -r assets/* dist/
	install -m0755 -D $(TARGET) dist/

ipk: release
	gm2xpkg -c -i miyoo-pkg.cfg

clean:
	rm -rf $(OBJS) $(TARGET)
