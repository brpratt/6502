#ifndef __CPU_H__
#define __CPU_H__

#include <stdint.h>

#include "bus.h"

#define INTR_RESET (1 << 0)
#define INTR_NMI   (1 << 1)
#define INTR_IRQ   (1 << 2)

struct cpu {
    uint16_t pc;
    uint8_t sp;
    uint8_t p;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t cycle;
    uint8_t opc;
    uint8_t opr1;
    uint8_t opr2;
    uint8_t intr;
    uint16_t ea;
};

#define P_N (1 << 7)
#define P_V (1 << 6)
#define P_5 (1 << 5)
#define P_B (1 << 4)
#define P_D (1 << 3)
#define P_I (1 << 2)
#define P_Z (1 << 1)
#define P_C (1 << 0)

void cpu_init(struct cpu *cpu, uint16_t pc);
void cpu_tick(struct cpu *cpu, const struct bus *bus);
void cpu_step(struct cpu *cpu, const struct bus *bus);

#endif
