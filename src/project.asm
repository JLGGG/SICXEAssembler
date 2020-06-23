PROG     START   0    
         STL     RETADR  
	   +LDT     REF1
	    LDT     REF1,X
         ADD     REF1
	    ADDR    A,S
	    AND     REF1
	    CLEAR   X
LABEL1   COMP    REF1
	    COMPR   S,T
	    DIV     REF1
	    DIVR    A,T
	    HIO
	    J       TEST
LABEL2   JEQ     TEST
	    JGT     TEST
	    JLT     TEST
	   +JSUB    TEST
	    JSUB    TEST
	    LDA     REF1
	    LDB     REF1
LABEL3   LDCH    REF1
	    LDL     RETADR
	    LDS     REF1
	    LDT     REF1
	    LDX     REF1
	    LPS     STAT
	    MUL     REF1
	    MULR    A,B
	    OR      REF1
	    RD      DEVICE
LABEL4   RMO     S,B
         RSUB
	    SHIFTL  REF1,A
	    SHIFTR  REF1,A
	    SIO
	    SSK     REF1
	    STA     REF1
	    STB     REF1
	    STCH    REF1
	    STI     REF1
LABEL5   STL     RETADR
	    STS     BOX
	    STSW    BOX
	    STT     BOX
	    STX     BOX
	    SUB     REF1
	    SUBR    A,S
LABEL6   SVC     T
	    TD      DEVICE
	    TIO
	    TIX     STAT
	    TIXR    A
	    WD      DEVICE 
REF1     WORD    10
STAT     WORD    1
EOF      BYTE    c'EOF'
DEVICE   BYTE    X'05'
RETADR   RESW    1
BOX      RESW    1
TEST     WORD    1 
        +LDS     EOF
	    LDF     FLOAT1
	    COMPF   FLOAT2
	    ADDF    FLOAT2
	    DIVF    FLOAT2
	    MULF    FLOAT2
	    SUBF    FLOAT2
	    STF     FLOAT1
	    FIX
	    FLOAT
	    NORM
FLOAT1   WORD    1.5
FLOAT2   WORD    10.5
	    END     PROG      