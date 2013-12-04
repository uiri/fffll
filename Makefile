CC=gcc
CFLAGS=-Wall -g -pedantic
LIBS=-L. -lm -llist
# you may possible also want the following for profiling:
# -fprofile-arcs -ftest-coverage -pg

all:	fffll.l.c fffll.y.c tree.o
	if ! [ -f liblist.so ]; then make liblist.so; fi;
	$(CC) $(LIBS) $(CFLAGS) -o fffll tree.o fffll.l.c fffll.y.c

fffll.l.c: fffll.l
	flex -o fffll.l.c fffll.l

fffll.y.c: fffll.y
	bison -d -o fffll.y.c fffll.y

liblist.so: list.o array.o
	$(CC) -shared $(CFLAGS) -o liblist.so list.o array.o

list.o: list.c list.h
	$(CC) -fPIC $(CFLAGS) -c list.c

array.o: array.c array.h
	$(CC) -fPIC $(CFLAGS) -c array.c

tree.o: tree.c tree.h
	$(CC) -fPIC $(CFLAGS) -c tree.c

clean:
	rm list.so
	rm list.o
	rm tree.o
	rm array.o
	rm fffll.y.c
	rm fffll.y.h
	rm fffll.l.c
	rm fffll
test:
	for TESTFILE in examples/*; do valgrind ./fffll $$TESTFILE; done;
	rm test
