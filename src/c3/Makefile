OBJECT	=	reboot.o
BIN	=	reboot
ifndef CFLAGS
CFLAGS	:=	-g -O2 --std=c99 -D_XOPEN_SOURCE=600 -I../lib
endif
ifndef CC
CC	:=	gcc
endif

all	:	$(BIN)

$(BIN)	:	$(OBJECT)
	$(CC) $< -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@


.PHONY: clean
clean :
	-rm *.o $(BIN)


