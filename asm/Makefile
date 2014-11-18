all:	test.o numtostr.o std.o
	ld -o test test.o numtostr.o std.o

test.o: test.s
	nasm -f elf64 test.s

numtostr.o: numtostr.s
	nasm -f elf64 numtostr.s

std.o: std.s
	nasm -f elf64 std.s
