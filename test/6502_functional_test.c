#include "test.h"

static uint8_t peek(void *inst, uint16_t addr) {
    uint8_t *mem = (uint8_t *)inst;

    return mem[addr];
}

static void poke(void *inst, uint16_t addr, uint8_t data) {
    uint8_t *mem = (uint8_t *)inst;

    mem[addr] = data;
}

int main(void) {
    TEST_INIT();

    uint8_t memory[0x10000];

    struct bus bus = {
        .inst = memory,
        .peek = peek,
        .poke = poke
    };

    FILE *bin = fopen("./test/6502_functional_test/6502_functional_test.bin", "r");
    if (bin == NULL) {
        printf("FAIL unable to open file\n");
        return 1;
    }

    size_t bytes_read = fread(memory, 1, 0x10000, bin);
    if (bytes_read != 65536) {
        printf("FAIL read 0x%04lX bytes out of expected 64K\n", bytes_read);
        return 1;
    }

    fclose(bin);

    struct cpu cpu;
    cpu_init(&cpu, 0x0400);

    uint16_t prev_pc;
    do {
        prev_pc = cpu.pc;
        cpu_step(&cpu, &bus);
    } while (prev_pc != cpu.pc);

    if (cpu.pc == 0x3469) {
        printf("PASS\n");
    } else {
        printf("FAIL at 0x%04X\n", cpu.pc);
    }

    return 0;
}
