CC=gcc
SYMBOLS?=-D _ARCHLINUX
CFLAGS=-std=c99 $(SYMBOLS) -Wall -Wextra -pedantic
LDLIBS=-ledit
SRCS=grammar.c prompt.c mpc.c lisper.c
OBJS=$(patsubst %.c, %.o, $(SRCS))
TARGET?=lisper

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

clean:
	$(RM) $(TARGET) $(OBJS)

