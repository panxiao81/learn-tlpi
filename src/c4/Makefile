ifndef CFLAGS
CFLAGS := -g -O2 --std=c99 -D_XOPEN_SOURCE=600 -I../lib
endif

ifndef CC
CC := cc
endif

BIN = tee cp_hole
OBJECT := tee.o cp_hole.o
TLPI_LIB = ../../tlpi-dist/libtlpi.a

all:$(BIN)

$(BIN):$(OBJECT) $(TLPI_LIB) 
	$(CC) $< $(TLPI_LIB) -o $@ 

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	-rm *.o $(BIN)