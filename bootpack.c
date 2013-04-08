void main(void)
{
	char *p = (char*) 0xa0000;
	int i;
	for(i = 0x0; i <=0xffff; ++i)
	{
		*(p+i) = 0x15;
	}
}
