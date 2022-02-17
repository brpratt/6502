#include "test.h"

void test_ora_idx(void) {
    uint8_t program[] = { 0x01, 0x02 };

    uint8_t data[] = {
        0x00, 0x00, 0x00, 0x00,
        TEST_ROM_OFFSET & 0xFF,
        (TEST_ROM_OFFSET >> 8) & 0xFF,
        (TEST_ROM_OFFSET + 2) & 0xFF,
        (TEST_ROM_OFFSET >> 8) & 0xFF,
    };

    const struct bus *bus = test_bus();

    test_load_rom(program, sizeof(program));
    test_load_ram(data, sizeof(data));

    struct cpu cpu;

    struct {
        uint8_t a;
        uint8_t x;
        uint8_t expected_a;
        uint8_t expected_p;
    } tests[] = {
        {
            .a = 0xF0,
            .x = 0x02,
            .expected_a = 0xF1,
            .expected_p = P_N,
        },
        {
            .a = 0x00,
            .x = 0x04,
            .expected_a = 0x00,
            .expected_p = P_Z,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.a = tests[i].a;
        cpu.x = tests[i].x;

        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);

        assert(cpu.cycle == 0);
        assert(cpu.a == tests[i].expected_a);
        assert(cpu.p == tests[i].expected_p);
    }
}

void test_ora_zpg(void) {
    uint8_t data[] = { 0x00, 0x01 };

    const struct bus *bus = test_bus();

    test_load_ram(data, sizeof(data));

    struct cpu cpu;

    struct {
        uint8_t prg[2];
        uint8_t a;
        uint8_t expected_a;
        uint8_t expected_p;
    } tests[] = {
        {
            .prg = { 0x05, 0x01 },
            .a = 0xF0,
            .expected_a = 0xF1,
            .expected_p = P_N,
        },
        {
            .prg = { 0x05, 0x00 },
            .a = 0x00,
            .expected_a = 0x00,
            .expected_p = P_Z,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        test_load_rom(tests[i].prg, 2);
        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.a = tests[i].a;

        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);

        assert(cpu.cycle == 0);
        assert(cpu.a == tests[i].expected_a);
        assert(cpu.p == tests[i].expected_p);
    }
}

void test_ora_zpx(void) {
    uint8_t data[] = { 0x00, 0x01, 0x02, 0x03 };

    const struct bus *bus = test_bus();

    test_load_ram(data, sizeof(data));

    struct cpu cpu;

    struct {
        uint8_t prg[2];
        uint8_t a;
        uint8_t x;
        uint8_t expected_a;
        uint8_t expected_p;
    } tests[] = {
        {
            .prg = { 0x15, 0x01 },
            .a = 0xF0,
            .x = 0x02,
            .expected_a = 0xF3,
            .expected_p = P_N,
        },
        {
            .prg = { 0x15, 0x00 },
            .a = 0x00,
            .x = 0x00,
            .expected_a = 0x00,
            .expected_p = P_Z,
        },
        {
            .prg = { 0x15, 0xF0 },
            .a = 0x20,
            .x = 0x12,
            .expected_a = 0x22,
            .expected_p = 0,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        test_load_rom(tests[i].prg, 2);
        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.a = tests[i].a;
        cpu.x = tests[i].x;

        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);

        assert(cpu.cycle == 0);
        assert(cpu.a == tests[i].expected_a);
        assert(cpu.p == tests[i].expected_p);
    }
}

void test_ora_abl(void) {
    uint8_t data[] = { 0x00, 0x01 };

    test_load_ram(data, sizeof(data));

    const struct bus *bus = test_bus();

    struct cpu cpu;

    struct {
        uint8_t prg[3];
        uint8_t a;
        uint8_t expected_a;
        uint8_t expected_p;
    } tests[] = {
        {
            .prg = { 0x0D, 0x01, 0x00 },
            .a = 0xF0,
            .expected_a = 0xF1,
            .expected_p = P_N,
        },
        {
            .prg = { 0x0D, 0x00, 0x00 },
            .a = 0x00,
            .expected_a = 0x00,
            .expected_p = P_Z,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        test_load_rom(tests[i].prg, 3);
        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.a = tests[i].a;

        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);

        assert(cpu.cycle == 0);
        assert(cpu.a == tests[i].expected_a);
        assert(cpu.p == tests[i].expected_p);
    }
}

void test_ora_abx(void) {
    uint8_t data[0x0200];

    data[0x00FE] = 0x03;
    data[0x00FF] = 0x01;
    data[0x0100] = 0x02;
    data[0x0101] = 0x00;

    test_load_ram(data, sizeof(data));

    const struct bus *bus = test_bus();

    struct cpu cpu;

    struct {
        uint8_t prg[3];
        uint8_t a;
        uint8_t x;
        int expected_ticks;
        uint8_t expected_a;
        uint8_t expected_p;
    } tests[] = {
        {
            .prg = { 0x1D, 0x01, 0x00 },
            .a = 0xF0,
            .x = 0xFE,
            .expected_ticks = 4,
            .expected_a = 0xF1,
            .expected_p = P_N,
        },
        {
            .prg = { 0x1D, 0x03, 0x00 },
            .a = 0x00,
            .x = 0xFE,
            .expected_ticks = 5,
            .expected_a = 0x00,
            .expected_p = P_Z,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        test_load_rom(tests[i].prg, 3);
        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.a = tests[i].a;
        cpu.x = tests[i].x;

        int ticks = 0;
        do {
            cpu_tick(&cpu, bus);
            ticks++;
        } while (cpu.cycle != 0);

        assert(ticks == tests[i].expected_ticks);
        assert(cpu.a == tests[i].expected_a);
        assert(cpu.p == tests[i].expected_p);
    }
}

void test_ora_aby(void) {
    uint8_t data[0x0200];

    data[0x00FE] = 0x03;
    data[0x00FF] = 0x01;
    data[0x0100] = 0x02;
    data[0x0101] = 0x00;

    test_load_ram(data, sizeof(data));

    const struct bus *bus = test_bus();

    struct cpu cpu;

    struct {
        uint8_t prg[3];
        uint8_t a;
        uint8_t y;
        int expected_ticks;
        uint8_t expected_a;
        uint8_t expected_p;
    } tests[] = {
        {
            .prg = { 0x19, 0x01, 0x00 },
            .a = 0xF0,
            .y = 0xFE,
            .expected_ticks = 4,
            .expected_a = 0xF1,
            .expected_p = P_N,
        },
        {
            .prg = { 0x19, 0x03, 0x00 },
            .a = 0x00,
            .y = 0xFE,
            .expected_ticks = 5,
            .expected_a = 0x00,
            .expected_p = P_Z,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        test_load_rom(tests[i].prg, 3);
        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.a = tests[i].a;
        cpu.y = tests[i].y;

        int ticks = 0;
        do {
            cpu_tick(&cpu, bus);
            ticks++;
        } while (cpu.cycle != 0);

        assert(ticks == tests[i].expected_ticks);
        assert(cpu.a == tests[i].expected_a);
        assert(cpu.p == tests[i].expected_p);
    }
}

void test_ora_idy(void) {
    uint8_t data[0x0200];

    data[0x0000] = 0x00;
    data[0x0001] = 0x00;
    data[0x0002] = 0xFC;
    data[0x0003] = 0x00;
    data[0x00FE] = 0x03;
    data[0x00FF] = 0x01;
    data[0x0100] = 0x02;
    data[0x0101] = 0x00;

    test_load_ram(data, sizeof(data));

    const struct bus *bus = test_bus();

    struct cpu cpu;

    struct {
        uint8_t prg[2];
        uint8_t a;
        uint8_t y;
        int expected_ticks;
        uint8_t expected_a;
        uint8_t expected_p;
    } tests[] = {
        {
            .prg = { 0x11, 0x02 },
            .a = 0xF0,
            .y = 0x03,
            .expected_ticks = 5,
            .expected_a = 0xF1,
            .expected_p = P_N,
        },
        {
            .prg = { 0x11, 0x02 },
            .a = 0x00,
            .y = 0x05,
            .expected_ticks = 6,
            .expected_a = 0x00,
            .expected_p = P_Z,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        test_load_rom(tests[i].prg, 2);
        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.a = tests[i].a;
        cpu.y = tests[i].y;

        int ticks = 0;
        do {
            cpu_tick(&cpu, bus);
            ticks++;
        } while (cpu.cycle != 0);

        assert(ticks == tests[i].expected_ticks);
        assert(cpu.a == tests[i].expected_a);
        assert(cpu.p == tests[i].expected_p);
    }
}

void test_ora_imm(void) {
    const struct bus *bus = test_bus();

    struct cpu cpu;

    struct {
        uint8_t prg[2];
        uint8_t a;
        uint8_t expected_a;
        uint8_t expected_p;
    } tests[] = {
        {
            .prg = { 0x09, 0x01 },
            .a = 0xF0,
            .expected_a = 0xF1,
            .expected_p = P_N,
        },
        {
            .prg = { 0x09, 0x00 },
            .a = 0x00,
            .expected_a = 0x00,
            .expected_p = P_Z,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        test_load_rom(tests[i].prg, 2);
        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.a = tests[i].a;

        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);

        assert(cpu.cycle == 0);
        assert(cpu.a == tests[i].expected_a);
        assert(cpu.p == tests[i].expected_p);
    }
}

void test_asl_acc(void) {
    uint8_t program[] = { 0x0A };

    test_load_rom(program, 1);

    const struct bus *bus = test_bus();

    struct cpu cpu;

    struct {
        uint8_t initial_a;
        uint8_t expected_a;
        uint8_t expected_p;
    } tests[] = {
        {
            .initial_a = 0x33,
            .expected_a = 0x66,
            .expected_p = 0,
        },
        {
            .initial_a = 0x70,
            .expected_a = 0xE0,
            .expected_p = P_N,
        },
        {
            .initial_a = 0x80,
            .expected_a = 0x00,
            .expected_p = P_C | P_Z,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.a = tests[i].initial_a;

        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);

        assert(cpu.cycle == 0);
        assert(cpu.a == tests[i].expected_a);
        assert(cpu.p == tests[i].expected_p);
    }
}

void test_asl_zpg(void) {
    uint8_t data[] = { 0x01, 0x70, 0x80 };

    const struct bus *bus = test_bus();

    test_load_ram(data, sizeof(data));

    struct cpu cpu;

    struct {
        uint8_t prg[2];
        uint8_t expected_data;
        uint8_t expected_p;
    } tests[] = {
        {
            .prg = { 0x06, 0x00 },
            .expected_data = 0x02,
            .expected_p = 0,
        },
        {
            .prg = { 0x06, 0x01 },
            .expected_data = 0xE0,
            .expected_p = P_N,
        },
        {
            .prg = { 0x06, 0x02 },
            .expected_data = 0x00,
            .expected_p = P_C | P_Z,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        test_load_rom(tests[i].prg, 2);
        cpu_init(&cpu, TEST_ROM_OFFSET);

        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);

        uint16_t addr = tests[i].prg[1];
        assert(bus_peek(bus, addr) == tests[i].expected_data);
        assert(cpu.cycle == 0);
        assert(cpu.p == tests[i].expected_p);
    }
}

void test_asl_zpx(void) {
    uint8_t data[] = { 0x00, 0x80, 0x02, 0x75 };

    const struct bus *bus = test_bus();

    test_load_ram(data, sizeof(data));

    struct cpu cpu;

    struct {
        uint16_t addr;
        uint8_t prg[2];
        uint8_t x;
        uint8_t expected_data;
        uint8_t expected_p;
    } tests[] = {
        {
            .addr = 0x0003,
            .prg = { 0x16, 0x01 },
            .x = 0x02,
            .expected_data = 0xEA,
            .expected_p = P_N,
        },
        {
            .addr = 0x0001,
            .prg = { 0x16, 0xF0 },
            .x = 0x11,
            .expected_data = 0x00,
            .expected_p = P_C | P_Z,
        },
        {
            .addr = 0x0002,
            .prg = { 0x16, 0x00 },
            .x = 0x02,
            .expected_data = 0x04,
            .expected_p = 0,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        test_load_rom(tests[i].prg, 2);
        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.x = tests[i].x;

        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);

        assert(cpu.cycle == 0);
        assert(bus_peek(bus, tests[i].addr) == tests[i].expected_data);
        assert(cpu.p == tests[i].expected_p);
    }
}

void test_asl_abl(void) {
    uint8_t data[0x0200];
    uint8_t program[] = { 0x0E, 0x80, 0x01 };

    data[0x0180] = 0x55;

    const struct bus *bus = test_bus();
    struct cpu cpu;

    test_load_ram(data, sizeof(data));
    test_load_rom(program, sizeof(program));
    cpu_init(&cpu, TEST_ROM_OFFSET);

    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);

    assert(cpu.cycle == 0);
    assert(bus_peek(bus, 0x0180) == 0xAA);
    assert(cpu.pc == TEST_ROM_OFFSET + 3);
    assert(cpu.p == P_N);
}

void test_asl_abx(void) {
    uint8_t data[0x0200];

    data[0x00FE] = 0x40;
    data[0x00FF] = 0x01;
    data[0x0100] = 0x02;
    data[0x0101] = 0x03;

    test_load_ram(data, sizeof(data));

    const struct bus *bus = test_bus();

    struct cpu cpu;

    struct {
        uint16_t addr;
        uint8_t prg[3];
        uint8_t x;
        uint8_t expected_data;
        uint8_t expected_p;
    } tests[] = {
        {
            .addr = 0x0100,
            .prg = { 0x1E, 0x10, 0x00 },
            .x = 0xF0,
            .expected_data = 0x04,
            .expected_p = 0,
        },
        {
            .addr = 0x00FE,
            .prg = { 0x1E, 0xF0, 0x00 },
            .x = 0x0E,
            .expected_data = 0x80,
            .expected_p = P_N,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        test_load_rom(tests[i].prg, 3);
        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.x = tests[i].x;

        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);

        assert(cpu.cycle == 0);
        assert(bus_peek(bus, tests[i].addr) == tests[i].expected_data);
        assert(cpu.p == tests[i].expected_p);
    }
}


void test_sta_abl(void) {
    uint8_t program[] = { 0x8D, 0x00, 0x02 };

    const struct bus *bus = test_bus();
    struct cpu cpu;

    test_load_rom(program, sizeof(program));
    cpu_init(&cpu, TEST_ROM_OFFSET);

    cpu.a = 0x12;
    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);

    assert(cpu.cycle == 0);
    assert(bus_peek(bus, 0x0200) == 0x12);
    assert(cpu.pc == TEST_ROM_OFFSET + 3);
}

void test_sta_zpg(void) {
    uint8_t program[] = { 0x85, 0x02 };

    const struct bus *bus = test_bus();
    struct cpu cpu;

    test_load_rom(program, sizeof(program));
    cpu_init(&cpu, TEST_ROM_OFFSET);

    cpu.a = 0x12;
    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);

    assert(cpu.cycle == 0);
    assert(bus_peek(bus, 0x0002) == 0x12);
    assert(cpu.pc == TEST_ROM_OFFSET + 2);
}

void test_sta_zpx(void) {
    const struct bus *bus = test_bus();

    struct cpu cpu;

    struct {
        uint16_t addr;
        uint8_t prg[2];
        uint8_t a;
        uint8_t x;
    } tests[] = {
        {
            .addr = 0x0004,
            .prg = { 0x95, 0x01 },
            .a = 0xF0,
            .x = 0x03,
        },
        {
            .addr = 0x0002,
            .prg = { 0x95, 0xF0 },
            .a = 0x55,
            .x = 0x12,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        test_load_rom(tests[i].prg, 2);
        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.a = tests[i].a;
        cpu.x = tests[i].x;

        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);

        assert(cpu.cycle == 0);
        assert(bus_peek(bus, tests[i].addr) == cpu.a);
    }
}

void test_sta_abx(void) {
    uint8_t data[0x0200];

    data[0x00FE] = 0x03;
    data[0x00FF] = 0x01;
    data[0x0100] = 0x02;
    data[0x0101] = 0x00;

    test_load_ram(data, sizeof(data));

    const struct bus *bus = test_bus();

    struct cpu cpu;

    struct {
        uint16_t addr;
        uint8_t prg[3];
        uint8_t a;
        uint8_t x;
    } tests[] = {
        {
            .addr = 0x0001,
            .prg = { 0x9D, 0x00, 0x00 },
            .a = 0xF0,
            .x = 0x01,
        },
        {
            .addr = 0x0140,
            .prg = { 0x9D, 0x20, 0x01 },
            .a = 0x55,
            .x = 0x20,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        test_load_rom(tests[i].prg, 3);
        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.a = tests[i].a;
        cpu.x = tests[i].x;

        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);

        assert(cpu.cycle == 0);
        assert(bus_peek(bus, tests[i].addr) == cpu.a);
    }
}

void test_sta_aby(void) {
    uint8_t data[0x0200];

    data[0x00FE] = 0x03;
    data[0x00FF] = 0x01;
    data[0x0100] = 0x02;
    data[0x0101] = 0x00;

    test_load_ram(data, sizeof(data));

    const struct bus *bus = test_bus();

    struct cpu cpu;

    struct {
        uint16_t addr;
        uint8_t prg[3];
        uint8_t a;
        uint8_t y;
    } tests[] = {
        {
            .addr = 0x0001,
            .prg = { 0x99, 0x00, 0x00 },
            .a = 0xF0,
            .y = 0x01,
        },
        {
            .addr = 0x0140,
            .prg = { 0x99, 0x20, 0x01 },
            .a = 0x55,
            .y = 0x20,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        test_load_rom(tests[i].prg, 3);
        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.a = tests[i].a;
        cpu.y = tests[i].y;

        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);

        assert(cpu.cycle == 0);
        assert(bus_peek(bus, tests[i].addr) == cpu.a);
    }
}

void test_sta_idx(void) {
    uint8_t program[] = { 0x81, 0x02 };

    uint8_t data[0x0200];

    data[0x0000] = 0x30;
    data[0x0001] = 0x01;
    data[0x0004] = 0x12;
    data[0x0005] = 0x01;

    const struct bus *bus = test_bus();

    test_load_rom(program, sizeof(program));
    test_load_ram(data, sizeof(data));

    struct cpu cpu;

    struct {
        uint16_t addr;
        uint8_t a;
        uint8_t x;
    } tests[] = {
        {
            .addr = 0x0112,
            .a = 0xF0,
            .x = 0x02,
        },
        {
            .addr = 0x0130,
            .a = 0x55,
            .x = 0xFE,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.a = tests[i].a;
        cpu.x = tests[i].x;

        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);

        assert(cpu.cycle == 0);
        assert(bus_peek(bus, tests[i].addr) == cpu.a);
    }
}

void test_sta_idy(void) {
    uint8_t program[] = { 0x91, 0x02 };

    uint8_t data[0x0200];

    data[0x0002] = 0x30;
    data[0x0003] = 0x01;

    const struct bus *bus = test_bus();

    test_load_rom(program, sizeof(program));
    test_load_ram(data, sizeof(data));

    struct cpu cpu;

    struct {
        uint16_t addr;
        uint8_t a;
        uint8_t y;
    } tests[] = {
        {
            .addr = 0x0134,
            .a = 0xF0,
            .y = 0x04,
        },
        {
            .addr = 0x0180,
            .a = 0x55,
            .y = 0x50,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.a = tests[i].a;
        cpu.y = tests[i].y;

        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);

        assert(cpu.cycle == 0);
        assert(bus_peek(bus, tests[i].addr) == cpu.a);
    }
}

void test_stx_abl(void) {
    uint8_t program[] = { 0x8E, 0x00, 0x02 };

    const struct bus *bus = test_bus();
    struct cpu cpu;

    test_load_rom(program, sizeof(program));
    cpu_init(&cpu, TEST_ROM_OFFSET);

    cpu.x = 0x12;
    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);

    assert(cpu.cycle == 0);
    assert(bus_peek(bus, 0x0200) == 0x12);
    assert(cpu.pc == TEST_ROM_OFFSET + 3);
}

void test_sty_abl(void) {
    uint8_t program[] = { 0x8C, 0x00, 0x02 };

    const struct bus *bus = test_bus();
    struct cpu cpu;

    test_load_rom(program, sizeof(program));
    cpu_init(&cpu, TEST_ROM_OFFSET);

    cpu.y = 0x12;
    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);

    assert(cpu.cycle == 0);
    assert(bus_peek(bus, 0x0200) == 0x12);
    assert(cpu.pc == TEST_ROM_OFFSET + 3);
}

void test_php(void) {
    uint8_t program[] = { 0x08 };

    const struct bus *bus = test_bus();
    struct cpu cpu;

    test_load_rom(program, sizeof(program));
    cpu_init(&cpu, TEST_ROM_OFFSET);

    cpu.p = P_C | P_N;

    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);


    assert(cpu.cycle == 0);
    assert(cpu.sp == 0xFE);
    assert(bus_peek(bus, 0x01FF) == (cpu.p | P_B | (1 << 5)));
}

void test_bpl(void) {
    uint8_t program[0x0200];
    const struct bus *bus = test_bus();
    struct cpu cpu;

    struct {
        uint16_t offset;
        uint8_t prg[2];
        uint8_t p;
        int ticks;
        uint16_t expected_pc;
    } tests[] = {
        {
            .offset = 0,
            .prg = { 0x10, 0x30 },
            .p = P_N,
            .ticks = 2,
            .expected_pc = TEST_ROM_OFFSET + 2,
        },
        {
            .offset = 0,
            .prg = { 0x10, 0x30 },
            .p = 0,
            .ticks = 3,
            .expected_pc = TEST_ROM_OFFSET + 0x32,
        },
        {
            .offset = 0x00F0,
            .prg = { 0x10, 0x30 },
            .p = 0,
            .ticks = 4,
            .expected_pc = TEST_ROM_OFFSET + 0x122,
        },
        {
            .offset = 0,
            .prg = { 0x10, 0xFE },
            .p = P_N,
            .ticks = 2,
            .expected_pc = TEST_ROM_OFFSET + 2,
        },
        {
            .offset = 0,
            .prg = { 0x10, 0xFE },
            .p = 0,
            .ticks = 3,
            .expected_pc = TEST_ROM_OFFSET,
        },
        {
            .offset = 0,
            .prg = { 0x10, 0xF0 },
            .p = 0,
            .ticks = 4,
            .expected_pc = TEST_ROM_OFFSET - 14,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        uint16_t offset = tests[i].offset;
        program[offset] = tests[i].prg[0];
        program[offset + 1] = tests[i].prg[1];
        test_load_rom(program, sizeof(program));

        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.pc += offset;
        cpu.p = tests[i].p;

        for (int tick = 0; tick < tests[i].ticks; tick++) {
            cpu_tick(&cpu, bus);
        }

        assert(cpu.cycle == 0);
        assert(cpu.pc == tests[i].expected_pc);
    }
}

void test_bne(void) {
    uint8_t program[0x0200];
    const struct bus *bus = test_bus();
    struct cpu cpu;

    struct {
        uint16_t offset;
        uint8_t prg[2];
        uint8_t p;
        int ticks;
        uint16_t expected_pc;
    } tests[] = {
        {
            .offset = 0,
            .prg = { 0xD0, 0x30 },
            .p = P_Z,
            .ticks = 2,
            .expected_pc = TEST_ROM_OFFSET + 2,
        },
        {
            .offset = 0,
            .prg = { 0xD0, 0x30 },
            .p = 0,
            .ticks = 3,
            .expected_pc = TEST_ROM_OFFSET + 0x32,
        },
        {
            .offset = 0x00F0,
            .prg = { 0xD0, 0x30 },
            .p = 0,
            .ticks = 4,
            .expected_pc = TEST_ROM_OFFSET + 0x122,
        },
        {
            .offset = 0,
            .prg = { 0xD0, 0xFE },
            .p = P_Z,
            .ticks = 2,
            .expected_pc = TEST_ROM_OFFSET + 2,
        },
        {
            .offset = 0,
            .prg = { 0xD0, 0xFE },
            .p = 0,
            .ticks = 3,
            .expected_pc = TEST_ROM_OFFSET,
        },
        {
            .offset = 0,
            .prg = { 0xD0, 0xF0 },
            .p = 0,
            .ticks = 4,
            .expected_pc = TEST_ROM_OFFSET - 14,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        uint16_t offset = tests[i].offset;
        program[offset] = tests[i].prg[0];
        program[offset + 1] = tests[i].prg[1];
        test_load_rom(program, sizeof(program));

        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.pc += offset;
        cpu.p = tests[i].p;

        for (int tick = 0; tick < tests[i].ticks; tick++) {
            cpu_tick(&cpu, bus);
        }

        assert(cpu.cycle == 0);
        assert(cpu.pc == tests[i].expected_pc);
    }
}

void test_clc(void) {
    uint8_t program[] = { 0x18 };

    const struct bus *bus = test_bus();
    struct cpu cpu;

    test_load_rom(program, sizeof(program));
    cpu_init(&cpu, TEST_ROM_OFFSET);

    cpu.p = P_C | P_N;

    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);

    assert(cpu.cycle == 0);
    assert(cpu.pc == TEST_ROM_OFFSET + 1);
    assert(cpu.p == P_N);
}

void test_jsr(void) {
    uint8_t program[] = { 0x20, 0x48, 0xF0 };

    const struct bus *bus = test_bus();
    struct cpu cpu;

    test_load_rom(program, sizeof(program));
    cpu_init(&cpu, TEST_ROM_OFFSET);

    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);
    cpu_tick(&cpu, bus);

    cpu_tick(&cpu, bus);
    assert(cpu.sp == 0xFE);
    assert(bus_peek(bus, 0x01FF) == ((TEST_ROM_OFFSET >> 8) & 0xFF));

    cpu_tick(&cpu, bus);
    assert(cpu.sp == 0xFD);
    assert(bus_peek(bus, 0x01FE) == ((TEST_ROM_OFFSET + 2) & 0xFF));

    cpu_tick(&cpu, bus);
    assert(cpu.cycle == 0);
    assert(cpu.pc == 0xF048);
}

void test_adc(void) {
    const struct bus *bus = test_bus();
    struct cpu cpu;

    struct {
        uint8_t prg[2];
        uint8_t a;
        uint8_t p;
        uint8_t expected_a;
        uint8_t expected_p;
    } tests[] = {
        {
            .prg = { 0x69, 0x01 },
            .a = 0x01,
            .p = P_Z | P_V | P_N,
            .expected_a = 0x02,
            .expected_p = 0,
        },
        {
            .prg = { 0x69, 0x01 },
            .a = 0x01,
            .p = P_C,
            .expected_a = 0x03,
            .expected_p = 0,
        },
        {
            .prg = { 0x69, 0x70 },
            .a = 0x70,
            .p = 0,
            .expected_a = 0xE0,
            .expected_p = P_N | P_V,
        },
        {
            .prg = { 0x69, 0x80 },
            .a = 0x80,
            .p = 0,
            .expected_a = 0x00,
            .expected_p = P_C | P_V | P_Z,
        },
        {
            .prg = { 0x69, 0x70 },
            .a = 0x90,
            .p = 0,
            .expected_a = 0x00,
            .expected_p = P_C | P_Z,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        test_load_rom(tests[i].prg, 2);

        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.a = tests[i].a;
        cpu.p = tests[i].p;

        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);

        assert(cpu.cycle == 0);
        assert(cpu.a == tests[i].expected_a);
        assert(cpu.p == tests[i].expected_p);
    }
}

void test_sbc(void) {
    const struct bus *bus = test_bus();
    struct cpu cpu;

    struct {
        uint8_t prg[2];
        uint8_t a;
        uint8_t p;
        uint8_t expected_a;
        uint8_t expected_p;
    } tests[] = {
        {
            .prg = { 0xE9, 0x01 },
            .a = 0x01,
            .p = P_C | P_V,
            .expected_a = 0x00,
            .expected_p = P_Z | P_C,
        },
        {
            .prg = { 0xE9, 0xF0 },
            .a = 0x50,
            .p = P_C,
            .expected_a = 0x60,
            .expected_p = 0,
        },
        {
            .prg = { 0xE9, 0xB0 },
            .a = 0x50,
            .p = P_C,
            .expected_a = 0xA0,
            .expected_p = P_N | P_V,
        },
        {
            .prg = { 0xE9, 0x70 },
            .a = 0x50,
            .p = P_C,
            .expected_a = 0xE0,
            .expected_p = P_N,
        },
        {
            .prg = { 0xE9, 0x00 },
            .a = 0x00,
            .p = P_C | P_Z,
            .expected_a = 0x00,
            .expected_p = P_C | P_Z,
        },
        {
            .prg = { 0xE9, 0x12 },
            .a = 0x34,
            .p = 0,
            .expected_a = 0x21,
            .expected_p = P_C
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        test_load_rom(tests[i].prg, 2);

        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.a = tests[i].a;
        cpu.p = tests[i].p;

        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);

        assert(cpu.cycle == 0);
        assert(cpu.a == tests[i].expected_a);
        assert(cpu.p == tests[i].expected_p);
    }
}

void test_cmp(void) {
    const struct bus *bus = test_bus();
    struct cpu cpu;

    struct {
        uint8_t prg[2];
        uint8_t a;
        uint8_t p;
        uint8_t expected_a;
        uint8_t expected_p;
    } tests[] = {
        {
            .prg = { 0xC9, 0x01 },
            .a = 0x01,
            .p = P_C | P_N,
            .expected_a = 0x01,
            .expected_p = P_Z | P_C,
        },
        {
            .prg = { 0xC9, 0x01 },
            .a = 0x02,
            .p = P_Z,
            .expected_a = 0x02,
            .expected_p = P_C,
        },
        {
            .prg = { 0xC9, 0x70 },
            .a = 0x20,
            .p = 0,
            .expected_a = 0x20,
            .expected_p = P_N,
        },
    };

    for (size_t i = 0; i < COUNT(tests); i++) {
        test_load_rom(tests[i].prg, 2);

        cpu_init(&cpu, TEST_ROM_OFFSET);
        cpu.a = tests[i].a;
        cpu.p = tests[i].p;

        cpu_tick(&cpu, bus);
        cpu_tick(&cpu, bus);

        assert(cpu.cycle == 0);
        assert(cpu.a == tests[i].expected_a);
        assert(cpu.p == tests[i].expected_p);
    }
}

void test_simple_load_and_store(void) {
    uint8_t program[] = {
        0xA9, 0x01, 0x8D, 0x00, 0x02, 0xA9, 0x05, 0x8D,
        0x01, 0x02, 0xA9, 0x08, 0x8D, 0x02, 0x02, 0x00,
        0x00
    };

    const struct bus *bus = test_bus();
    struct cpu cpu;

    test_load_rom(program, sizeof(program));
    cpu_init(&cpu, TEST_ROM_OFFSET);

    while (!(cpu.p & P_I)) {
        cpu_step(&cpu, bus);
    }

    assert(bus_peek(bus, 0x0200) == 0x01);
    assert(bus_peek(bus, 0x0201) == 0x05);
    assert(bus_peek(bus, 0x0202) == 0x08);
}

int main(void) {
    TEST_INIT();

    TEST(test_ora_imm);
    TEST(test_ora_abl);
    TEST(test_ora_abx);
    TEST(test_ora_aby);
    TEST(test_ora_zpg);
    TEST(test_ora_zpx);
    TEST(test_ora_idx);
    TEST(test_ora_idy);

    TEST(test_sta_abl);
    TEST(test_sta_abx);
    TEST(test_sta_aby);
    TEST(test_sta_zpg);
    TEST(test_sta_zpx);
    TEST(test_sta_idx);
    TEST(test_sta_idy);

    TEST(test_asl_acc);
    TEST(test_asl_zpg);
    TEST(test_asl_zpx);
    TEST(test_asl_abl);
    TEST(test_asl_abx);

    TEST(test_stx_abl);
    TEST(test_sty_abl);

    TEST(test_php);
    TEST(test_bpl);
    TEST(test_bne);
    TEST(test_clc);
    TEST(test_jsr);
    TEST(test_adc);
    TEST(test_sbc);
    TEST(test_cmp);

    TEST(test_simple_load_and_store);

    return 0;
}
