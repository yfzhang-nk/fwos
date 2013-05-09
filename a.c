void api_putchar(int c);
void api_end(void);

void main(void)
{
	char a[100];
	a[124] = 'B';
	api_putchar(a[124]);
	api_end();
}
