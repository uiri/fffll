CC=gcc
CFLAGS=-Wall -g -pedantic
LIBS=-L. -lm -lffflllist -lfffllstd
# you may possible also want the following for profiling:
# -fprofile-arcs -ftest-coverage -pg

all:
	make libffflllist.so
	make libfffllstd.so
	make fffll

fffll:	fffll.l.c fffll.y.c value.c tree.o
	$(CC) $(LIBS) $(CFLAGS) -o fffll tree.o value.c fffll.l.c fffll.y.c

fffll.l.c: fffll.l
	flex -o fffll.l.c fffll.l

fffll.y.c: fffll.y
	bison -d -o fffll.y.c fffll.y

libffflllist.so: list.o array.o
	$(CC) -shared $(CFLAGS) -o libffflllist.so list.o array.o

libfffllstd.so: builtin.o evaluate.o
	$(CC) -shared $(CFLAGS) -o libfffllstd.so builtin.o evaluate.o

array.o: array.c array.h
	$(CC) -fPIC $(CFLAGS) -c array.c

builtin.o: builtin.c builtin.h
	$(CC) -fPIC $(CFLAGS) -c builtin.c

evaluate.o: evaluate.c evaluate.h
	$(CC) -fPIC $(CFLAGS) -c evaluate.c

list.o: list.c list.h
	$(CC) -fPIC $(CFLAGS) -c list.c

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
