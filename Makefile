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

bootpack.o: bootpack.c
	$(GCC) -m32 -c -o bootpack.o bootpack.c
asmhead.o: asmhead.asm 
	$(NASM) asmhead.asm -o asmhead.o -felf32
asmhead.elf: asmhead.o bootpack.o
	$(LD) --verbose -Tasmhead.lds -M -melf_i386 -o asmhead.elf asmhead.o bootpack.o
asmhead.sys: asmhead.elf
	$(OBJCOPY) -O binary asmhead.elf asmhead.sys
	

myos: ipl.bin asmhead.sys
	$(DD) if=ipl.bin of=../myos.img bs=512 
	$(DD) if=asmhead.sys of=../myos.img bs=512 seek=33
	$(DD) if=/dev/zero of=../myos.img bs=512 seek=2880 count=0

clean:
	$(DEL) ipl.bin asmhead.sys bootpack.o asmhead.o asmhead.elf
