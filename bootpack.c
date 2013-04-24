#include "bootpack.h"
#include <stdio.h>
void main(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40];
	int mx, my, i, count = 0;
	int fifobuf[128];
	unsigned int memtotal;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHTCTL *shtctl;
	struct SHEET *sht_back, *sht_mouse, *sht_win;
	unsigned char *buf_back, buf_mouse[256], *buf_win;
	struct TIMER *timer, *timer2, *timer3;
	struct FIFO32 fifo;

	mx = (binfo->scrnx - 16) / 2; 
	my = (binfo->scrny - 28 - 16) / 2;
	init_gdtidt();
	init_pic();
	io_sti(); //IDT／PIC 的初始化完成，于是开放CPU中断

	fifo32_init(&fifo, 128, fifobuf);

	init_pit();

	io_out8(PIC0_IMR, 0xf8); // 开放 PIC1, 键盘终端, 计时器 11111000
	io_out8(PIC1_IMR, 0xef); // 开放鼠标中断 11101111

	timer = timer_alloc();
	timer_init(timer, &fifo, 10);
	timer_settime(timer, 1000);
	timer2 = timer_alloc();
	timer_init(timer2, &fifo, 3);
	timer_settime(timer2, 300);
	timer3 = timer_alloc();
	timer_init(timer3, &fifo, 1);
	timer_settime(timer3, 50);

	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);

	// 初始化内存管理
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	//memman_free(memman, 0x00030000, 0x0009e000); 
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	//初始化图层
	init_palette();
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	sht_back = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	sht_win = sheet_alloc(shtctl);
	buf_back = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	buf_win = (unsigned char *) memman_alloc_4k(memman, 160 * 52);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	sheet_setbuf(sht_win, buf_win, 160, 52, -1);
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(buf_mouse, 99);
	make_window8(buf_win, 160, 52, "counter");
	sheet_slide(sht_back, 0, 0);
	sheet_slide(sht_win, 80, 72);
	sheet_slide(sht_mouse, mx, my);
	sheet_updown(sht_back, 0);
	sheet_updown(sht_win, 1);
	sheet_updown(sht_mouse, 2);
	sprintf(s, "(%d, %d)", mx, my);
	putfont8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

	sprintf(s, "memory %dMB   free %dKB", memtotal/(1024*1024), memman_total(memman)/1024);
	putfont8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
	sheet_refresh(sht_back, 0, 0, binfo->scrnx, 48);
    for (;;) 
	{
		count++;
		//sprintf(s, "%010d", timerctl.count);
		//boxfill8(buf_win, 160, COL8_C6C6C6, 40, 28, 119, 43);
		//putfont8_asc(buf_win, 160, 40, 28, COL8_000000, s);
		//sheet_refresh(sht_win, 40, 28, 120, 44);

        io_cli();
		if (fifo32_status(&fifo) != 0)
		{
			i = fifo32_get(&fifo);
			io_sti();
			if (256 <= i && i <= 511)
			{
				sprintf(s, "%02x", i-256);
				boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
				putfont8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
				sheet_refresh(sht_back, 0, 16, 16, 32);
			}
			else if (512 <= i && i <= 767)
			{
				if (mouse_decode(&mdec, i-512) > 0)
				{
					//三字节到齐，显示
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0)
						s[1] = 'L';
					if ((mdec.btn & 0x02) != 0)
						s[3] = 'R';
					if ((mdec.btn & 0x04) != 0)
						s[2] = 'C';
					boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32+8*15-1, 31);
					putfont8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
					sheet_refresh(sht_back, 32, 16, 32 + 15 * 8, 32);
					//鼠标移动
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) mx = 0;
					if (my < 0) my = 0;
					if (mx > binfo->scrnx - 1) mx = binfo->scrnx - 1;
					if (my > binfo->scrny - 1) my = binfo->scrny - 1;
					sprintf(s, "(%3d, %3d)", mx, my);
					boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
					putfont8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
					sheet_refresh(sht_back, 0, 0, 80, 16);
					sheet_slide(sht_mouse, mx, my);
				}
			}
			else if (i == 10)
			{
				putfont8_asc(buf_back, binfo->scrnx, 0, 64, COL8_FFFFFF, "10(sec)");
				sheet_refresh(sht_back, 0, 64, 56, 80);
				sprintf(s, "%010d", count);
				boxfill8(buf_win, 160, COL8_C6C6C6, 40, 28, 119, 43);
				putfont8_asc(buf_win, 160, 40, 28, COL8_000000, s);
				sheet_refresh(sht_win, 40, 28, 120, 44);
			}
			else if (i == 3)
			{
				putfont8_asc(buf_back, binfo->scrnx, 0, 80, COL8_FFFFFF, "3(sec)");
				sheet_refresh(sht_back, 0, 80, 48, 96);
				count = 0;
			}
			else if (i == 1)
			{
				timer_init(timer3, &fifo, 0);
				boxfill8(buf_back, binfo->scrnx, COL8_FFFFFF, 8, 96, 15, 111);
				timer_settime(timer3, 50);
				sheet_refresh(sht_back, 8, 96, 16, 112);
			}
			else if (i == 0)
			{
				timer_init(timer3, &fifo, 1);
				boxfill8(buf_back, binfo->scrnx, COL8_848484, 8, 96, 15, 111);
				timer_settime(timer3, 50);
				sheet_refresh(sht_back, 8, 96, 16, 112);
			}
		}
		else
            //io_stihlt();
            io_sti();
    }   
}

void make_window8(unsigned char *buf, int xsize, int ysize, char *title)
{
	static char closebtn[14][16] =
	{
		"OOOOOOOOOOOOOOO@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@",
		"@@@@@@@@@@@@@@@@"
	};
	int x, y;
	char c;
	boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, xsize-1, 0);
	boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, xsize-2, 1);
	boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, 0, ysize-1);
	boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, 1, ysize-2);
	boxfill8(buf, xsize, COL8_848484, xsize-2, 1, xsize-2, ysize-2);
	boxfill8(buf, xsize, COL8_000000, xsize-1, 0, xsize-1, ysize-1);
	boxfill8(buf, xsize, COL8_C6C6C6, 2, 2, xsize-3, ysize-3);
	boxfill8(buf, xsize, COL8_000084, 3, 3, xsize-4, 20);
	boxfill8(buf, xsize, COL8_848484, 1, ysize-2, xsize-2, ysize-2);
	boxfill8(buf, xsize, COL8_000000, 0, ysize-2, xsize-1, ysize-1);
	putfont8_asc(buf, xsize, 24, 4, COL8_FFFFFF, title);
	for (y = 0; y < 14; ++y)
	{
		for (x = 0; x < 16; ++x)
		{
			c = closebtn[y][x];
			if (c == '@')
				c = COL8_000000;
			else if (c == '$')
				c = COL8_848484;
			else if (c == 'Q')
				c = COL8_C6C6C6;
			else
				c = COL8_FFFFFF;
			buf[ (5+y) * xsize + (xsize - 21 + x) ] = c;
		}
	}
	return;
}
