#include "color.h"

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

#define ADR_BOOTINFO    0x00000ff0

#define PORT_KEYDAT	0x0060
#define PORT_KEYSTA 0x0064
#define PORT_KEYCMD 0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47
#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE 0xf4

/* asm function */
void io_out8(int port, int data);
int io_in8(int port);
void io_sti(void);
/* c function */
void init_palette(void);
void init_screen(char* vram, int xsize, int ysize);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void putfont8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);
void init_gdtidt(void);
void init_pic(void);

struct BOOTINFO {
	char cyls, leds, vmode, reserve;
	short scrnx, scrny;
	char *vram;
};

struct FIFO8
{
	unsigned char *buf;
	int p, q, size, free, flags;
};

struct FIFO8 keyfifo;
struct FIFO8 mousefifo;

/* c function from fifo.c */
int fifo8_status(struct FIFO8 *fifo);
int fifo8_get(struct FIFO8 *fifo);
int fifo8_put(struct FIFO8 *fifo, unsigned char data);
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf);

/* c function */
void wait_KBC_sendready(void);
void init_keyboard(void);
void enable_mouse(void);
