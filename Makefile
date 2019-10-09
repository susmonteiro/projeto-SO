# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

CC   = gcc
LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../
LDFLAGS=-lm

#Flag adicional que muda conforme a implementacao
ADDICFLAGS=

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run

all: tecnicofs

tecnicofs-nosync: lib/bst.o fs.o main.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-nosync lib/bst.o fs.o main.o
	make clean_ofiles

tecnicofs-mutex: ADDICFLAGS = -DMUTEX 
tecnicofs-mutex: lib/bst.o fs.o main.o
	$(LD) $(CFLAGS) $(LDFLAGS) -lpthread -o tecnicofs-mutex lib/bst.o fs.o main.o
	make clean_ofiles

tecnicofs-rwlock: ADDICFLAGS = -DRWLOCK 
tecnicofs-rwlock: lib/bst.o fs.o main.o
	$(LD) $(CFLAGS) $(LDFLAGS) -lpthread -o tecnicofs-mutex lib/bst.o fs.o main.o
	make clean_ofiles


lib/bst.o: lib/bst.c lib/bst.h
	$(CC) $(CFLAGS) $(ADDICFLAGS) -o lib/bst.o -c lib/bst.c

fs.o: fs.c fs.h lib/bst.h
	$(CC) $(CFLAGS) $(ADDICFLAGS) -o fs.o -c fs.c

main.o: main.c fs.h lib/bst.h
	$(CC) $(CFLAGS) $(ADDICFLAGS) -o main.o -c main.c

clean_ofiles:
	@echo Cleaning Ofiles...
	rm -f lib/*.o *.o

clean:
	@echo Cleaning...
	rm -f lib/*.o *.o tecnicofs*

run: tecnicofs
	./tecnicofs-nosync in out 1
