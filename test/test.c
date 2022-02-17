#include "test.h"

static struct memory {
    uint8_t ram[TEST_RAM_SIZE];
    uint8_t rom[TEST_ROM_SIZE];
} memory;

/*
    Disabling -Wtype-limits because the offsets may be 0, resulting in an
    expression that is always true (unsigned comparison against 0).

    I don't want to remove the redundant comparisons in case the offsets change
*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
static uint8_t peek(void *inst, uint16_t addr) {
    struct memory *mem = (struct memory *)inst;

    if (addr >= TEST_RAM_OFFSET && addr < (TEST_RAM_OFFSET + TEST_RAM_SIZE)) {
        return mem->ram[addr - TEST_RAM_OFFSET];
    }

    if (addr >= TEST_ROM_OFFSET && addr < (TEST_ROM_OFFSET + TEST_ROM_SIZE)) {
        return mem->rom[addr - TEST_ROM_OFFSET];
    }

    return 0;
}

static void poke(void *inst, uint16_t addr, uint8_t data) {
    struct memory *mem = (struct memory *)inst;

    if (addr >= TEST_RAM_OFFSET && addr < (TEST_RAM_OFFSET + TEST_RAM_SIZE)) {
        mem->ram[addr - TEST_RAM_OFFSET] = data;
    }
}
#pragma GCC diagnostic pop

struct bus _bus = {
    .inst = &memory,
    .peek = peek,
    .poke = poke
};

void test_init(const char *name) {
    memset(memory.ram, 0, sizeof(memory.ram));
    memset(memory.rom, 0, sizeof(memory.rom));

    printf("*** %s ***\n", name);
}

void test_load_ram(const uint8_t *data, uint16_t data_n) {
    memcpy(memory.ram, data, data_n);
}

void test_load_rom(const uint8_t *data, uint16_t data_n) {
    memcpy(memory.rom, data, data_n);
}

const struct bus * test_bus(void) {
    return &_bus;
}
