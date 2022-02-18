#include "test.h"

void test_write_then_read(void) {
    const struct bus *bus = test_bus();

    for (int i = 0; i < TEST_RAM_SIZE; i++) {
        uint16_t addr = TEST_RAM_OFFSET + i;
        bus_poke(bus, addr, (uint8_t)i);
        assert(bus_peek(bus, addr) == (uint8_t)i);;
    }
}

int main(void) {
    TEST_INIT();

    TEST(test_write_then_read);

    return 0;
}
