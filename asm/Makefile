#test.o: test.s
#	nasm -f elf64 test.s

#numtostr.o: numtostr.s
#	nasm -f elf64 numtostr.s

#std.o: std.s
#	nasm -f elf64 std.s


#std.o: std.s
#	nasm -f elf64 std.s

#list.o: list.s
#	nasm -f elf64 list.s

#all:	test.o numtostr.o std.o
#	ld -o test test.o numtostr.o std.o
#cat test.s | gcc -x assembler -nostdlib -o test std.s numtostr.s list.s -
all:	test.s std.s numtostr.s list.s
	gcc -g -nostdlib -o test test.s std.s numtostr.s list.s
