CC=gcc
SYMBOLS?=_ARCHLINUX
CFLAGS=-std=c99 $(addprefix -D , ${SYMBOLS}) -Wall -Wextra -pedantic
LDLIBS=-ledit -lm
SRCS=grammar.c prompt.c mpc.c lisper.c eval.c
OBJS=$(patsubst %.c, %.o, $(SRCS))
TARGET?=lisper

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

clean:
	$(RM) $(TARGET) $(OBJS)

