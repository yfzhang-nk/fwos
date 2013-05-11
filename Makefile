NASM = nasm
GCC = gcc 
MAKE = make
CAT = cat
LD = ld
DD = dd
DEL = rm
CP = cp
MOUNT = mount
OBJCOPY = objcopy

MIDOBJ = graphic.o dsctbl.o int.o libc_required.o fifo.o keyboard.o mouse.o memory.o sheet.o timer.o mtask.o window.o console.o file.o

default:
	$(MAKE) fwos

ipl.bin: ipl.asm 
	$(NASM) ipl.asm -o ipl.bin	

%.o: %.c
	$(GCC) -m32 -fno-stack-protector -c -o $@ $<
nasmfunc.o: nasmfunc.asm
	$(NASM) -felf -o nasmfunc.o nasmfunc.asm
	
bootpack.elf: bootpack.o nasmfunc.o $(MIDOBJ) libc.a
	$(LD) -T bootpack.lds -melf_i386 -o bootpack.elf bootpack.o $(MIDOBJ) nasmfunc.o libc.a

bootpack.sys: bootpack.elf
	$(OBJCOPY) -Obinary bootpack.elf bootpack.sys

APPOBJ = a.o a_nasm.o a.elf a.bin crack.bin
a_nasm.o: a_nasm.asm
	$(NASM) -felf -o a_nasm.o a_nasm.asm
a.elf: a.o a_nasm.o
	$(LD) -T a.lds -melf_i386 -o a.elf a.o a_nasm.o
a.bin: a.elf
	$(OBJCOPY) -Obinary a.elf a.bin
crack.bin: crack.asm
	$(NASM) -o crack.bin crack.asm

asmhead.sys: asmhead.asm 
	$(NASM) asmhead.asm -o asmhead.sys

fwos: ipl.bin asmhead.sys bootpack.sys a.bin crack.bin
	$(DEL) -f fwos.img
	$(DD) if=ipl.bin of=fwos.img bs=512 
	$(DD) if=/dev/zero of=fwos.img bs=512 seek=2880 count=0
	sudo $(MOUNT) -o loop fwos.img floopy/
	sudo $(CP) asmhead.sys floopy/
	sudo $(CP) bootpack.sys floopy/
	sudo $(CP) a.bin floopy/
	sudo $(CP) crack.bin floopy/
	sleep 2
	sudo umount floopy/
	#$(DD) if=bootpack.sys of=fwos.img bs=512 seek=38
	#$(DD) if=/dev/zero of=fwos.img bs=512 seek=2880 count=0

clean:
	$(DEL) ipl.bin asmhead.sys bootpack.o bootpack.elf bootpack.sys nasmfunc.o $(MIDOBJ) fwos.img $(APPOBJ)
