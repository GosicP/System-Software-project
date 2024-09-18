

enum { r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15 };

enum { status, handler, cause, sp = 14U, pc = 15U };

typedef unsigned int reg;

// HALT
#define HALT 0x00

// SWI (Software Interrupt)
#define SWI 0x10

// CALL Instruction (subroutine call)
#define CALL_REG 0x20  // MMMM == 0b0000
#define CALL_MEM 0x21  // MMMM == 0b0001

// JMP (Jump Instructions)
#define JMP_REG 0x30       // MMMM == 0b0000
#define JMP_EQ 0x31        // MMMM == 0b0001
#define JMP_NEQ 0x32       // MMMM == 0b0010
#define JMP_GT 0x33        // MMMM == 0b0011
#define JMP_MEM 0x38       // MMMM == 0b1000
#define JMP_MEM_EQ 0x39    // MMMM == 0b1001
#define JMP_MEM_NEQ 0x3A   // MMMM == 0b1010
#define JMP_MEM_GT 0x3B    // MMMM == 0b1011

// XCHG (Atomic Swap)
#define XCHG 0x40

// ARITHMETIC Instructions
#define ADD 0x50       // MMMM == 0b0000
#define SUB 0x51       // MMMM == 0b0001
#define MUL 0x52       // MMMM == 0b0010
#define DIV 0x53       // MMMM == 0b0011

// LOGICAL Instructions
#define NOT 0x60       // MMMM == 0b0000
#define AND 0x61       // MMMM == 0b0001
#define OR  0x62       // MMMM == 0b0010
#define XOR 0x63       // MMMM == 0b0011

// SHIFT Instructions
#define SHL 0x70       // MMMM == 0b0000
#define SHR 0x71       // MMMM == 0b0001

// STORE Instructions
#define STR_REG 0x80   // MMMM == 0b0000
#define STR_MEM 0x82   // MMMM == 0b0010
#define STR_INC 0x81   // MMMM == 0b0001

// LOAD Instructions
#define LDR_CSR 0x90   // MMMM == 0b0000
#define LDR_REG 0x91   // MMMM == 0b0001
#define LDR_MEM 0x92   // MMMM == 0b0010
#define LDR_MEM_INC 0x93  // MMMM == 0b0011
#define CSR_LOAD 0x94    // MMMM == 0b0100
#define CSR_OR 0x95      // MMMM == 0b0101
#define CSR_MEM 0x96     // MMMM == 0b0110
#define CSR_MEM_INC 0x97 // MMMM == 0b0111

enum masksStatus { interruptStatus = 4U }; 

enum masksCause {
    illegal = 1,
    software = 4
};

struct registersCpu {
  reg gpr[16];
  reg csr[3];
};
