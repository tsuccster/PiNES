#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

uint8_t cpu_read(uint16_t address);
void cpu_write(uint16_t address, uint8_t data);

#endif