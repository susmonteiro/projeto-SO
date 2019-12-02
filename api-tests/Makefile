CC=gcc
CFLAGS=-Wall -std=gnu99 -I../ 

test=client-api-test-read.c

.PHONY: all clean

all: a.out

a.out: ../client/tecnicofs-client-api.c  ../common/constants.h ../common/erro.c ../common/tecnicofs-api-constants.h ../client/tecnicofs-client-api.h $(test)
	$(CC) $(CFLAGS) $(test) ../client/tecnicofs-client-api.c ../common/erro.c


clean:
	rm a.out
