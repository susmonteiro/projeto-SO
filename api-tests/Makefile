CC=gcc
CFLAGS =-Wall -std=gnu99 -I../ 

all: a.out

a.out: ../tecnicofs-client-api.c client-api-test-create.c
     $(CC) -o a.out client-api-test-create.c ../tecnicofs-client-api.c


clean:
	rm a.out
