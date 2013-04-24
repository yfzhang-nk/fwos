#include "bootpack.h"
#define FLAGS_OVERRUN 0x0001

void fifo32_init(struct FIFO32 *fifo, int size, int *buf)
/* 初始化FIFO缓冲区 */
{
	fifo->size = size;
	fifo->buf = buf;
	fifo->free = size; //缓冲区大小
	fifo->flags = 0;
	fifo->p = 0; // 下一个数据写入位置
	fifo->q = 0; // 下一个数据读出位置
	return;
}

int fifo32_put(struct FIFO32 *fifo, int data)
/* 向FIFO里传送数据并保存 */
{
	if (fifo->free == 0) //溢出
	{
		fifo->flags |= FLAGS_OVERRUN;
		return -1;
	}
	fifo->buf[fifo->p] = data;
	fifo->p++;
	fifo->p = fifo->p % fifo->size;
	fifo->free--;
	return 0;
}

int fifo32_get(struct FIFO32 *fifo)
/* 从FIFO里读出一个数据 */
{
	int data;
	if (fifo->free == fifo->size)
	{
		return -1;
	}
	data = fifo->buf[fifo->q];
	fifo->q++;
	fifo->q = fifo->q % fifo->size;
	fifo->free++;
	return data;
}

int fifo32_status(struct FIFO32 *fifo)
/* 输出FIFO里有多少数据 */
{
	return fifo->size - fifo->free;
}
