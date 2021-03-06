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
#define ADR_DISKIMG		0x00100000
#define ADR_IDT         0x0026f800
#define LIMIT_IDT       0x000007ff
#define ADR_GDT         0x00270000
#define LIMIT_GDT       0x0000ffff
#define ADR_BOTPAK      0x00280000
#define LIMIT_BOTPAK    0x0007ffff
#define AR_DATA32_RW    0x4092
#define AR_CODE32_ER    0x409a
#define AR_INTGATE32    0x008e
#define AR_TSS32        0x0089


// 键盘鼠标
#define PORT_KEYDAT	0x0060
#define PORT_KEYSTA 0x0064
#define PORT_KEYCMD 0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47
#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE 0xf4
#define KEYCMD_LED 0xed

// 内存
#define EFLAGS_AC_BIT 0x0004000
#define CR0_CACHE_DISABLE 0x60000000
#define MEMMAN_FREES 4090 // about 32MB, MEMMAN is short for Memory Management
#define MEMMAN_ADDR 0x003c0000

//图层
#define MAX_SHEETS 256

//定时器
#define MAX_TIMER 500

//多任务
#define MAX_TASKS 1000   //max task count
#define TASK_GDT0 3      //定义从GDT的几号开始分配TSS
#define MAX_TASKS_LV 100
#define MAX_TASKLEVELS 10

struct BOOTINFO 
{
	char cyls, leds, vmode, reserve;
	short scrnx, scrny;
	char *vram;
};

// GDT / IDT
struct SEGMENT_DESCRIPTOR {
	short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
};

struct GATE_DESCRIPTOR {
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
};

struct FIFO32
{
	int *buf;
	int p, q, size, free, flags;
	struct TASK *task;
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

// multi task
struct TSS32
{
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;
	int ldtr, iomap;
};
struct TASK
{
	int sel, flags;
	int priority, level;
	struct FIFO32 fifo;
	struct TSS32 tss;
};
struct TASKLEVEL
{
	int running;  //正在运行的任务数量
	int now; //记录当前正在运行的是哪个任务
	struct TASK *tasks[MAX_TASKS_LV];
};
struct TASKCTL
{
	int now_lv;  //现在活动中的level
	char lv_change;  //在下次任务切换时是否需要改变LEVEL
	struct TASKLEVEL level[MAX_TASKLEVELS];
	struct TASK tasks0[MAX_TASKS];
};
struct TASKCTL *taskctl;
struct TIMER *task_timer;

struct FILEINFO
{
	unsigned char name[8], ext[3], type;
	char reserve[10];
	unsigned short time, date, clustno;
	unsigned int size;
};

struct CONSOLE
{
	struct SHEET *sht;
	int cur_x, cur_y, cur_c;
};

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
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);

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

/* c function for PIT */
void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);

/* window */
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);

/* multi task */
void load_tr(int tr);
void farjmp(int eip, int cs);
void farcall(int eip, int cs);
void console_task(struct SHEET *sht_cons, unsigned int memtotal);
struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
struct TASK *task_now(void);

/* GDT */
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void asm_os_api(void);

/* console */
void cons_putchar(struct CONSOLE *cons, int chr, char move);
void cons_putstr1(struct CONSOLE *cons, char *s, int l);
void cons_putstr0(struct CONSOLE *cons, char *s);
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal);
void cmd_mem(struct CONSOLE *cons, unsigned int memtotal);
void cmd_cls(struct CONSOLE *cons);
void cmd_dir(struct CONSOLE *cons);
void cmd_cat(struct CONSOLE *cons, int *fat, char *cmdline);
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline);
void cons_newline(struct CONSOLE *cons);
void console_task(struct SHEET *sht_cons, unsigned int memtotal);
int *os_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);
void asm_inthandler0d(void);
void asm_inthandler0c(void);
void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
void end_app(void);
