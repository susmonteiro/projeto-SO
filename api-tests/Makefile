CC=gcc
CFLAGS=-Wall -std=gnu99 -I../ 

test=client-api-test-delete.c

.PHONY: all clean

all: a.out

a.out: ../tecnicofs-client-api.c ../tecnicofs-api-constants.h ../tecnicofs-client-api.h $(test)
	$(CC) $(CFLAGS) $(test) ../tecnicofs-client-api.c


clean:
	rm a.out
