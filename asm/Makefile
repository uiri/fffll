all:	test.o write.o std.o
	ld -s -o test test.o write.o std.o

test.o: test.s
	nasm -f elf64 test.s

write.o: write.s
	nasm -f elf64 write.s

std.o: std.s
	nasm -f elf64 std.s
