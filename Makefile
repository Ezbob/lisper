CC=gcc
SYMBOLS?=_ARCHLINUX
DEBUG?=
ifeq (${DEBUG}, true)
CFLAGS=-std=c99 -g $(addprefix -D , ${SYMBOLS}) -Wall -Wextra -pedantic
else
CFLAGS=-std=c99 -g $(addprefix -D , ${SYMBOLS}) -Wall -Wextra -pedantic
endif
LDLIBS=-ledit -lm
SRCS=grammar.c prompt.c mpc.c lisper.c lval.c
OBJS=$(patsubst %.c, %.o, $(SRCS))
TARGET?=lisper

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

clean:
	$(RM) $(TARGET) $(OBJS)

