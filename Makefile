CC=gcc
CFLAGS=-Wall -g -lm -pedantic -o
# you may possible also want the following for profiling:
# -fprofile-arcs -ftest-coverage -pg

all:	fffll.l.c fffll.y.c list.o array.o tree.o
	$(CC) $(CFLAGS) fffll list.o array.o tree.o fffll.l.c fffll.y.c

fffll.l.c: fffll.l
	flex -o fffll.l.c fffll.l

fffll.y.c: fffll.y
	bison -d -o fffll.y.c fffll.y

list.o: list.c list.h
	$(CC) $(CFLAGS) list.o -c list.c

array.o: array.c array.h
	$(CC) $(CFLAGS) array.o -c array.c

tree.o: tree.c tree.h
	$(CC) $(CFLAGS) tree.o -c tree.c

clean:
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
