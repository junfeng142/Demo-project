# st version
VERSION = 0.3

# Customize below to fit your system

# paths
CROSS_COMPILE ?= /opt/FunKey-sdk-2.3.0/bin/arm-linux-
CC = ${CROSS_COMPILE}gcc
SYSROOT	?= $(shell ${CC} --print-sysroot)

# includes and libs
INCS = -I. -I${SYSROOT}/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
LIBS = -lc -L${SYSROOT}/usr/lib -lSDL -lpthread -Wl,-Bstatic,-lutil,-Bdynamic

# flags
CPPFLAGS = -DVERSION=\"${VERSION}\" -fPIC
CFLAGS = -Os -marm -mtune=cortex-a7 -march=armv7ve+simd -mfpu=neon-vfpv4 -mfloat-abi=hard
CFLAGS += ${INCS} ${CPPFLAGS} -Wall -ffunction-sections -fdata-sections -std=gnu99 
LDFLAGS += ${LIBS} -Wl,--gc-sections -frerun-loop-opt -funroll-loops -ffast-math -fexpensive-optimizations -fomit-frame-pointer -fno-strict-aliasing
