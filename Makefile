CC=gcc
SYMBOLS?=_ARCHLINUX # Arch linux symbol; replace with empty definition to compile for other linux / macOS
CFLAGS=-std=c18 $(addprefix -D , ${SYMBOLS}) -Wall -Wextra -pedantic -Wfatal-errors
LDLIBS=-ledit -lm
VPATH=src/
OBJPATH=out/

SRCS=grammar.c builtin.c execute.c mpc.c lisper.c lvalue.c lenvironment.c mempool.c prgparams.c
OBJS=$(SRCS:%.c=${OBJPATH}%.o)
HDRS=$(wildcard ${VPATH}*.h)
TARGET?=lisper

ifeq (${DEBUG}, 1) # use DEBUG=1 to enable debug symbols to be compiled in
CFLAGS+=-g3 -gdwarf-2
SYMBOLS+=_DEBUG
endif

.PHONY: all clean debug

all: $(TARGET)

debug: clean
	DEBUG=1 $(MAKE) all

$(TARGET): $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(OBJS): $(OBJPATH)%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $<    -o $@

clean:
	$(RM) -r $(TARGET) $(OBJPATH)

