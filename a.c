void api_putchar(int c);
void api_end(void);

void main(void)
{
	*((char *) 0x102600) = 0;
	api_end();
}
