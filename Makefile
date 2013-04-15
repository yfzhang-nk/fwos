NASM = nasm
GCC = gcc 
MAKE = make
CAT = cat
LD = ld
DD = dd
DEL = rm
OBJCOPY = objcopy

MIDOBJ = graphic.o dsctbl.o int.o libc_required.o

default:
	$(MAKE) myos 

ipl.bin: ipl.asm 
	$(NASM) ipl.asm -o ipl.bin	

libc_required.o: libc_required.c 
	$(GCC) -m32 -c -o libc_required.o libc_required.c
int.o: int.c bootpack.h
	$(GCC) -m32 -c -o int.o int.c
dsctbl.o: dsctbl.c
	$(GCC) -m32 -c -o dsctbl.o dsctbl.c
graphic.o: graphic.c hankaku.h color.h
	$(GCC) -m32 -c -o graphic.o graphic.c
bootpack.o: bootpack.c color.h bootpack.h
	$(GCC) -m32 -c -o bootpack.o bootpack.c

nasmfunc.o: nasmfunc.asm
	$(NASM) -felf -o nasmfunc.o nasmfunc.asm
	
bootpack.elf: bootpack.o nasmfunc.o $(MIDOBJ) libc.a
	$(LD) -T bootpack.lds -melf_i386 -o bootpack.elf bootpack.o $(MIDOBJ) nasmfunc.o libc.a

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
	$(DEL) ipl.bin asmhead.sys bootpack.o bootpack.elf bootpack.sys nasmfunc.o $(MIDOBJ)
