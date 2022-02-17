#ifndef __BUS_H__
#define __BUS_H__

#include <stdint.h>

struct bus {
    void *inst;
    uint8_t (*peek)(void *inst, uint16_t addr);
    void (*poke)(void *inst, uint16_t addr, uint8_t data);
};


uint8_t bus_peek(const struct bus *bus, uint16_t addr);
void bus_poke(const struct bus *bus, uint16_t addr, uint8_t data);

#endif
