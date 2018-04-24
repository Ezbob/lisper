CC=gcc
SYMBOLS?=_ARCHLINUX
CFLAGS=-std=c99 $(addprefix -D , ${SYMBOLS}) -Wall -Wextra -pedantic
ifeq (${DEBUG}, 1) # use DEBUG=1 to enable debug symbols to be compiled in
CFLAGS+=-g
endif
LDLIBS=-ledit -lm
VPATH=src/
OBJPATH=obj/

SRCS=grammar.c builtin.c prompt.c mpc.c lisper.c lval.c
OBJS=$(SRCS:%.c=${OBJPATH}%.o)
HDRS=$(wildcard ${VPATH}*.h)
TARGET?=lisper

all: $(TARGET)

$(TARGET): $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(OBJS): $(OBJPATH)%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $<    -o $@

clean:
	$(RM) -r $(TARGET) $(OBJPATH)

