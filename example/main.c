#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "cpu.h"

const char * banner =
"#################\n"
"#     COMPY     #\n"
"#################\n\n";

static struct memory {
    uint8_t ram[0x8000];
    uint8_t rom[0x8000];
} memory;

static uint8_t peek(void *inst, uint16_t addr) {
    struct memory *mem = (struct memory *)inst;

    if (addr < 0x8000) {
        return mem->ram[addr];
    } else {
        return mem->rom[addr - 0x8000];
    }
}

static void poke(void *inst, uint16_t addr, uint8_t data) {
    struct memory *mem = (struct memory *)inst;

    if (addr < 0x8000) {
        mem->ram[addr] = data;
    }
}

static size_t load(FILE *bin) {
    uint8_t binbuf[0x10000];

    size_t bytes_read = fread(binbuf, 1, 0x10000, bin);

    if (bytes_read < 0x8000) {
        memcpy(memory.ram, binbuf, bytes_read);
        return bytes_read;
    }

    memcpy(memory.ram, binbuf, 0x8000);
    memcpy(memory.rom, binbuf + 0x8000, bytes_read - 0x8000);
    return bytes_read;
}

int main(int argc, char *argv[]) {
    struct bus bus = {
        .inst = &memory,
        .peek = peek,
        .poke = poke
    };

    if (argc != 2) {
        printf("Usage: compy <program>\n");
        return 1;
    }

    FILE *bin = fopen(argv[1], "r");
    if (bin == NULL) {
        printf("ERROR: unable to open %s\n", argv[1]);
        return 1;
    }

    size_t bytes_read = load(bin);
    fclose(bin);

    if (bytes_read != 0x10000) {
        printf("ERROR: program file is not 0x10000 bytes\n");
        return 1;
    }

    uint16_t pc_start = bus_peek(&bus, 0xFFFD);
    pc_start = pc_start << 8 | bus_peek(&bus, 0xFFFC);

    struct cpu cpu;
    cpu_init(&cpu, pc_start);

    printf(banner);

    uint16_t prev_pc;
    do {
        prev_pc = cpu.pc;
        cpu_step(&cpu, &bus);
    } while (prev_pc != cpu.pc);

    printf("Result of computation is: %d\n", memory.ram[0]);

    return 0;
}
