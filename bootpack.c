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
	io_sti(); //IDT／PIC 的初始化完成，于是开放CPU中断
	
	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(mcursor, COL8_008484);
	sprintf(s, "(%d, %d)", mx, my);
	putfont8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	io_out8(PIC0_IMR, 0xf9); // 开放 PIC1和键盘终端 11111001
	io_out8(PIC1_IMR, 0xef); // 开放鼠标中断 11101111
	
	init_keyboard();
	enable_mouse(&mdec);

    for (;;) {
        io_cli();
		if (fifo8_status(&keyfifo) != 0)
		{
			i = fifo8_get(&keyfifo);
			io_sti();
			sprintf(s, "%02x", i);
			boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
			putfont8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
		}
		else if (fifo8_status(&mousefifo) !=0)
		{
			i = fifo8_get(&mousefifo);
			io_sti();
			if (mouse_decode(&mdec, i) > 0)
			{
				//三字节到齐，显示
				sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
				if ((mdec.btn & 0x01) != 0)
					s[1] = 'L';
				if ((mdec.btn & 0x02) != 0)
					s[3] = 'R';
				if ((mdec.btn & 0x04) != 0)
					s[2] = 'C';
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32+8*15-1, 31);
				putfont8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
				//鼠标移动
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx+15, my+15);
				mx += mdec.x;
				my += mdec.y;
				if (mx < 0) mx = 0;
				if (my < 0) my = 0;
				if (mx > binfo->scrnx - 16) mx = binfo->scrnx - 16;
				if (my > binfo->scrny - 16) my = binfo->scrny - 16;
				sprintf(s, "(%3d, %3d)", mx, my);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
				putfont8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
				putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
			}
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

void enable_mouse(struct MOUSE_DEC *mdec)
{
	//激活鼠标
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);//顺利的话，键盘控制其返回ACK(0xfa)
	mdec->phase = 0; //等待0xfa
	return; 
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
	if (mdec->phase == 0)
	{ //等待鼠标0xfa状态
		if (dat == 0xfa) 
			mdec->phase = 1;
		return 0;
	}
	if (mdec->phase == 1)
	{ //等待鼠标输入第一字节
		if ((dat & 0xc8) == 0x08)
		{
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}
		return 0;
	}
	if (mdec->phase == 2)
	{ //等待鼠标输入的第二字节
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if (mdec->phase == 3)
	{ //等待鼠标输入的第三字节
		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if ((mdec->buf[0] & 0x10) != 0)
			mdec->x |= 0xffffff00;
		if ((mdec->buf[0] & 0x20) != 0)
			mdec->y |= 0xffffff00;
		mdec->y = - mdec->y;
		return 1;
	}
	return -1;
}
