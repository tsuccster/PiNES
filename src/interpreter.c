#include <stdint.h>

#include "./headers/common.h"
#include "./headers/memory.h"


struct Instruction {
    char name[3];
    uint8_t opcode;
    uint8_t cycles;
    uint16_t (*mode)(struct NES*);
    void (*op)(struct NES*, uint16_t (*mode)(struct NES*));
};


/* -----------------
    Addressing Modes
    ---------------- */
uint16_t imp(struct NES *nes) {
    nes->programCounter++;
    return 0x00;
}

uint16_t acc(struct NES *nes) {
    nes->programCounter++;
    return 0x00;
}

uint16_t imm(struct NES *nes) {
    nes->programCounter += 2;
    return (nes->programCounter - 1);
}

uint16_t rel(struct NES *nes) {
    nes->programCounter += 2;
    uint8_t byte2 = cpu_read(nes->programCounter - 1);
    uint16_t branch_destination = ((byte2 & 0x80) ? nes->programCounter - 128 : nes->programCounter);
    byte2 &= 0x7F;
    branch_destination += byte2;
    nes->oopsCycle = ( (branch_destination & 0xFF00) != (nes->programCounter & 0xFF00) ) ? 1 : 0;
    return branch_destination;
}

uint16_t abl(struct NES *nes) {
    nes->programCounter += 3;
    uint8_t byte2 = cpu_read(nes->programCounter - 2);
    uint8_t byte3 = cpu_read(nes->programCounter - 1);
    return ( ((uint16_t)byte3 << 8) | (byte2) );
}

uint16_t zpa(struct NES *nes) {
    nes->programCounter += 2;
    uint8_t byte2 = cpu_read(nes->programCounter - 1);
    return (uint16_t)byte2;
}

uint16_t ind(struct NES *nes) {
    nes->programCounter += 3;
    uint8_t byte2 = cpu_read(nes->programCounter - 2);
    uint8_t byte3 = cpu_read(nes->programCounter - 1);
    uint16_t tempAddress = ((uint16_t)byte3 << 8) | byte2;
    return (byte2 == 0xFF) ? ((((uint16_t)cpu_read(tempAddress & 0xFF00)) << 8) | cpu_read(tempAddress)) : (((uint16_t)cpu_read(tempAddress + 1) << 8) | (cpu_read(tempAddress)));
}

uint16_t aix(struct NES *nes) {
    nes->programCounter += 3;
    uint8_t byte2 = cpu_read(nes->programCounter - 2);
    uint8_t byte3 = cpu_read(nes->programCounter - 1);
    nes->oopsCycle = ((byte2 + nes->xRegister) & 0xFF00) ? 1 : 0;
    return ( ((uint16_t)byte3 << 8) | byte2 ) + nes->xRegister;
}

uint16_t aiy(struct NES *nes) {
    nes->programCounter += 3;
    uint8_t byte2 = cpu_read(nes->programCounter - 2);
    uint8_t byte3 = cpu_read(nes->programCounter - 1);
    nes->oopsCycle = ((byte2 + nes->yRegister) & 0xFF00) ? 1 : 0;
    return ( ((uint16_t)byte3 << 8) | byte2 ) + nes->yRegister;
}

uint16_t zpx(struct NES *nes) {
    nes->programCounter += 2;
    return ((cpu_read(nes->programCounter - 1) + nes->xRegister ) & 0xFF);
}

uint16_t zpy(struct NES *nes) {
    nes->programCounter += 2;
    return ((cpu_read(nes->programCounter - 1) + nes->yRegister ) & 0xFF);
}

uint16_t idx(struct NES *nes) {
    nes->programCounter += 2;
    uint8_t byte2 = cpu_read(nes->programCounter - 1);
    uint16_t tempAddress = cpu_read((byte2 + nes->xRegister) & 0xFF);
    return ( cpu_read((byte2 + nes->xRegister + 1) & 0xFF)  << 8)  | tempAddress;
}

uint16_t idy(struct NES *nes) {
    nes->programCounter += 2;
    uint8_t byte2 = cpu_read(nes->programCounter - 1);
    uint16_t tempAddress = ((uint16_t)cpu_read((byte2 + 1) & 0x00FF) << 8);
    uint16_t operandAddress = cpu_read(byte2) + nes->yRegister;
    operandAddress += tempAddress;
    nes->oopsCycle = ( tempAddress != (operandAddress & 0xFF00) ) ? 1 : 0;
    return operandAddress;
}


/* -------------------
    Bitwise Operations 
    ------------------ */
void and(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    nes->accumulatorRegister &= cpu_read(mode(nes));
    nes->statusRegister.z = (nes->accumulatorRegister == 0) ? 1 : 0;
    nes->statusRegister.n = (nes->accumulatorRegister & 0x80) ? 1 : 0;
    nes->oopsCycle++;
}

void asl(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    nes->oopsCycle++;
    if (mode == &acc) {
        mode(nes);
        nes->statusRegister.c = ( (nes->accumulatorRegister & 0x80) ? 1 : 0);
        nes->statusRegister.n = ( (nes->accumulatorRegister & 0x40) ? 1 : 0);
        nes->statusRegister.z = ( (nes->accumulatorRegister & 0x7F) ? 0 : 1);
        nes->accumulatorRegister <<= 1;
    }
    else {
        uint16_t address = mode(nes);
        uint8_t operand = cpu_read(address);
        nes->statusRegister.c = ( (operand & 0x80) ? 1 : 0);
        nes->statusRegister.n = ( (operand & 0x40) ? 1 : 0);
        nes->statusRegister.z = ( (operand & 0x7F) ? 0 : 1);
        operand <<= 1;
        cpu_write(address, operand);
    }
}

void eor(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    nes->accumulatorRegister ^= cpu_read(mode(nes));
    nes->statusRegister.z = (nes->accumulatorRegister == 0) ? 1 : 0;
    nes->statusRegister.n = (nes->accumulatorRegister & 0x80) ? 1 : 0;
    nes->oopsCycle++;
}

void lsr(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    nes->oopsCycle++;
    if (mode == &acc) {
        mode(nes);
        nes->statusRegister.c = ( (nes->accumulatorRegister & 0x01) ? 1 : 0);
        nes->statusRegister.n = 0;
        nes->statusRegister.z = ( (nes->accumulatorRegister & 0xFE) ? 0 : 1);
        nes->accumulatorRegister >>= 1;
    }
    else {
        uint16_t address = mode(nes);
        uint8_t operand = cpu_read(address);
        nes->statusRegister.c = ( (operand & 0x01) ? 1 : 0);
        nes->statusRegister.n = 0;
        nes->statusRegister.z = ( (operand & 0xFE) ? 0 : 1);
        operand >>= 1;
        cpu_write(address, operand);
    }
}

void ora(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    nes->accumulatorRegister |= cpu_read(mode(nes));
    nes->statusRegister.z = (nes->accumulatorRegister == 0) ? 1 : 0;
    nes->statusRegister.n = (nes->accumulatorRegister & 0x80) ? 1 : 0;
    nes->oopsCycle++;
}

void rol(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    if (mode == &acc) {
        mode(nes);
        int rolledBit = nes->statusRegister.c;
        nes->statusRegister.n = ( (nes->accumulatorRegister & 0x40) ? 1 : 0);
        nes->statusRegister.c = ( (nes->accumulatorRegister & 0x80) ? 1 : 0);
        nes->accumulatorRegister = ((nes->accumulatorRegister << 1) | rolledBit);
        nes->statusRegister.z = ( (nes->accumulatorRegister == 0) ? 1 : 0);
    }
    else {
        int rolledBit = nes->statusRegister.c;
        uint16_t address = mode(nes);
        uint8_t operand = cpu_read(address);
        nes->statusRegister.n = ( (operand & 0x40) ? 1 : 0);
        nes->statusRegister.c = ( (operand & 0x80) ? 1 : 0);
        operand = ((operand << 1) | rolledBit);
        nes->statusRegister.z = ( (operand == 0) ? 1 : 0);
        cpu_write(address, operand);
    }
} 

void ror(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    if (mode == &acc) {
        mode(nes);
        int rolledBit = nes->statusRegister.c;
        nes->statusRegister.n = nes->statusRegister.c;
        nes->statusRegister.c = ( (nes->accumulatorRegister & 0x01) ? 1 : 0);
        nes->accumulatorRegister = ( (rolledBit != 0) ? ((nes->accumulatorRegister >> 1) | 0x80) : nes->accumulatorRegister >> 1) ;
        nes->statusRegister.z = ( (nes->accumulatorRegister == 0) ? 1 : 0);
    }
    else {
        int rolledBit = nes->statusRegister.c;
        uint16_t address = mode(nes);
        uint8_t operand = cpu_read(address);
        nes->statusRegister.n = nes->statusRegister.c;
        nes->statusRegister.c = ( (operand & 0x01) ? 1 : 0);
        operand = ( (rolledBit != 0) ? ((operand >> 1) | 0x80) : operand >> 1) ;
        nes->statusRegister.z = ( (operand == 0) ? 1 : 0);
        cpu_write(address, operand);
    }
} 


/* ------------------
    Branch Operations 
    ----------------- */
void bpl(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    if (!nes->statusRegister.n) {
        nes->programCounter = rel(nes);
        nes->branchCycle = 1;
        nes->oopsCycle++;
    }
}

void bmi(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    if (nes->statusRegister.n) {
        nes->programCounter = rel(nes);
        nes->branchCycle = 1;
        nes->oopsCycle++;
    }
}

void bvc(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    if (!nes->statusRegister.v) {
        nes->programCounter = rel(nes);
        nes->branchCycle = 1;
        nes->oopsCycle++;
    }
}

void bvs(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    if (nes->statusRegister.v) {
        nes->programCounter = rel(nes);
        nes->branchCycle = 1;
        nes->oopsCycle++;
    }
}

void bcc(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    if (!nes->statusRegister.c) {
        nes->programCounter = rel(nes);
        nes->branchCycle = 1;
        nes->oopsCycle++;
    }
}

void bcs(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    if (nes->statusRegister.c) {
        nes->programCounter = rel(nes);
        nes->branchCycle = 1;
        nes->oopsCycle++;
    }
}

void bne(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    if (!nes->statusRegister.z) {
        nes->programCounter = rel(nes);
        nes->branchCycle = 1;
        nes->oopsCycle++;
    }
}

void beq(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    if (nes->statusRegister.z) {
        nes->programCounter = rel(nes);
        nes->branchCycle = 1;
        nes->oopsCycle++;
    }
}


/* ----------------------
    Comparison Operations 
    --------------------- */
void cmp(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    uint8_t operand = cpu_read(mode(nes));
    nes->statusRegister.c = ( (nes->accumulatorRegister >= operand) ? 1 : 0);
    operand = nes->accumulatorRegister - operand;
    nes->statusRegister.n = ( (operand & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (operand == 0) ? 1 : 0);
    nes->oopsCycle++;
}

void bit(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    uint8_t operand = cpu_read(mode(nes));
    nes->statusRegister.v = ( (operand & 0x40) ? 1 : 0);
    nes->statusRegister.n = ( (operand & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( ((operand & nes->accumulatorRegister) == 0) ? 1 : 0);
}

void cpx(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    uint8_t operand = cpu_read(mode(nes));
    nes->statusRegister.c = ( (nes->xRegister >= operand) ? 1 : 0);
    operand = nes->xRegister - operand;
    nes->statusRegister.n = ( (operand & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (operand == 0) ? 1 : 0);
}

void cpy(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    uint8_t operand = cpu_read(mode(nes));
    nes->statusRegister.c = ( (nes->yRegister >= operand) ? 1 : 0);
    operand = nes->yRegister - operand;
    nes->statusRegister.n = ( (operand & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (operand == 0) ? 1 : 0);
}


/* ----------------
    Flag Operations
    --------------- */
void clc(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->statusRegister.c = 0;
}    

void cld(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->statusRegister.d = 0;
}    

void sec(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->statusRegister.c = 1;
}    

void sed(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->statusRegister.d = 1;
}    

void cli(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->statusRegister.i = 0;
}

void clv(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->statusRegister.v = 0;
}

void sei(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->statusRegister.i = 1;
}


/* ----------------
    Jump Operations
    --------------- */
void jmp(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    nes->programCounter = mode(nes);
}

void jsr(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    nes->programCounter--;
    uint8_t temp = (nes->programCounter >> 8);
    cpu_write(nes->stackPointer + 0x0100, temp);
    nes->stackPointer--;
    temp = nes->programCounter;
    cpu_write(nes->stackPointer + 0x0100, temp);
    nes->stackPointer--;
    nes->programCounter = mode(nes);
}

void rts(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->stackPointer++;
    nes->programCounter = cpu_read(nes->stackPointer + 0x0100);
    nes->stackPointer++;
    nes->programCounter |= (cpu_read(nes->stackPointer + 0x0100) << 8);
    nes->programCounter++;
}

void rti(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->stackPointer++;
    nes->statusRegister.reg = cpu_read(nes->stackPointer + 0x0100);
    nes->stackPointer++;
    nes->programCounter = cpu_read(nes->stackPointer + 0x0100);
    nes->stackPointer++;
    nes->programCounter |= (cpu_read(nes->stackPointer + 0x0100) << 8);
    nes->statusRegister.b = 0;
    nes->statusRegister.u = 1;
}


/* ----------------------
    Arithmetic Operations
    --------------------- */
void adc(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    uint8_t operand = cpu_read(mode(nes));
    uint16_t temp = nes->accumulatorRegister + operand + nes->statusRegister.c;
    nes->statusRegister.c = ( (temp > 0xFF) ? 1 : 0);
    nes->statusRegister.z = ( ((temp & 0x00FF)== 0) ? 1 : 0);
    nes->statusRegister.n = ( (temp & (1 << 7)) ? 1 : 0);
    nes->statusRegister.v = ( (((~(nes->accumulatorRegister ^ operand)) & (nes->accumulatorRegister ^ temp)) & (1 << 7)) ? 1 : 0);
    nes->accumulatorRegister = temp & 0xFF;
    nes->oopsCycle++;
}

void sbc(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    uint8_t operand = cpu_read(mode(nes));
    operand = operand ^ 0x00FF;
    uint16_t temp = nes->accumulatorRegister + operand + nes->statusRegister.c;
    nes->statusRegister.c = ( (temp > 0xFF) ? 1 : 0);
    nes->statusRegister.z = ( ((temp & 0x00FF)== 0) ? 1 : 0);
    nes->statusRegister.n = ( (temp & (1 << 7)) ? 1 : 0);
    nes->statusRegister.v = ( (((~(nes->accumulatorRegister ^ operand)) & (nes->accumulatorRegister ^ temp)) & (1 << 7)) ? 1 : 0);
    nes->accumulatorRegister = temp & 0xFF;
    nes->oopsCycle++;
}


/* ------------------
    Memory Operations
    ----------------- */
void lda(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    nes->accumulatorRegister = cpu_read(mode(nes));
    nes->statusRegister.n = ( (nes->accumulatorRegister & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (nes->accumulatorRegister == 0) ? 1 : 0);
    nes->oopsCycle++;
}

void sta(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    cpu_write(mode(nes), nes->accumulatorRegister);
}

void ldx(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    nes->xRegister = cpu_read(mode(nes));
    nes->statusRegister.n = ( (nes->xRegister & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (nes->xRegister == 0) ? 1 : 0);
    nes->oopsCycle++;
}

void stx(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    cpu_write(mode(nes), nes->xRegister);
}

void ldy(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    nes->yRegister = cpu_read(mode(nes));
    nes->statusRegister.n = ( (nes->yRegister & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (nes->yRegister == 0) ? 1 : 0);
    nes->oopsCycle++;
}

void sty(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    cpu_write(mode(nes), nes->yRegister);
}

void dec(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    uint16_t address = mode(nes);
    uint8_t operand = cpu_read(address);
    operand--;
    nes->statusRegister.n = ( (operand & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (operand == 0) ? 1 : 0);
    cpu_write(address, operand);
}

void inc(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    uint16_t address = mode(nes);
    uint8_t operand = cpu_read(address);
    operand++;
    nes->statusRegister.n = ( (operand & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (operand == 0) ? 1 : 0);
    cpu_write(address, operand);
}


/* --------------------
    Register Operations
    ------------------- */
void tax(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->xRegister = nes->accumulatorRegister;
    nes->statusRegister.n = ( (nes->xRegister & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (nes->xRegister == 0) ? 1 : 0);
}

void tay(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->yRegister = nes->accumulatorRegister;
    nes->statusRegister.n = ( (nes->yRegister & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (nes->yRegister == 0) ? 1 : 0);
}

void txa(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->accumulatorRegister = nes->xRegister;
    nes->statusRegister.n = ( (nes->accumulatorRegister & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (nes->accumulatorRegister == 0) ? 1 : 0);
}

void tya(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->accumulatorRegister = nes->yRegister;
    nes->statusRegister.n = ( (nes->accumulatorRegister & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (nes->accumulatorRegister == 0) ? 1 : 0);
}

void dex(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->xRegister--;
    nes->statusRegister.n = ( (nes->xRegister & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (nes->xRegister == 0) ? 1 : 0);
}

void dey(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->yRegister--;
    nes->statusRegister.n = ( (nes->yRegister & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (nes->yRegister == 0) ? 1 : 0);
}

void inx(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->xRegister++;
    nes->statusRegister.n = ( (nes->xRegister & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (nes->xRegister == 0) ? 1 : 0);
}

void iny(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->yRegister++;
    nes->statusRegister.n = ( (nes->yRegister & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (nes->yRegister == 0) ? 1 : 0);
}


/* -----------------
    Stack Operations
    ---------------- */
void pha(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    cpu_write(nes->stackPointer + 0x0100, nes->accumulatorRegister);
    nes->stackPointer--;
}

void php(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->statusRegister.u = 1;
    cpu_write(nes->stackPointer + 0x0100, nes->statusRegister.reg);
    nes->statusRegister.b = 0;
    nes->stackPointer--;
}

void txs(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->stackPointer = nes->xRegister;
}

void pla(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->stackPointer++;
    nes->accumulatorRegister = cpu_read(nes->stackPointer + 0x0100);
    nes->statusRegister.n = ( (nes->accumulatorRegister & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (nes->accumulatorRegister == 0) ? 1 : 0);
}

void tsx(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->xRegister = nes->stackPointer;
    nes->statusRegister.n = ( (nes->xRegister & 0x80) ? 1 : 0);
    nes->statusRegister.z = ( (nes->xRegister == 0) ? 1 : 0);
}

void plp(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->stackPointer++;
    nes->statusRegister.reg = cpu_read(nes->stackPointer + 0x0100);
    nes->statusRegister.u = 1;
    nes->statusRegister.b = 0;
}


/* -----------------
    Other Operations
    ---------------- */
void brk(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    imp(nes);
    nes->programCounter++;
    nes->statusRegister.i = 1;
    nes->statusRegister.b = 1;
    uint8_t temp = (nes->programCounter >> 8);
    cpu_write(nes->stackPointer + 0x0100, temp);
    nes->stackPointer--;
    temp = nes->programCounter;
    cpu_write(nes->stackPointer + 0x0100, temp);
    nes->stackPointer--;
    cpu_write(nes->stackPointer + 0x0100, nes->statusRegister.reg);
    nes->stackPointer--;
    nes->statusRegister.b = 0;
    nes->programCounter = (((uint16_t)cpu_read(0xFFFF) << 8) | cpu_read(0xFFFE));
}

void nop(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    mode(nes);
    return;
}

void nul(struct NES *nes, uint16_t (*mode)(struct NES*)) {
    mode(nes);
    return;
}


struct Instruction opcodes[256] = { 
    {"BRK", 0x00, 7, &imp, &brk}, {"ORA", 0x01, 6, &idx, &ora}, {"STP", 0x02, 0, &imp, &nul}, {"SLO", 0x03, 0, &idx, &nul}, {"NOP", 0x04, 0, &zpa, &nul}, {"ORA", 0x05, 3, &zpa, &ora}, {"ASL", 0x06, 5, &zpa, &asl}, {"SLO", 0x07, 0, &zpa, &nul}, {"PHP", 0x08, 3, &imp, &php}, {"ORA", 0x09, 2, &imm, &ora}, {"ASL", 0x0A, 2, &acc, &asl}, {"ANC", 0x0B, 0, &imm, &nul}, {"NOP", 0x0C, 0, &abl, &nul}, {"ORA", 0x0D, 4, &abl, &ora}, {"ASL", 0x0E, 6, &abl, &asl}, {"SLO", 0x0F, 0, &abl, &nul},
    {"BPL", 0x10, 2, &rel, &bpl}, {"ORA", 0x11, 5, &idy, &ora}, {"STP", 0x12, 0, &imp, &nul}, {"SLO", 0x13, 0, &idy, &nul}, {"NOP", 0x14, 0, &zpx, &nul}, {"ORA", 0x15, 4, &zpx, &ora}, {"ASL", 0x16, 6, &zpx, &asl}, {"SLO", 0x17, 0, &zpx, &nul}, {"CLC", 0x18, 2, &imp, &clc}, {"ORA", 0x19, 4, &aiy, &ora}, {"NOP", 0x1A, 0, &imp, &nul}, {"SLO", 0x1B, 0, &aiy, &nul}, {"NOP", 0x1C, 0, &aix, &nul}, {"ORA", 0x1D, 4, &aix, &ora}, {"ASL", 0x1E, 7, &aix, &asl}, {"SLO", 0x1F, 0, &aix, &nul},
    {"JSR", 0x20, 6, &abl, &jsr}, {"AND", 0x21, 6, &idx, &and}, {"STP", 0x22, 0, &imp, &nul}, {"RLA", 0x23, 0, &idx, &nul}, {"BIT", 0x24, 3, &zpa, &bit}, {"AND", 0x25, 3, &zpa, &and}, {"ROL", 0x26, 5, &zpa, &rol}, {"RLA", 0x27, 0, &zpa, &nul}, {"PLP", 0x28, 4, &imp, &plp}, {"AND", 0x29, 2, &imm, &and}, {"ROL", 0x2A, 2, &acc, &rol}, {"ANC", 0x2B, 0, &imm, &nul}, {"BIT", 0x2C, 4, &abl, &bit}, {"AND", 0x2D, 4, &abl, &and}, {"ROL", 0x2E, 6, &abl, &rol}, {"RLA", 0x2F, 0, &abl, &nul},
    {"BMI", 0x30, 2, &rel, &bmi}, {"AND", 0x31, 5, &idy, &and}, {"STP", 0x32, 0, &imp, &nul}, {"RLA", 0x33, 0, &idy, &nul}, {"NOP", 0x34, 0, &zpx, &nul}, {"AND", 0x35, 4, &zpx, &and}, {"ROL", 0x36, 6, &zpx, &rol}, {"RLA", 0x37, 0, &zpx, &nul}, {"SEC", 0x38, 2, &imp, &sec}, {"AND", 0x39, 4, &aiy, &and}, {"NOP", 0x3A, 0, &imp, &nul}, {"RLA", 0x3B, 0, &aiy, &nul}, {"NOP", 0x3C, 0, &aix, &nul}, {"AND", 0x3D, 4, &aix, &and}, {"ROL", 0x3E, 7, &aix, &rol}, {"RLA", 0x3F, 0, &aix, &nul},
    {"RTI", 0x40, 6, &imp, &rti}, {"EOR", 0x41, 6, &idx, &eor}, {"STP", 0x42, 0, &imp, &nul}, {"SRE", 0x43, 0, &idx, &nul}, {"NOP", 0x44, 0, &zpa, &nul}, {"EOR", 0x45, 3, &zpa, &eor}, {"LSR", 0x46, 5, &zpa, &lsr}, {"SRE", 0x47, 0, &zpa, &nul}, {"PHA", 0x48, 3, &imp, &pha}, {"EOR", 0x49, 2, &imm, &eor}, {"LSR", 0x4A, 2, &acc, &lsr}, {"ALR", 0x4B, 0, &imm, &nul}, {"JMP", 0x4C, 3, &abl, &jmp}, {"EOR", 0x4D, 4, &abl, &eor}, {"LSR", 0x4E, 6, &abl, &lsr}, {"SRE", 0x4F, 0, &abl, &nul},
    {"BVC", 0x50, 2, &rel, &bvc}, {"EOR", 0x51, 5, &idy, &eor}, {"STP", 0x52, 0, &imp, &nul}, {"SRE", 0x53, 0, &idy, &nul}, {"NOP", 0x54, 0, &zpx, &nul}, {"EOR", 0x55, 4, &zpx, &eor}, {"LSR", 0x56, 6, &zpx, &lsr}, {"SRE", 0x57, 0, &zpx, &nul}, {"CLI", 0x58, 2, &imp, &cli}, {"EOR", 0x59, 4, &aiy, &eor}, {"NOP", 0x5A, 0, &imp, &nul}, {"SRE", 0x5B, 0, &aiy, &nul}, {"NOP", 0x5C, 0, &aix, &nul}, {"EOR", 0x5D, 4, &aix, &eor}, {"LSR", 0x5E, 7, &aix, &lsr}, {"SRE", 0x5F, 0, &aix, &nul},
    {"RTS", 0x60, 6, &imp, &rts}, {"ADC", 0x61, 6, &idx, &adc}, {"STP", 0x62, 0, &imp, &nul}, {"RRA", 0x63, 0, &idx, &nul}, {"NOP", 0x64, 0, &zpa, &nul}, {"ADC", 0x65, 3, &zpa, &adc}, {"ROR", 0x66, 5, &zpa, &ror}, {"RRA", 0x67, 0, &zpa, &nul}, {"PLA", 0x68, 4, &imp, &pla}, {"ADC", 0x69, 2, &imm, &adc}, {"ROR", 0x6A, 2, &acc, &ror}, {"ARR", 0x6B, 0, &imm, &nul}, {"JMP", 0x6C, 3, &ind, &jmp}, {"ADC", 0x6D, 4, &abl, &adc}, {"ROR", 0x6E, 6, &abl, &ror}, {"RRA", 0x6F, 0, &abl, &nul},
    {"BVS", 0x70, 2, &rel, &bvs}, {"ADC", 0x71, 5, &idy, &adc}, {"STP", 0x72, 0, &imp, &nul}, {"RRA", 0x73, 0, &idy, &nul}, {"NOP", 0x74, 0, &zpx, &nul}, {"ADC", 0x75, 4, &zpx, &adc}, {"ROR", 0x76, 6, &zpx, &ror}, {"RRA", 0x77, 0, &zpx, &nul}, {"SEI", 0x78, 2, &imp, &sei}, {"ADC", 0x79, 4, &aiy, &adc}, {"NOP", 0x7A, 0, &imp, &nul}, {"RRA", 0x7B, 0, &aiy, &nul}, {"NOP", 0x7C, 0, &aix, &nul}, {"ADC", 0x7D, 4, &aix, &adc}, {"ROR", 0x7E, 7, &aix, &ror}, {"RRA", 0x7F, 0, &aix, &nul},
    {"NOP", 0x80, 0, &imm, &nul}, {"STA", 0x81, 6, &idx, &sta}, {"NOP", 0x82, 0, &imm, &nul}, {"SAX", 0x83, 0, &idx, &nul}, {"STY", 0x84, 3, &zpa, &sty}, {"STA", 0x85, 3, &zpa, &sta}, {"STX", 0x86, 3, &zpa, &stx}, {"SAX", 0x87, 0, &zpa, &nul}, {"DEY", 0x88, 2, &imp, &dey}, {"NOP", 0x89, 0, &imm, &nul}, {"TXA", 0x8A, 2, &imp, &txa}, {"ANE", 0x8B, 0, &imm, &nul}, {"STY", 0x8C, 4, &abl, &sty}, {"STA", 0x8D, 4, &abl, &sta}, {"STX", 0x8E, 4, &abl, &stx}, {"SAX", 0x8F, 0, &abl, &nul},
    {"BCC", 0x90, 2, &rel, &bcc}, {"STA", 0x91, 6, &idy, &sta}, {"STP", 0x92, 0, &imp, &nul}, {"SHA", 0x93, 0, &idy, &nul}, {"STY", 0x94, 4, &zpx, &sty}, {"STA", 0x95, 4, &zpx, &sta}, {"STX", 0x96, 4, &zpy, &stx}, {"SAX", 0x97, 0, &zpy, &nul}, {"TYA", 0x98, 2, &imp, &tya}, {"STA", 0x99, 5, &aiy, &sta}, {"TXS", 0x9A, 2, &imp, &txs}, {"TAS", 0x9B, 0, &aiy, &nul}, {"SHY", 0x9C, 0, &aix, &nul}, {"STA", 0x9D, 5, &aix, &sta}, {"SHX", 0x9E, 0, &aiy, &nul}, {"SHA", 0x9F, 0, &aix, &nul},
    {"LDY", 0xA0, 2, &imm, &ldy}, {"LDA", 0xA1, 6, &idx, &lda}, {"LDX", 0xA2, 2, &imm, &ldx}, {"LAX", 0xA3, 0, &idx, &nul}, {"LDY", 0xA4, 3, &zpa, &ldy}, {"LDA", 0xA5, 3, &zpa, &lda}, {"LDX", 0xA6, 3, &zpa, &ldx}, {"LAX", 0xA7, 0, &zpa, &nul}, {"TAY", 0xA8, 2, &imp, &tay}, {"LDA", 0xA9, 2, &imm, &lda}, {"TAX", 0xAA, 2, &imp, &tax}, {"LXA", 0xAB, 0, &imm, &nul}, {"LDY", 0xAC, 4, &abl, &ldy}, {"LDA", 0xAD, 4, &abl, &lda}, {"LDX", 0xAE, 4, &abl, &ldx}, {"LAX", 0xAF, 0, &abl, &nul},
    {"BCS", 0xB0, 2, &rel, &bcs}, {"LDA", 0xB1, 5, &idy, &lda}, {"STP", 0xB2, 0, &imp, &nul}, {"LAX", 0xB3, 0, &idy, &nul}, {"LDY", 0xB4, 4, &zpx, &ldy}, {"LDA", 0xB5, 4, &zpx, &lda}, {"LDX", 0xB6, 4, &zpy, &ldx}, {"LAX", 0xB7, 0, &zpy, &nul}, {"CLV", 0xB8, 2, &imp, &clv}, {"LDA", 0xB9, 4, &aiy, &lda}, {"TSX", 0xBA, 2, &imp, &tsx}, {"LAS", 0xBB, 0, &aiy, &nul}, {"LDY", 0xBC, 4, &aix, &ldy}, {"LDA", 0xBD, 4, &aix, &lda}, {"LDX", 0xBE, 4, &aiy, &ldx}, {"LAX", 0xBF, 0, &aiy, &nul},
    {"CPY", 0xC0, 2, &imm, &cpy}, {"CMP", 0xC1, 6, &idx, &cmp}, {"NOP", 0xC2, 0, &imm, &nul}, {"DCP", 0xC3, 0, &idx, &nul}, {"CPY", 0xC4, 3, &zpa, &cpy}, {"CMP", 0xC5, 3, &zpa, &cmp}, {"DEC", 0xC6, 5, &zpa, &dec}, {"DCP", 0xC7, 0, &zpa, &nul}, {"INY", 0xC8, 2, &imp, &iny}, {"CMP", 0xC9, 2, &imm, &cmp}, {"DEX", 0xCA, 2, &imp, &dex}, {"SBX", 0xCB, 0, &imm, &nul}, {"CPY", 0xCC, 4, &abl, &cpy}, {"CMP", 0xCD, 4, &abl, &cmp}, {"DEC", 0xCE, 6, &abl, &dec}, {"DCP", 0xCF, 0, &abl, &nul},
    {"BNE", 0xD0, 2, &rel, &bne}, {"CMP", 0xD1, 5, &idy, &cmp}, {"STP", 0xD2, 0, &imp, &nul}, {"DCP", 0xD3, 0, &idy, &nul}, {"NOP", 0xD4, 0, &zpx, &nul}, {"CMP", 0xD5, 4, &zpx, &cmp}, {"DEC", 0xD6, 6, &zpx, &dec}, {"DCP", 0xD7, 0, &zpx, &nul}, {"CLD", 0xD8, 2, &imp, &cld}, {"CMP", 0xD9, 4, &aiy, &cmp}, {"NOP", 0xDA, 0, &imp, &nul}, {"DCP", 0xDB, 0, &aiy, &nul}, {"NOP", 0xDC, 0, &aix, &nul}, {"CMP", 0xDD, 4, &aix, &cmp}, {"DEC", 0xDE, 7, &aix, &dec}, {"DCP", 0xDF, 0, &aix, &nul},
    {"CPX", 0xE0, 2, &imm, &cpx}, {"SBC", 0xE1, 6, &idx, &sbc}, {"NOP", 0xE2, 0, &imm, &nul}, {"ISC", 0xE3, 0, &idx, &nul}, {"CPX", 0xE4, 3, &zpa, &cpx}, {"SBC", 0xE5, 3, &zpa, &sbc}, {"INC", 0xE6, 5, &zpa, &inc}, {"ISC", 0xE7, 0, &zpa, &nul}, {"INX", 0xE8, 2, &imp, &inx}, {"SBC", 0xE9, 2, &imm, &sbc}, {"NOP", 0xEA, 2, &imp, &nop}, {"SBC", 0xEB, 0, &imm, &nul}, {"CPX", 0xEC, 4, &abl, &cpx}, {"SBC", 0xED, 4, &abl, &sbc}, {"INC", 0xEE, 6, &abl, &inc}, {"ISC", 0xEF, 0, &abl, &nul},
    {"BEQ", 0xF0, 2, &rel, &beq}, {"SBC", 0xF1, 5, &idy, &sbc}, {"STP", 0xF2, 0, &imp, &nul}, {"ISC", 0xF3, 0, &idy, &nul}, {"NOP", 0xF4, 0, &zpx, &nul}, {"SBC", 0xF5, 4, &zpx, &sbc}, {"INC", 0xF6, 6, &zpx, &inc}, {"ISC", 0xF7, 0, &zpx, &nul}, {"SED", 0xF8, 2, &imp, &sed}, {"SBC", 0xF9, 4, &aiy, &sbc}, {"NOP", 0xFA, 0, &imp, &nul}, {"ISC", 0xFB, 0, &aiy, &nul}, {"NOP", 0xFC, 0, &aix, &nul}, {"SBC", 0xFD, 4, &aix, &sbc}, {"INC", 0xFE, 7, &aix, &inc}, {"ISC", 0xFF, 0, &aix, &nul}
};


/*  ---------
    Main loop
    --------- */
void cpu_execute (struct NES *nes) {
    nes->programCounter = (((uint16_t)cpu_read(0xFFFD)) << 8) | cpu_read(0xFFFC);
    struct Instruction currentOp;
    uint8_t opcode = cpu_read(nes->programCounter);

    while (1) {
        currentOp = opcodes[opcode];
        currentOp.op(nes, currentOp.mode);

        int cycles = ((nes->oopsCycle == 2) ? 1 : 0);
        cycles += currentOp.cycles;
        cycles += nes->branchCycle;
        while (cycles > 0) {
            
        }
        nes->oopsCycle = 0;
        nes->branchCycle = 0;

        if (nes->pendingNMI) {

        }
        else if (nes->pendingIRQ) {

        }
    }
}