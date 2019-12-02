# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

SOURCES = main.c fs.c sync.c erro.c socket.c tecnicofs-server-api.c
SOURCES+= lib/bst.c lib/hash.c lib/inodes.c
SOURCES+= ../common/erro.c
# OBJS_NOSYNC = $(SOURCES:%.c=%.o)
# OBJS_MUTEX  = $(SOURCES:%.c=%-mutex.o)
OBJS_RWLOCK = $(SOURCES:%.c=%-rwlock.o)
#OBJS = $(OBJS_NOSYNC) $(OBJS_MUTEX) $(OBJS_RWLOCK)
OBJS = $(OBJS_RWLOCK)
CC   = gcc
LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../ -g
LDFLAGS=-lm -pthread
TARGETS = tecnicofs-rwlock

.PHONY: all clean

all: $(TARGETS)

$(TARGETS):
	$(LD) $(CFLAGS) $^ -o $@ $(LDFLAGS)


### no sync ###
lib/bst.o: lib/bst.c lib/bst.h
fs.o: fs.c fs.h lib/bst.h lib/hash.h
sync.o: sync.c sync.h 
main.o: main.c fs.h lib/bst.h lib/hash.h sync.h

lib/hash.o: lib/hash.c lib/hash.h
erro.o: ../common/erro.c ../common/erro.h
socket.o: socket.c socket.h
tecnicofs-server-api.o: tecnicofs-server-api.c tecnicofs-server-api.h 

tecnicofs-nosync: lib/bst.o fs.o sync.o main.o lib/hash.o socket.o tecnicofs-server-api.o erro.o


### MUTEX ###
lib/bst-mutex.o: CFLAGS+=-DMUTEX
lib/bst-mutex.o: lib/bst.c lib/bst.h

fs-mutex.o: CFLAGS+=-DMUTEX
fs-mutex.o: fs.c fs.h lib/bst.h lib/hash.h

sync-mutex.o: CFLAGS+=-DMUTEX
sync-mutex.o: sync.c sync.h 

lib/hash-mutex.o: lib/hash.c lib/hash.h

erro-mutex.o: ../common/erro.c ../common/erro.h

socket-mutex.o: socket.c socket.h

tecnicofs-server-api-mutex.o: tecnicofs-server-api.c tecnicofs-server-api.h 

main-mutex.o: CFLAGS+=-DMUTEX
main-mutex.o: main.c fs.h lib/bst.h sync.h lib/hash.h erro.h socket.h tecnicofs-server-api.h
tecnicofs-mutex: lib/bst-mutex.o fs-mutex.o sync-mutex.o main-mutex.o lib/hash.o erro-mutex.o socket-mutex.o tecnicofs-server-api-mutex.o

### RWLOCK ###
lib/bst-rwlock.o: CFLAGS+=-DRWLOCK
lib/bst-rwlock.o: lib/bst.c lib/bst.h

fs-rwlock.o: CFLAGS+=-DRWLOCK
fs-rwlock.o: fs.c fs.h lib/bst.h lib/hash.h

sync-rwlock.o: CFLAGS+=-DRWLOCK
sync-rwlock.o: sync.c sync.h 

lib/hash-rwlock.o: lib/hash.c lib/hash.h

lib/inodes-rwlock.o: lib/inodes.c lib/inodes.h

erro-rwlock.o: ../common/erro.c ../common/erro.h

socket-rwlock.o: socket.c socket.h

tecnicofs-server-api-rwlock.o: CFLAGS+=-DRWLOCK
tecnicofs-server-api-rwlock.o: tecnicofs-server-api.c tecnicofs-server-api.h

main-rwlock.o: CFLAGS+=-DRWLOCK
main-rwlock.o: main.c main.h fs.h lib/bst.h lib/hash.h lib/inodes.h sync.h ../common/erro.h socket.h tecnicofs-server-api.h
tecnicofs-rwlock: lib/bst-rwlock.o fs-rwlock.o sync-rwlock.o lib/inodes-rwlock.o main-rwlock.o lib/hash.o erro-rwlock.o socket-rwlock.o tecnicofs-server-api-rwlock.o


%.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo Cleaning...
	rm -f $(OBJS) $(TARGETS)
