#include "bootpack.h"
#include <stdio.h>
void main(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char mcursor[256], s[40], keybuf[32], mousebuf[128];
	int mx, my, i, j;
	mx = (binfo->scrnx - 16) / 2; 
	my = (binfo->scrny - 28 - 16) / 2;
	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 32, mousebuf);
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
	
	init_keyboard();
	enable_mouse();

    for (;;) {
        io_cli();
		if (fifo8_status(&keyfifo) != 0)
		{
			i = fifo8_get(&keyfifo);
			io_sti();
			sprintf(s, "%02x", i);
			boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 31, 31, 52);
			putfont8_asc(binfo->vram, binfo->scrnx, 0, 31, COL8_FFFFFF, s);
		}
		else if (fifo8_status(&mousefifo) !=0)
		{
			i = fifo8_get(&mousefifo);
			io_sti();
			sprintf(s, "%02x", i);
			boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 31, 63, 52);
			putfont8_asc(binfo->vram, binfo->scrnx, 32, 31, COL8_FFFFFF, s);
		}
		else
            io_stihlt();
    }   
}

void wait_KBC_sendready(void)
{
	while(1)
	{
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) break;
	}
	return;
}

void init_keyboard(void)
{
	//初始话键盘控制电路
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}

void enable_mouse(void)
{
	//激活鼠标
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	return; //顺利的话，键盘控制其返回ACK(0xfa)
}
