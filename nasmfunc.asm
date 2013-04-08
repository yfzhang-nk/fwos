[BITS 32]
EXTERN main
_start:
	CALL main
fin:
	HLT
	JMP fin
