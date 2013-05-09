#include "bootpack.h"
#include <stdio.h>

void main(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40];
	int mx, my, i, cursor_x, cursor_c;
	int key_to=0, key_shift=0, keycmd_wait = -1;
	int key_leds = (binfo->leds >> 4) & 7;  //leds的4/5/6位分别是ScrollLock/NumLock/CapsLock
	int fifobuf[128], keycmd_buf[32];
	unsigned int memtotal;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHTCTL *shtctl;
	struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_cons;
	unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons;
	struct TIMER *timer;
	struct FIFO32 fifo, keycmd;
	static char keytable[2][0x54] = {
		0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,   
		0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0, 0,
		'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',   0, '\\', 
		'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.',
		0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   
		0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0, 0,
		'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',   0, '|', 
		'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.'
	};
	struct TASK *task_a, *task_cons;
	struct CONSOLE *cons;
	mx = (binfo->scrnx - 16) / 2; 
	my = (binfo->scrny - 28 - 16) / 2;
	init_gdtidt();
	init_pic();
	io_sti(); //IDT／PIC 的初始化完成，于是开放CPU中断

	fifo32_init(&fifo, 128, fifobuf, 0);
	fifo32_init(&keycmd, 32, keycmd_buf, 0);

	init_pit();

	io_out8(PIC0_IMR, 0xf8); // 开放 PIC1, 键盘终端, 计时器 11111000
	io_out8(PIC1_IMR, 0xef); // 开放鼠标中断 11101111

	timer = timer_alloc();
	timer_init(timer, &fifo, 1);
	timer_settime(timer, 50);

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

	task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 2);

	sht_cons = sheet_alloc(shtctl);
	buf_cons = (unsigned char *) memman_alloc_4k(memman, 256*165);
	sheet_setbuf(sht_cons, buf_cons, 256, 165, -1);
	make_window8(buf_cons, 256, 165, "console", 0);
	make_textbox8(sht_cons, 8, 28, 240, 128, COL8_000000);
	task_cons = task_alloc();
	task_cons->tss.esp = memman_alloc_4k(memman, 64*1024) + 64*1024 - 8;
	task_cons->tss.eip = (int) &console_task;
	task_cons->tss.es = 1*8;
	task_cons->tss.cs = 2*8;
	task_cons->tss.ss = 1*8;
	task_cons->tss.ds = 1*8;
	task_cons->tss.fs = 1*8;
	task_cons->tss.gs = 1*8;
	*((int *) (task_cons->tss.esp+4)) = (int) sht_cons;
	*((int *) (task_cons->tss.esp+8)) = (int) memtotal;
	task_run(task_cons, 2, 2);

	init_mouse_cursor8(buf_mouse, 99);
	make_window8(buf_win, 160, 52, "task_a", 1);
	make_textbox8(sht_win, 8, 28, 144, 16, COL8_FFFFFF);
	cursor_x = 8;
	sheet_slide(sht_back, 0, 0);
	sheet_slide(sht_cons, 32, 4);
	sheet_slide(sht_win, 64, 56);
	sheet_slide(sht_mouse, mx, my);
	sheet_updown(sht_back, 0);
	sheet_updown(sht_cons, 1);
	sheet_updown(sht_win, 2);
	sheet_updown(sht_mouse, 3);

	//为了避免和键盘当前状态冲突，在一开始先进行设置
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);
	
    for (;;) 
	{
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0)
		{
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
        io_cli();
		if (fifo32_status(&fifo) != 0)
		{
			i = fifo32_get(&fifo);
			io_sti();
			if (256 <= i && i <= 511)
			{
				if (i < 0x54 + 256)
				{
					if (key_shift == 0)
					{
						s[0] = keytable[0][i - 256];
					}
					else
					{
						s[0] = keytable[1][i - 256];
					}
				}
				else 
					s[0] = 0;

				if (s[0] >= 'A' && s[0] <= 'Z')
				{
					if ((key_shift == 0 && (key_leds & 4) == 0) || (key_shift != 0 && (key_leds & 4) != 0))
						s[0] += 0x20;
				}
				if (s[0] != 0)
				{
					if (key_to == 0)
					{
						if (cursor_x < 128) 
						{
							s[1] = 0;
							putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
							cursor_x += 8;
						}
					}
					else
						fifo32_put(&task_cons->fifo, s[0] + 256);
				}
				if (i == 256 + 0x0e)  //退格键
				{
					if (key_to == 0)
					{
						if (cursor_x > 8)
						{
							putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
							cursor_x -= 8;
						}
					}
					else
						fifo32_put(&task_cons->fifo, 8+256);
				}
				if (i == 256 + 0x0f) //Tab键
				{
					if (key_to == 0)
					{
						key_to = 1;
						make_wtitle8(buf_win, sht_win->bxsize, "task_a", 0);
						make_wtitle8(buf_cons, sht_cons->bxsize, "console", 1);
						cursor_c = -1; //不显示光标
						boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cursor_x, 28, cursor_x + 7, 43);
						fifo32_put(&task_cons->fifo, 2); //命令行打开光标
					}
					else
					{
						key_to = 0;
						make_wtitle8(buf_win, sht_win->bxsize, "task_a", 1);
						make_wtitle8(buf_cons, sht_cons->bxsize, "console", 0);
						cursor_c = COL8_000000;
						fifo32_put(&task_cons->fifo, 3); //命令行关闭光标
					}
					sheet_refresh(sht_win, 0, 0, sht_win->bxsize, 21);
					sheet_refresh(sht_cons, 0, 0, sht_cons->bxsize, 21);

				}
				if (i == 256 + 0x1c) //回车
				{
					if (key_to != 0)
						fifo32_put(&task_cons->fifo, 10 + 256);
				}
				if (i == 256 + 0x2a) //left shift on
					key_shift |= 1;
				if (i == 256 + 0x36) //right shift on
					key_shift |= 2;
				if (i == 256 + 0xaa) //left shift off
					key_shift &= ~1;
				if (i == 256 + 0xb6) // right shift off
					key_shift &= ~2;
				if (i == 256 + 0x3a) // CapsLock
				{
					key_leds ^= 4;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x45) //NumLock 
				{
					key_leds ^= 2;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x46) // CapsLock
				{
					key_leds ^= 1;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0xfa) //键盘成功接收到了数据
				{
					keycmd_wait = -1;
				}
				if (i == 256 + 0xfe) //键盘没有成功接收到数据
				{
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}
				if (i == 256 + 0x2e && key_shift != 0 && task_cons->tss.ss0 != 0)
				{
					cons = (struct CONSOLE *) *((int *)0x0fec);
					cons_putstr0(cons, "\nBreak(key)\n");
					io_cli();
					task_cons->tss.eax = (int) &(task_cons->tss.esp0);
					task_cons->tss.eip = (int) end_app;
					io_sti();
				}
				//重新显示光标
				if (cursor_c >= 0)
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
			}
			else if (512 <= i && i <= 767)
			{
				if (mouse_decode(&mdec, i-512) > 0)
				{
					//三字节到齐，显示
					//鼠标移动
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) mx = 0;
					if (my < 0) my = 0;
					if (mx > binfo->scrnx - 1) mx = binfo->scrnx - 1;
					if (my > binfo->scrny - 1) my = binfo->scrny - 1;
					sheet_slide(sht_mouse, mx, my);
					if ((mdec.btn & 0x01) != 0)
					{
						sheet_slide(sht_win, mx-80, my-8);
					}
				}
			}
			else if (i <= 1)
			{
				if (i != 0) 
				{
					timer_init(timer, &fifo, 0); 
					if (cursor_c >= 0)
						cursor_c = COL8_000000;
				} 
				else 
				{
					timer_init(timer, &fifo, 1); 
					if (cursor_c >= 0)
						cursor_c = COL8_FFFFFF;
				}
				timer_settime(timer, 50);
				if (cursor_c >= 0)
				{
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
					sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
				}
			}
		}
		else
		{
            //io_stihlt();
			task_sleep(task_a);
            io_sti();
		}
    }   
}

