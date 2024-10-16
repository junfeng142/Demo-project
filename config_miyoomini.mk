# st version
VERSION = 0.3

# Customize below to fit your system

# paths
CROSS_COMPILE ?= /opt/mmiyoo/bin/arm-linux-gnueabihf-
CC = ${CROSS_COMPILE}gcc
SYSROOT	?= $(shell ${CC} --print-sysroot)

# includes and libs
INCS = -I. -I${SYSROOT}/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
LIBS = -lc -L${SYSROOT}/usr/lib -lSDL -lpthread -lmi_sys -lmi_gfx -lmi_ao -lmi_common -Wl,-Bstatic,-lutil,-Bdynamic

# flags
CPPFLAGS = -DVERSION=\"${VERSION}\" -DMIYOOMINI -fPIC
CFLAGS = -Os -marm -mtune=cortex-a7 -march=armv7ve+simd -mfpu=neon-vfpv4 -mfloat-abi=hard
CFLAGS += ${INCS} ${CPPFLAGS} -Wall -ffunction-sections -fdata-sections -std=gnu99 
LDFLAGS += ${LIBS} -Wl,--gc-sections -s
