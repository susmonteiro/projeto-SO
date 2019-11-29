CC=gcc
CFLAGS=-Wall -std=gnu99 -I../ 

test=client-api-test-read.c

.PHONY: all clean

all: a.out

a.out: ../tecnicofs-client-api.c  ../../constants.h ../../tecnicofs-api-constants.h ../tecnicofs-client-api.h $(test)
	$(CC) $(CFLAGS) $(test) ../tecnicofs-client-api.c


clean:
	rm a.out
