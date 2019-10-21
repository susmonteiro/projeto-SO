# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

CC   = gcc
LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../
LDFLAGS=-lm -lpthread


#Flag adicional que muda conforme a implementacao
ADDICFLAGS=

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run

all: 
	make tecnicofs-nosync 
	make tecnicofs-mutex 
	make tecnicofs-rwlock 

#tecnicofs-nosync: ADDICFLAGS = 	(REDUNDANTE)
tecnicofs-nosync: clean_ofiles lib/bst.o lib/hash.o fs.o locks.o main.o
	$(LD) $(CFLAGS) -o tecnicofs-nosync lib/bst.o lib/hash.o fs.o locks.o main.o $(LDFLAGS) 

tecnicofs-mutex: ADDICFLAGS = -DMUTEX 
tecnicofs-mutex: clean_ofiles lib/bst.o lib/hash.o fs.o locks.o main.o
	$(LD) $(CFLAGS) -o tecnicofs-mutex lib/bst.o lib/hash.o fs.o locks.o main.o $(LDFLAGS) 

tecnicofs-rwlock: ADDICFLAGS = -DRWLOCK 
tecnicofs-rwlock: clean_ofiles lib/bst.o lib/hash.o fs.o locks.o main.o
	$(LD) $(CFLAGS) -o tecnicofs-rwlock lib/bst.o lib/hash.o fs.o locks.o main.o $(LDFLAGS)  

lib/bst.o: lib/bst.c lib/bst.h
	$(CC) $(CFLAGS) $(ADDICFLAGS) -o lib/bst.o -c lib/bst.c

lib/hash.o: lib/hash.c lib/hash.h
	$(CC) $(CFLAGS) $(ADDICFLAGS) -o lib/hash.o -c lib/hash.c

fs.o: fs.c fs.h lib/bst.h
	$(CC) $(CFLAGS) $(ADDICFLAGS) -o fs.o -c fs.c

locks.o: locks.c locks.h fs.h 
	$(CC) $(CFLAGS) $(ADDICFLAGS) -o locks.o -c locks.c

main.o: main.c fs.h lib/bst.h
	$(CC) $(CFLAGS) $(ADDICFLAGS) -o main.o -c main.c


clean_ofiles:
	@echo Cleaning Ofiles...
	rm -f lib/*.o *.o

clean:
	@echo Cleaning...
	rm -f lib/*.o *.o tecnicofs* out*

run: 
	make
	./tecnicofs-nosync inputs/test4.txt out1 1 10
	./tecnicofs-mutex inputs/test4.txt out2 2 10
	./tecnicofs-rwlock inputs/test4.txt out3 3 10
