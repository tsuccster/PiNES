#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

union StatusReg {
    struct {
        uint8_t c : 1;
        uint8_t z : 1;
        uint8_t i : 1;
        uint8_t d : 1;
        uint8_t b : 1;
        uint8_t u : 1;
        uint8_t v : 1;
        uint8_t n : 1;
    };
    uint8_t reg;
};

struct NES {
    uint8_t xRegister;
    uint8_t yRegister;
    union StatusReg statusRegister;
    uint8_t accumulatorRegister;
    uint8_t stackPointer;
    uint16_t programCounter;

    int branchCycle;
    int oopsCycle;
    int dmaCycles;
    int pendingNMI;
    int pendingIRQ;

    int scanline;
    int cycle;

    void *surface;
};

#endif