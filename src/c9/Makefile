ifndef CFLAGS
CFLAGS := -g --std=c99 -D_XOPEN_SOURCE=600 -D_BSD_SOURCE -I../lib
endif

ifndef CC
CC := cc
endif

SRCS = $(wildcard *.c)

PROGS = $(patsubst %.c,%,$(SRCS))
TLPI_LIB = ../../tlpi-dist/libtlpi.a

all: $(PROGS)

%: %.c 
	$(CC) $(CFLAGS) $< $(TLPI_LIB) -o $@ 

.PHONY: clean
clean:
	rm -f $(PROGS) test