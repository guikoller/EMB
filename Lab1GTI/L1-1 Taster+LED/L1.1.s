	AREA L1, CODE 
	INCLUDE LPC1768.inc
	EXPORT __main
	ALIGN

	ENTRY

__main PROC
	
		LDR R0, = FIO2DIR0
		LDR R1, [R0]
		ORR R1, R1, #0x4
		STRB R1, [R0]
	
		LDR R0, = FIO2MASK
		LDR R1, [R0]
		LDR R3, =0xFFFFFBFB
		ORR R1, R1, R3
		STR R1, [R0]
		
LOOP

		LDR R2,	= FIO2PIN0
		LDR R1, = FIO2PIN1
		LDR R3,	[R1]
		EOR R3, R3, #0xf
		STR R3, [R2]
		
		
		B LOOP
		ENDP
		END