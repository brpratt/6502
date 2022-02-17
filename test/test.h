#ifndef __TEST_H__
#define __TEST_H__

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "bus.h"
#include "cpu.h"


#define COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

void test_init(const char *name);
void test_load_ram(const uint8_t *data, uint16_t data_n);
void test_load_rom(const uint8_t *data, uint16_t data_n);

const struct bus * test_bus(void);

#define TEST_RAM_SIZE   0x1000
#define TEST_RAM_OFFSET 0x0000
#define TEST_ROM_SIZE   0x1000
#define TEST_ROM_OFFSET 0xF000

#define TEST_INIT() test_init(__FILE__)

#define TEST(fn) \
    do { \
        printf("%s...", #fn); \
        fn(); \
        printf("ok\n"); \
    } while (0)

#endif
