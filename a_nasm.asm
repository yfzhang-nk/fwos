[CPU 486]
[BITS 32]
GLOBAL api_putchar, api_end, api_putstr0
[SECTION .text]
api_putchar: ; void api_putchar(int c)
	MOV EDX, 1
	MOV AL, [ESP+4]
	INT 0x40
	RET

api_putstr0: ; void api_putstr(char *s)
	PUSH EBX
	MOV EDX, 2
	MOV EBX, [ESP+8]
	INT 0x40
	POP EBX
	RET

api_end:  ; void api_end(void)
	MOV EDX, 4
	INT 0x40
