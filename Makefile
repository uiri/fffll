CC=gcc
CFLAGS=-Wall -g -pedantic -o
# you may possible also want the following for profiling:
# -fprofile-arcs -ftest-coverage -pg

all:	fffll.l.c fffll.y.c list.o array.o
	$(CC) $(CFLAGS) fffll list.o array.o fffll.l.c fffll.y.c

fffll.l.c: fffll.l
	flex -o fffll.l.c fffll.l

fffll.y.c: fffll.y
	bison -d -o fffll.y.c fffll.y

list.o: list.c list.h
	$(CC) $(CFLAGS) list.o -c list.c

array.o: array.c array.h
	$(CC) $(CFLAGS) array.o -c array.c

clean:
	rm list.o
	rm explang.y.c
	rm explang.y.h
	rm explang.l.c
	rm explang
test:
	for TESTFILE in examples/*; do valgrind ./fffll $$TESTFILE; done;
	rm test
