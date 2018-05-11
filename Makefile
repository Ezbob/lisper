CC=gcc
SYMBOLS?=_ARCHLINUX # Arch linux symbol; replace with empty definition to compile for other linux / macOS
CFLAGS=-std=c99 $(addprefix -D , ${SYMBOLS}) -Wall -Wextra -pedantic
LDLIBS=-ledit -lm
VPATH=src/
OBJPATH=obj/

SRCS=grammar.c builtin.c exec.c mpc.c lisper.c lval.c lenv.c hashmap.c
OBJS=$(SRCS:%.c=${OBJPATH}%.o)
HDRS=$(wildcard ${VPATH}*.h)
TARGET?=lisper

ifeq (${DEBUG}, 1) # use DEBUG=1 to enable debug symbols to be compiled in
CFLAGS+=-g
SYMBOLS+=_DEBUG
endif

.PHONY: all clean debug

all: $(TARGET)

debug:
	DEBUG=1 $(MAKE) all

$(TARGET): $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(OBJS): $(OBJPATH)%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $<    -o $@

clean:
	$(RM) -r $(TARGET) $(OBJPATH)

