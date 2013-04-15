#include "bootpack.h"
#include <stdio.h>
void main(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char mcursor[256], s[40];
	int mx, my, i;
	mx = (binfo->scrnx - 16) / 2; 
	my = (binfo->scrny - 28 - 16) / 2;
	init_gdtidt();
	init_pic();
	io_sti();
	
	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(mcursor, COL8_008484);
	sprintf(s, "(%d, %d)", mx, my);
	putfont8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	io_out8(PIC0_IMR, 0xf9);
	io_out8(PIC1_IMR, 0xef);

    for (;;) {
        io_cli();
        if (keybuf.flag == 0) {
            io_stihlt();
        } else {
            i = keybuf.data;
            keybuf.flag = 0;
            io_sti();
			sprintf(s, "0x%02x", i);
			boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 31, 31, 52);
			putfont8_asc(binfo->vram, binfo->scrnx, 0, 31, COL8_FFFFFF, s);
        }   
    }   
}
