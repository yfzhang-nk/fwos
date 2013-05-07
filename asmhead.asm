; haribote-os boot asm
; TAB=4
[SECTION .data]
VBEMODE EQU 0x103
BOTPAK	EQU 0x00280000
DSKCAC	EQU 0x00100000		
DSKCAC0	EQU 0x00008000		
; boot info
CYLS	EQU 0x0ff0 ;设定启动区
LEDS	EQU 0x0ff1 ;
VMODE	EQU 0x0ff2 ;关于颜色数目的信息，颜色的位数
SCRNX	EQU 0x0ff4 ;分辨率的X
SCRNY	EQU 0x0ff6 ;分辨率的Y
VRAM	EQU 0x0ff8 ;图像缓冲区的开始地址

; ----------------------------------------------------
; disk
; 0      0x200       0x1000
; |ipl.bin|            |asmhead.sys            |
;0x7c00  0x7e00      0x9000 
; ----------------------------------------------------
[SECTION .head]
start:
	ORG 0c400h
	; 确认VBE是否存在
	MOV AX, 0x9000
	MOV ES, AX
	MOV DI, 0
	MOV AX, 0x4f00
	INT 0x10
	CMP AX, 0x004f
	JNE scrn320

	;检查VBE版本
	MOV AX, [ES:DI+4]
	CMP AX, 0x0200
	JB scrn320
	
	;取得画面模式信息
	MOV CX, VBEMODE
	MOV AX, 0x4f01
	INT 0x10
	CMP AX, 0x004f
	JNE scrn320

; WORD [ES : DI+0x00] : 模式属性 
; WORD [ES : DI+0x12] : X的分辨率
; WORD [ES : DI+0x14] : Y的分辨率
; WORD [ES : DI+0x19] : 颜色数目
; WORD [ES : DI+0x1b] : 颜色的指定方法
; WORD [ES : DI+0x28] : VRAM的地址
	; 画面模式信息的确认
	CMP BYTE [ES:DI+0x19], 8
	JNE scrn320
	CMP BYTE [ES:DI+0x1b], 4
	JNE scrn320
	MOV AX, [ES:DI+0x00]
	AND AX, 0x0080
	JZ scrn320

; 0x101 ------ 640 * 480 * 8bit color
; 0x103 ------ 800 * 600 * 8bit color
; 0x105 ------ 1024 * 768 * 8bit color
; 0x107 ------ 1280 * 1024 * 8bit color
	;画面模式切换
	MOV BX, VBEMODE+0x4000
	MOV AX, 0x4f02
	INT 0x10
	MOV BYTE [VMODE], 8
	MOV AX, [ES:DI+0x12]
	MOV [SCRNX], AX
	MOV AX, [ES:DI+0x14]
	MOV [SCRNY], AX
	MOV EAX, [ES:DI+0x28]
	MOV [VRAM], EAX
	JMP keystatus

scrn320
	MOV AX, 0x0013
	INT 0x10
	MOV	BYTE [VMODE], 8 ;记录画面模式
	MOV WORD [SCRNX], 320 
	MOV WORD [SCRNY], 200 
	MOV DWORD [VRAM], 0x000a0000

keystatus:
; 用BIOS取得键盘上各种LED指示灯的状态
	MOV AH, 0x02
	INT 0x16 ; keyboard BIOS
	MOV [LEDS], AL
; PIC 关闭一切中断
; 根据AT兼容机的规格，如果要初始化PIC，必须在CLI之前进行，否则有时会挂起。随后进行PIC的初始化
	MOV AL, 0xff
	OUT	0x21, AL ; 相当于 io_out8(PIC0_IMR, 0xff) 禁止主PIC的全部中断
	NOP ; 如果连续执行OUT指令，有些机种会无法正常运行
	OUT	0xa1, AL ; 相当于 io_out8(PIC1_IMR, 0xff) 禁止从PIC的全部中断

	CLI ; 禁止CPU级别的中断
; 为了让CPU能够访问1M以上的内存空间，设定A20GATE
	CALL waitkbdout
	MOV	AL, 0xd1
	OUT	0x64, AL
	CALL waitkbdout
	MOV AL, 0xdf ; enable A20
	OUT	0x60, AL
	CALL waitkbdout

CPU 486
	LGDT [GDTR0]
	MOV	EAX, CR0
	AND EAX, 0x7fffffff
	OR	EAX, 0x00000001
	MOV	CR0, EAX
	JMP pipelineflush
pipelineflush:
	MOV AX, 1*8
	MOV	DS, AX
	MOV	ES, AX
	MOV	FS, AX
	MOV	GS, AX
	MOV SS, AX

	MOV ESI, 0xcc00
	MOV	EDI, BOTPAK + 0xcc00 
	MOV ECX, 512*1024/4
	CALL memcpy

	MOV ESI, 0x7c00
	MOV	EDI, DSKCAC
	MOV	ECX, 512/4
	CALL memcpy

	MOV ESI, DSKCAC0+512
	MOV	EDI, DSKCAC+512
	MOV	ECX, 0
	MOV CL, BYTE [CYLS]
	IMUL ECX, 512*18*2/4
	SUB	ECX, 512/4
	CALL memcpy

	;MOV		EBX,BOTPAK
	;MOV		ECX,[EBX+16]
	;ADD		ECX,3
	;SHR		ECX,2
	;JZ		skip
	;MOV		ESI,0x00310000 
	;ADD		ESI,EBX
	;MOV		EDI,[EBX+12]
	;CALL	memcpy

skip:
	MOV ESP, 0x00310000 ;栈的起始地址
	JMP DWORD 2*8:0xcc00

waitkbdout:
	IN	AL, 0x64
	AND	AL, 0x02
	IN	AL, 0x60
	JNZ waitkbdout
	RET

memcpy:
	MOV EAX,[ESI]
	ADD	ESI, 4
	MOV	[EDI], EAX
	ADD	EDI, 4
	SUB	ECX, 1
	JNZ memcpy
	RET

ALIGNB 16
GDT0:
	TIMES 8	DB 0
	DW	0xffff, 0x0000, 0x9200, 0x00cf
	DW 	0xffff, 0x0000, 0x9a28, 0x0047
	DW	0
GDTR0:
	DW	8*3-1
	DD	GDT0

ALIGNB	16

bootpack:
