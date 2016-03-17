#!/bin/bash

./fffll examples/$1.ff >target/$1.s && gcc -x assembler -gstab -nostdlib -o target/$1 target/$1.s asm/std.s asm/numtostr.s asm/list.s && ./target/$1
