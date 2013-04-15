void* sbrk(int incr)
{
	static char heap[8*1024];
	static int first=1;
	if (first==0 || incr > sizeof(heap))
		return (void*) 0;
	return (void*) heap;
}


