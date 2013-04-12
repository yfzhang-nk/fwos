NASM = nasm
GCC = gcc 
MAKE = make
CAT = cat
LD = ld
DD = dd
DEL = rm
OBJCOPY = objcopy

default:
	$(MAKE) myos 

ipl.bin: ipl.asm 
	$(NASM) ipl.asm -o ipl.bin	

bootpack.o: bootpack.c hankaku.h
	$(GCC) -m32 -c -o bootpack.o bootpack.c

nasmfunc.o: nasmfunc.asm
	$(NASM) -felf -o nasmfunc.o nasmfunc.asm
	
bootpack.elf: bootpack.o nasmfunc.o
	$(LD) -T bootpack.lds -melf_i386 -o bootpack.elf bootpack.o nasmfunc.o 

bootpack.sys: bootpack.elf
	$(OBJCOPY) -Obinary bootpack.elf bootpack.sys

asmhead.sys: asmhead.asm 
	$(NASM) asmhead.asm -o asmhead.sys 

myos: ipl.bin asmhead.sys bootpack.sys
	$(DD) if=ipl.bin of=../myos.img bs=512 
	$(DD) if=asmhead.sys of=../myos.img bs=512 seek=33
	$(DD) if=bootpack.sys of=../myos.img bs=512 seek=38
	$(DD) if=/dev/zero of=../myos.img bs=512 seek=2880 count=0

clean:
	$(DEL) ipl.bin asmhead.sys bootpack.o bootpack.elf bootpack.sys nasmfunc.o
