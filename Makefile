NASM = nasm
GCC = gcc 
MAKE = make
CAT = cat
LD = ld
DD = dd
DEL = rm
OBJCOPY = objcopy

MIDOBJ = graphic.o dsctbl.o int.o libc_required.o fifo.o keyboard.o mouse.o memory.o sheet.o timer.o mtask.o

default:
	$(MAKE) fwos

ipl.bin: ipl.asm 
	$(NASM) ipl.asm -o ipl.bin	

%.o: %.c
	$(GCC) -m32 -c -o $@ $<
nasmfunc.o: nasmfunc.asm
	$(NASM) -felf -o nasmfunc.o nasmfunc.asm
	
bootpack.elf: bootpack.o nasmfunc.o $(MIDOBJ) libc.a
	$(LD) -T bootpack.lds -melf_i386 -o bootpack.elf bootpack.o $(MIDOBJ) nasmfunc.o libc.a

bootpack.sys: bootpack.elf
	$(OBJCOPY) -Obinary bootpack.elf bootpack.sys

asmhead.sys: asmhead.asm 
	$(NASM) asmhead.asm -o asmhead.sys 

fwos: ipl.bin asmhead.sys bootpack.sys 
	$(DD) if=ipl.bin of=fwos.img bs=512 
	$(DD) if=asmhead.sys of=fwos.img bs=512 seek=8
	$(DD) if=bootpack.sys of=fwos.img bs=512 seek=16
	#$(DD) if=asmhead.sys of=fwos.img bs=512 seek=33
	#$(DD) if=bootpack.sys of=fwos.img bs=512 seek=38
	$(DD) if=/dev/zero of=fwos.img bs=512 seek=2880 count=0

clean:
	$(DEL) ipl.bin asmhead.sys bootpack.o bootpack.elf bootpack.sys nasmfunc.o $(MIDOBJ) fwos.img
