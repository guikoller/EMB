	AREA L1, CODE
	INCLUDE LPC1768.inc
	EXPORT __main
	ALIGN

ENTRY

__main PROC

    LDR R0, = PINSEL4
    LDR R1, [R0]
    LDR R3, = 0xC0C0
    AND R1, R1, R3
    STR R1, [R0]
    
    LDR R0,= FIO2DIR0
    LDR R1, [R0]
    LDR R3, = 0xFF
    ORR R1, R1, R3
    STRB R1, [R0]
    
    LDR R0, = FIO2MASK
    LDR R1, [R0]
    LDR R3, = 0xFFFFFB77
    ORR R1, R1, R3
    STR R1, [R0]
    
    LDR R2, =FIO2PIN0
    LDR R1, =FIO2PIN1
    
UP
    LDR R3, [R1]
    TST R3, #0x4
    BNE UP
    
    LDR R8, = 10000000

DELAY
    SUBS R8, R8, #1
    BNE DELAY

CHECK
    LDR R3, [R1]
    TST R3, #0x4
    BNE UP

DOWN
    LDR R3, [R1]
    TST R3, #0x4
	BEQ DOWN
    MOV R4, #0x88
    STR R4, [R2]
    B UP
    
    ENDP
    
    END