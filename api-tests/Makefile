CC=gcc
CFLAGS=-Wall -std=gnu99 -I../ 

.PHONY: all clean

all: a.out

a.out: ../tecnicofs-client-api.c ../tecnicofs-api-constants.h ../tecnicofs-client-api.h client-api-test-create.c
	$(CC) $(CFLAGS) client-api-test-create.c ../tecnicofs-client-api.c


clean:
	rm a.out
