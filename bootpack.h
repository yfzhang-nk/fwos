#include "color.h"

// 主从PIC 
#define PIC0_ICW1       0x0020
#define PIC0_OCW2       0x0020
#define PIC0_IMR        0x0021
#define PIC0_ICW2       0x0021
#define PIC0_ICW3       0x0021
#define PIC0_ICW4       0x0021
#define PIC1_ICW1       0x00a0
#define PIC1_OCW2       0x00a0
#define PIC1_IMR        0x00a1
#define PIC1_ICW2       0x00a1
#define PIC1_ICW3       0x00a1
#define PIC1_ICW4       0x00a1

// BOOTINFO 存放位置
#define ADR_BOOTINFO    0x00000ff0

// 键盘鼠标
#define PORT_KEYDAT	0x0060
#define PORT_KEYSTA 0x0064
#define PORT_KEYCMD 0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47
#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE 0xf4

// 内存
#define EFLAGS_AC_BIT 0x0004000
#define CR0_CACHE_DISABLE 0x60000000
#define MEMMAN_FREES 4090 // about 32MB, MEMMAN is short for Memory Management
#define MEMMAN_ADDR 0x003c0000

//图层
#define MAX_SHEETS 256

//定时器
#define MAX_TIMER 500

struct BOOTINFO 
{
	char cyls, leds, vmode, reserve;
	short scrnx, scrny;
	char *vram;
};

struct FIFO32
{
	int *buf;
	int p, q, size, free, flags;
};

struct MOUSE_DEC
{
	unsigned char buf[3], phase;
	int x, y, btn;
};
struct MOUSE_DEC mdec;

//memory management
struct FREEINFO
{
	unsigned int addr, size;
};

struct MEMMAN
{
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};

// sheet mangement
struct SHTCTL;
struct SHEET
{
	unsigned char *buf;
	int bxsize, bysize, vx0, vy0, col_inv/*透明色色号*/, height, flags;
	struct SHTCTL *ctl;
};

struct SHTCTL
{
	unsigned char *vram, *map;
	int xsize, ysize, top;
	struct SHEET *sheets[MAX_SHEETS];
	struct SHEET sheets0[MAX_SHEETS];
};

// timer
struct TIMER 
{
	struct TIMER *next;
	unsigned int timeout, flags;
	struct FIFO32 *fifo;
	int data;
};

struct TIMERCTL 
{
	unsigned int count, next;
	struct TIMER *t0;
	struct TIMER timers0[MAX_TIMER];
};
struct TIMERCTL timerctl;

/* asm function */
void io_out8(int port, int data);
int io_in8(int port);
int load_cr0(void);
void store_cr0(int cr0);
void io_sti(void);
void io_cli(void);
int io_load_eflags(void);
void io_store_eflags(int eflags);

/* c function */
void init_palette(void);
void init_screen(char* vram, int xsize, int ysize);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void putfont8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);
void init_gdtidt(void);
void init_pic(void);

/* c function from fifo.c */
int fifo32_status(struct FIFO32 *fifo);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_put(struct FIFO32 *fifo, int data);
void fifo32_init(struct FIFO32 *fifo, int size, int *buf);

/* c function keyboard&mouse*/
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32 *fifo, int data0);
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

/* c function memory */
unsigned int memtest(unsigned int start, unsigned int end);
/* memory management */
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

/* sheet management */
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0);

/* window */
void make_window8(unsigned char *buf, int xsize, int ysize, char *title);

/* c function for PIT */
void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, unsigned char data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
