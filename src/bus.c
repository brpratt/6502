#include <stdlib.h>

#include "bus.h"

uint8_t bus_peek(const struct bus *bus, uint16_t addr) {
    return bus->peek(bus->inst, addr);
}

void bus_poke(const struct bus *bus, uint16_t addr, uint8_t data) {
    bus->poke(bus->inst, addr, data);
}
