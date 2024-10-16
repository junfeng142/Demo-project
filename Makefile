# st - simple terminal
# See LICENSE file for copyright and license details.

ifeq ($(platform),miyoomini)
	include config_miyoomini.mk
else
	include config.mk
endif

SRC = st.c keyboard.c font.c
OBJ = ${SRC:.c=.o}

all: options st 

options:
	@echo st build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

config.h:
	cp -n config.def.h config.h

.c.o:
	@echo $(CC) $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.h

st: ${OBJ}
	@echo $(CC) -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f st ${OBJ}

.PHONY: all options clean
