#include "bootpack.h"

unsigned int memtest_sub(unsigned int start, unsigned int end)
{
	unsigned int i, *p, old, pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;
	for (i = start; i <= end; i += 0x1000)
	{
		p = (unsigned int *) (i + 0xffc);
		old = *p; //保存旧值
		*p = pat0; //试写
		*p ^= 0xffffffff; //反转
		if (*p != pat1)
		{
		not_memory:
			*p = old;
			break;
		}
		*p ^= 0xffffffff; // 再次反转
		if (*p != pat0)
		{
			goto not_memory;
		}
		*p = old; // 恢复
	}
	return i;
}

unsigned int memtest(unsigned int start, unsigned int end)
{
	char flag486 = 0;
	unsigned int eflag, cr0, i;
	// 确认CPU是386还是486以上
	eflag = io_load_eflags();
	eflag |= EFLAGS_AC_BIT; 
	io_store_eflags(eflag);
	eflag = io_load_eflags();
	if ((eflag & EFLAGS_AC_BIT) != 0) // 如果是386，即使设定了AC=1，AC的值会自动回到0
	{
		flag486 = 1;
	}
	eflag &= ~EFLAGS_AC_BIT;
	io_store_eflags(eflag);
	if (flag486 != 0)
	{
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE;
		store_cr0(cr0);
	}
	i = memtest_sub(start, end);
	if (flag486 != 0)
	{
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE;
		store_cr0(cr0);
	}
	return i;
}

void memman_init(struct MEMMAN *man)
{
	man->frees = 0; //可用信息数目
	man->maxfrees = 0; //用于观察可用状况：frees的最大值
	man->lostsize = 0; //释放失败的内存的大小总和
	man->losts = 0; //释放失败次数
	return;
}

unsigned int memman_total(struct MEMMAN *man)
// 报告空余内存大小的合计
{
	unsigned int i, t=0;
	for (i=0; i < man->frees; ++i)
	{
		t += man->free[i].size;
	}
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
	unsigned int i, a;
	for (i=0; i < man->frees; ++i)
	{
		if (man->free[i].size >= size) // 找到了足够大的内存
		a = man->free[i].addr;
		man->free[i].addr += size;
		man->free[i].size -= size;
		if (man->free[i].size == 0)
		{
			man->frees--;
			for (;i<man->frees; ++i)
			{
				man->free[i] = man->free[i+1];
			}
		}
		return a;
	}
	return 0; //没有可用内存
}

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
	unsigned int a;
	size = (size+0xfff) & 0xfffff000;
	a = memman_alloc(man, size);
	return a;
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i, j;
	for (i=0; i<man->frees; ++i)
	{
		if (man->free[i].addr > addr)
			break;
	}
	if (i > 0)
	{
		if ((man->free[i-1].addr + man->free[i-1].size) == addr)
		{
			man->free[i-1].size += size;
			if (i < man->frees)
			{
				if (addr+size == man->free[i].addr)
				{
					man->free[i-1].size += man->free[i].size;
					man->frees --;
					for (; i<man->frees; ++i)
						man->free[i] = man->free[i+1];
				}
			}
			return 0;
		}
	}
	if (i < man->frees)
	{
		
		if (man->free[i].addr == (addr+size))
		{
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0;
		}
	}
	if (man->frees < MEMMAN_FREES)
	{
		for (j=man->frees; j>i; ++j)
		{
			man->free[j] = man->free[j-1];
		}
		man->frees ++;
		if (man->maxfrees < man->frees)
			man->maxfrees = man->frees;
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0;
	}
	man->losts++;
	man->lostsize += size;
	return -1; //failed
}

int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i;
	size = (size+0xfff) & 0xfffff000;
	i = memman_free(man, addr, size);
	return i;
}
