fwos
======================

**fwos** is an operating system based on OSASK developed by Hidemi Kawai. Its main perpose is for my study on OS. Its branches are related to the book "30 days write OS" also written by Hidemi Kawai.

Building and Installing
----------------------
Required tools: nasm, gcc, ld, dd, make

Layout of Img File
----------------------
	File format: img (FAT12)
	Layout:
	fat12 header----ipl.bin              asmhead.sys        bootpack.sys               end
    	 |____________|_____________________|__________________|________________________| 
     	0                                0x4200             0x4c00                    1440k 

Layout of Memory
----------------------
	0x00000000 -- 0x000fffff : Load IPL  (1MB)
	0x00100000 -- 0x00267fff : Store the the content of the floopy (1440KB)
	0x00268000 -- 0x0026f7ff : Empty (30KB) 
	0x0026f800 -- 0x0026ffff : IDT (2KB)
	0x00270000 -- 0x0027ffff : GDT (64KB)
	0x00280000 -- 0x002fffff : bootpack.sys (512KB)
	0x00300000 -- 0x003fffff : Stack and other (1MB)
	0x00400000 --            : Empty
