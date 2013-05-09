void api_putchar(int c);
void api_putstr0(char *s);
void api_end(void);

void main(void)
{
	//api_putstr0("hello, world\n");
	char a[100];
	a[123] = 'A';
	api_putchar(a[123]);
	a[11223] = 'B';
	api_putchar(a[11223]);
	api_end();
}
