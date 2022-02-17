#include <string.h>
#include "cpu.h"

typedef void (*action)(struct cpu *cpu);

typedef int (*procedure)(struct cpu *cpu, const struct bus *bus);

#define ACTION_RD  (1 << 0)
#define ACTION_WR  (1 << 1)
#define ACTION_RMW (ACTION_RD | ACTION_WR)

struct instruction {
    procedure proc;
    action act;
    uint8_t act_type;
};

static const struct instruction instructions[];

static void set_z(struct cpu *cpu, uint8_t data) {
    cpu->p &= ~P_Z;
    cpu->p |= data ? 0 : P_Z;
}

static void set_n(struct cpu *cpu, uint8_t data) {
    cpu->p &= ~P_N;
    cpu->p |= data & 0x80 ? P_N : 0;
}

static void set_v(struct cpu *cpu, uint8_t in1, uint8_t in2, uint8_t out) {
    /*
    int u_over = (in1 & 0x80) && (in2 & 0x80) && !(out & 0x80); 
    int s_over = !(in1 & 0x80) && !(in2 & 0x80) && (out & 0x80);

    cpu->p &= ~P_V;
    cpu->p |= u_over || s_over ? P_V : 0;
    */
    cpu->p &= ~P_V;
    cpu->p |= (in1 ^ out) & (in2 ^ out) & 0x80 ? P_V : 0;
}

static void push_stack(struct cpu *cpu, const struct bus *bus, uint8_t data) {
    bus_poke(bus, 0x0100 | cpu->sp, data);
    cpu->sp--;
}

static uint8_t pop_stack(struct cpu *cpu, const struct bus *bus) {
    cpu->sp++;
    return bus_peek(bus, 0x0100 | cpu->sp);
}

static uint8_t curr_stack(struct cpu *cpu, const struct bus *bus) {
    return bus_peek(bus, 0x0100 | cpu->sp);
}

// only handles 3 digit BCD numbers
// assumes bcd number is valid
static uint16_t bcd2bin(uint16_t value) {
    uint16_t result = (value / 0x100) * 100;
    result += ((value & 0xFF) / 0x10) * 10;
    result += value & 0x0F;
    return result;
}

// only handles 3 digit BCD numbers
static uint16_t bin2bcd(uint16_t value) {
    uint16_t result = (value / 100) << 8;
    result |= ((value % 100) / 10) << 4;
    result += value % 10;
    return result;
}

//
// Explicit procedures
//

static int brk(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        bus_peek(bus, cpu->pc++);
        return 0;
    case 2:
        push_stack(cpu, bus, (cpu->pc >> 8) & 0xFF);
        return 0;
    case 3:
        push_stack(cpu, bus, cpu->pc & 0xFF);
        return 0;
    case 4:
        push_stack(cpu, bus, cpu->p | P_B | P_5);
        return 0;
    case 5:
        cpu->opr1 = bus_peek(bus, 0xFFFE);
        return 0;
    case 6:
        cpu->pc = bus_peek(bus, 0xFFFF);
        cpu->pc = (cpu->pc << 8) | cpu->opr1;
        cpu->p |= P_I;
        return 1;
    default:
        return 1;
    }
}

static int rst(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
        case 1:
            // normally read pc + 1
            return 0;
        case 2:
            // normally push high-byte of PC to stack
            cpu->sp--;
            return 0;
        case 3:
            // normally push low-byte of PC to stack
            cpu->sp--;
            return 0;
        case 4:
            // normally push P to stack
            cpu->sp--;
            return 0;
        case 5:
            cpu->opr1 = bus_peek(bus, 0xFFFC);
            return 0;
        case 6:
            cpu->pc = bus_peek(bus, 0xFFFD);
            cpu->pc = (cpu->pc << 8) | cpu->opr1;
            cpu->p |= P_I;
            cpu->intr &= ~INTR_RESET;
            return 1;
        default:
            return 1;
    }
}

static int rti(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        bus_peek(bus, cpu->pc);
        return 0;
    case 2:
        curr_stack(cpu, bus);
        return 0;
    case 3:
        cpu->p = pop_stack(cpu, bus);
        cpu->p &= ~(P_B | P_5);
        return 0;
    case 4:
        cpu->opr1 = pop_stack(cpu, bus);
        return 0;
    case 5:
        cpu->pc = pop_stack(cpu, bus);
        cpu->pc = (cpu->pc << 8) | cpu->opr1;
        return 1;
    default:
        return 1;
    }
}

static int php(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        bus_peek(bus, cpu->pc);
        return 0;
    case 2:
        push_stack(cpu, bus, cpu->p | P_B | P_5);
        return 1;
    default:
        return 1;
    }
}

static int plp(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        bus_peek(bus, cpu->pc);
        return 0;
    case 2:
        curr_stack(cpu, bus);
        return 0;
    case 3:
        cpu->p = pop_stack(cpu, bus);
        cpu->p &= ~(P_B | P_5);
        return 1;
    default:
        return 1;
    }
}

static int pha(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        bus_peek(bus, cpu->pc);
        return 0;
    case 2:
        push_stack(cpu, bus, cpu->a);
        return 1;
    default:
        return 1;
    }
}

static int pla(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        bus_peek(bus, cpu->pc);
        return 0;
    case 2:
        curr_stack(cpu, bus);
        return 0;
    case 3:
        cpu->a = pop_stack(cpu, bus);
        set_z(cpu, cpu->a);
        set_n(cpu, cpu->a);
        return 1;
    default:
        return 1;
    }
}

static int jsr(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        cpu->opr2 = bus_peek(bus, cpu->pc++);
        return 0;
    case 2:
        curr_stack(cpu, bus);
        return 0;
    case 3:
        push_stack(cpu, bus, (cpu->pc >> 8) & 0xFF);
        return 0;
    case 4:
        push_stack(cpu, bus, cpu->pc & 0xFF);
        return 0;
    case 5:
        cpu->pc = bus_peek(bus, cpu->pc);
        cpu->pc = (cpu->pc << 8) | cpu->opr2;
        return 1;
    default:
        return 1;
    }
}

static int rts(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        bus_peek(bus, cpu->pc);
        return 0;
    case 2:
        curr_stack(cpu, bus);
        return 0;
    case 3:
        cpu->opr1 = pop_stack(cpu, bus);
        return 0;
    case 4:
        cpu->pc = pop_stack(cpu, bus);
        cpu->pc = (cpu->pc << 8) | cpu->opr1;
        return 0;
    case 5:
        bus_peek(bus, cpu->pc);
        cpu->pc++;
        return 1;
    default:
        return 1;
    }
}

static int jmp_abl(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        cpu->opr1 = bus_peek(bus, cpu->pc++);
        return 0;
    case 2:
        cpu->pc = bus_peek(bus, cpu->pc);
        cpu->pc = (cpu->pc << 8) | cpu->opr1;
        return 1;
    default:
        return 1;
    }
}

static int jmp_ind(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        cpu->opr1 = bus_peek(bus, cpu->pc++);
        return 0;
    case 2:
        cpu->ea = bus_peek(bus, cpu->pc++);
        cpu->ea = (cpu->ea << 8) | cpu->opr1;
        return 0;
    case 3:
        cpu->opr2 = bus_peek(bus, cpu->ea);
        cpu->ea &= 0xFF00;
        cpu->ea |= (cpu->opr1 + 1) & 0x00FF;
        return 0;
    case 4:
        cpu->pc = bus_peek(bus, cpu->ea);
        cpu->pc = (cpu->pc << 8) | cpu->opr2;
        return 1;
    default:
        return 1;
    }
}

static int illegal(struct cpu *cpu, const struct bus *bus) {
    (void)cpu;
    (void)bus;
    return 1;
}

//
// Actions
//

static void ora(struct cpu *cpu) {
    cpu->a |= cpu->opr1;
    set_z(cpu, cpu->a);
    set_n(cpu, cpu->a);
}

static void and(struct cpu *cpu) {
    cpu->a &= cpu->opr1;
    set_z(cpu, cpu->a);
    set_n(cpu, cpu->a);
}

static void eor(struct cpu *cpu) {
    cpu->a ^= cpu->opr1;
    set_z(cpu, cpu->a);
    set_n(cpu, cpu->a);
}

// TODO handle V flag?
static void adc_bcd(struct cpu *cpu) {
    uint16_t result = bcd2bin(cpu->a) + bcd2bin(cpu->opr1);
    result += cpu->p & P_C ? 1 : 0;
    result = bin2bcd(result);

    cpu->p &= ~P_C;
    cpu->p |= result & 0x0100 ? P_C : 0;

    cpu->a = result & 0x00FF;

    set_z(cpu, cpu->a);
    set_n(cpu, cpu->a);
}

static void adc(struct cpu *cpu) {
    if (cpu->p & P_D) {
        adc_bcd(cpu);
        return;
    }

    uint16_t result = cpu->a + cpu->opr1 + (cpu->p & P_C ? 1 : 0);

    cpu->p &= ~P_C;
    cpu->p |= result & 0x0100 ? P_C : 0;

    set_v(cpu, cpu->a, cpu->opr1, result & 0x00FF);

    cpu->a = result & 0x00FF;

    set_z(cpu, cpu->a);
    set_n(cpu, cpu->a);
}

// TODO handle V flag?
static void sbc_bcd(struct cpu *cpu) {
    uint16_t result;
    uint16_t abin = bcd2bin(cpu->a);
    uint16_t opr1bin = bcd2bin(cpu->opr1) + (cpu->p & P_C ? 0 : 1);

    if (abin >= opr1bin) {
        result = abin - opr1bin;
        result = bin2bcd(result);
        cpu->p |= P_C;
    } else {
        result = 100 - opr1bin + abin;
        result = bin2bcd(result);
        cpu->p &= ~P_C;
    }

    cpu->a = result & 0x00FF;

    set_z(cpu, cpu->a);
    set_n(cpu, cpu->a);
}

static void sbc(struct cpu *cpu) {
    if (cpu->p & P_D) {
        sbc_bcd(cpu);
        return;
    }

    cpu->opr1 = ~cpu->opr1;
    adc(cpu);
}

static void cmp(struct cpu *cpu) {
    uint8_t result = cpu->a - cpu->opr1;

    cpu->p &= ~P_C;
    cpu->p |= cpu->a >= cpu->opr1 ? P_C : 0;

    set_z(cpu, result);
    set_n(cpu, result);
}

static void cpx(struct cpu *cpu) {
    uint8_t result = cpu->x - cpu->opr1;

    cpu->p &= ~P_C;
    cpu->p |= cpu->x >= cpu->opr1 ? P_C : 0;

    set_z(cpu, result);
    set_n(cpu, result);
}

static void cpy(struct cpu *cpu) {
    uint8_t result = cpu->y - cpu->opr1;

    cpu->p &= ~P_C;
    cpu->p |= cpu->y >= cpu->opr1 ? P_C : 0;

    set_z(cpu, result);
    set_n(cpu, result);
}

static void dec(struct cpu *cpu) {
    cpu->opr1--;
    set_z(cpu, cpu->opr1);
    set_n(cpu, cpu->opr1);
}

static void dex(struct cpu *cpu) {
    cpu->x--;
    set_z(cpu, cpu->x);
    set_n(cpu, cpu->x);
}

static void dey(struct cpu *cpu) {
    cpu->y--;
    set_z(cpu, cpu->y);
    set_n(cpu, cpu->y);
}

static void inc(struct cpu *cpu) {
    cpu->opr1++;
    set_z(cpu, cpu->opr1);
    set_n(cpu, cpu->opr1);
}

static void inx(struct cpu *cpu) {
    cpu->x++;
    set_z(cpu, cpu->x);
    set_n(cpu, cpu->x);
}

static void iny(struct cpu *cpu) {
    cpu->y++;
    set_z(cpu, cpu->y);
    set_n(cpu, cpu->y);
}

static void asl(struct cpu *cpu) {
    cpu->p &= ~P_C;
    cpu->p |= cpu->opr1 & 0x80 ? P_C : 0;

    cpu->opr1 <<= 1;
    set_z(cpu, cpu->opr1);
    set_n(cpu, cpu->opr1);
}

static void lsr(struct cpu *cpu) {
    cpu->p &= ~(P_C | P_N);
    cpu->p |= cpu->opr1 & 0x01 ? P_C : 0;

    cpu->opr1 >>= 1;
    set_z(cpu, cpu->opr1);

}

static void rol(struct cpu *cpu) {
    uint8_t c = cpu->p & P_C;
    cpu->p &= ~P_C;
    cpu->p |= cpu->opr1 & 0x80 ? P_C : 0;

    cpu->opr1 <<= 1;
    cpu->opr1 |= c ? 0x01 : 0;
    set_z(cpu, cpu->opr1);
    set_n(cpu, cpu->opr1);
}

static void ror(struct cpu *cpu) {
    uint8_t c = cpu->p & P_C;
    cpu->p &= ~P_C;
    cpu->p |= cpu->opr1 & 0x01 ? P_C : 0;

    cpu->opr1 >>= 1;
    cpu->opr1 |= c ? 0x80 : 0;
    set_z(cpu, cpu->opr1);
    set_n(cpu, cpu->opr1);
}

static void sta(struct cpu *cpu) {
    cpu->opr1 = cpu->a;
}

static void stx(struct cpu *cpu) {
    cpu->opr1 = cpu->x;
}

static void sty(struct cpu *cpu) {
    cpu->opr1 = cpu->y;
}

static void tax(struct cpu *cpu) {
    cpu->x = cpu->a;
    set_z(cpu, cpu->a);
    set_n(cpu, cpu->a);
}

static void tay(struct cpu *cpu) {
    cpu->y = cpu->a;
    set_z(cpu, cpu->a);
    set_n(cpu, cpu->a);
}

static void txa(struct cpu *cpu) {
    cpu->a = cpu->x;
    set_z(cpu, cpu->a);
    set_n(cpu, cpu->a);
}

static void tya(struct cpu *cpu) {
    cpu->a = cpu->y;
    set_z(cpu, cpu->a);
    set_n(cpu, cpu->a);
}

static void tsx(struct cpu *cpu) {
    cpu->x = cpu->sp;
    set_z(cpu, cpu->x);
    set_n(cpu, cpu->x);
}

static void txs(struct cpu *cpu) {
    cpu->sp = cpu->x;
}

static void lda(struct cpu *cpu) {
    cpu->a = cpu->opr1;
    set_z(cpu, cpu->opr1);
    set_n(cpu, cpu->opr1);
}

static void ldx(struct cpu *cpu) {
    cpu->x = cpu->opr1;
    set_z(cpu, cpu->opr1);
    set_n(cpu, cpu->opr1);
}

static void ldy(struct cpu *cpu) {
    cpu->y = cpu->opr1;
    set_z(cpu, cpu->opr1);
    set_n(cpu, cpu->opr1);
}

static void bpl(struct cpu *cpu) {
    cpu->opr1 = !(cpu->p & P_N);
}

static void bmi(struct cpu *cpu) {
    cpu->opr1 = cpu->p & P_N;
}

static void bne(struct cpu *cpu) {
    cpu->opr1 = !(cpu->p & P_Z);
}

static void beq(struct cpu *cpu) {
    cpu->opr1 = cpu->p & P_Z;
}

static void bcc(struct cpu *cpu) {
    cpu->opr1 = !(cpu->p & P_C);
}

static void bcs(struct cpu *cpu) {
    cpu->opr1 = cpu->p & P_C;
}

static void bvc(struct cpu *cpu) {
    cpu->opr1 = !(cpu->p & P_V);
}

static void bvs(struct cpu *cpu) {
    cpu->opr1 = cpu->p & P_V;
}

static void sec(struct cpu *cpu) {
    cpu->p |= P_C;
}

static void sed(struct cpu *cpu) {
    cpu->p |= P_D;
}

static void sei(struct cpu *cpu) {
    cpu->p |= P_I;
}

static void clc(struct cpu *cpu) {
    cpu->p &= ~P_C;
}

static void cld(struct cpu *cpu) {
    cpu->p &= ~P_D;
}

static void cli(struct cpu *cpu) {
    cpu->p &= ~P_I;
}

static void clv(struct cpu *cpu) {
    cpu->p &= ~P_V;
}

static void bit(struct cpu *cpu) {
    cpu->p &= ~(P_V | P_N | P_Z);
    cpu->p |= cpu->opr1 & (P_V | P_N);
    cpu->p |= cpu->opr1 & cpu->a ? 0 : P_Z;
}

static void nop(struct cpu *cpu) {
    (void)cpu;
}

//
// Address mode procedures
//

static int imm(struct cpu *cpu, const struct bus *bus) {
    cpu->opr1 = bus_peek(bus, cpu->pc++);
    instructions[cpu->opc].act(cpu);
    return 1;
}

static int imp(struct cpu *cpu, const struct bus *bus) {
    bus_peek(bus, cpu->pc);
    instructions[cpu->opc].act(cpu);
    return 1;
}

static int acc(struct cpu *cpu, const struct bus *bus) {
    bus_peek(bus, cpu->pc);
    cpu->opr1 = cpu->a;
    instructions[cpu->opc].act(cpu);
    cpu->a = cpu->opr1;
    return 1;
}

static int zpg(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        cpu->ea = bus_peek(bus, cpu->pc++);
        return 0;
    case 2:
        if (instructions[cpu->opc].act_type == ACTION_WR) {
            instructions[cpu->opc].act(cpu);
            bus_poke(bus, cpu->ea, cpu->opr1);
            return 1;
        }

        cpu->opr1 = bus_peek(bus, cpu->ea);

        if (instructions[cpu->opc].act_type == ACTION_RD) {
            instructions[cpu->opc].act(cpu);
            return 1;
        }

        return 0;
    case 3:
        bus_poke(bus, cpu->ea, cpu->opr1);
        instructions[cpu->opc].act(cpu);
        return 0;
    case 4:
        bus_poke(bus, cpu->ea, cpu->opr1);
        return 1;

    default:
        return 1;
    }
}

static int zpx(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        cpu->ea = bus_peek(bus, cpu->pc++);
        return 0;
    case 2:
        bus_peek(bus, cpu->ea);
        cpu->ea = (cpu->ea + cpu->x) & 0x00FF;
        return 0;
    case 3:
        if (instructions[cpu->opc].act_type == ACTION_WR) {
            instructions[cpu->opc].act(cpu);
            bus_poke(bus, cpu->ea, cpu->opr1);
            return 1;
        } 

        cpu->opr1 = bus_peek(bus, cpu->ea);

        if (instructions[cpu->opc].act_type == ACTION_RD) {
            instructions[cpu->opc].act(cpu);
            return 1;
        } 

        return 0;
    case 4:
        bus_poke(bus, cpu->ea, cpu->opr1);
        instructions[cpu->opc].act(cpu);
        return 0;
    case 5:
        bus_poke(bus, cpu->ea, cpu->opr1);
        return 1;
    default:
        return 1;
    }
}

// TODO I think I can remove the RMW steps
static int zpy(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        cpu->ea = bus_peek(bus, cpu->pc++);
        return 0;
    case 2:
        bus_peek(bus, cpu->ea);
        cpu->ea = (cpu->ea + cpu->y) & 0x00FF;
        return 0;
    case 3:
        if (instructions[cpu->opc].act_type == ACTION_WR) {
            instructions[cpu->opc].act(cpu);
            bus_poke(bus, cpu->ea, cpu->opr1);
            return 1;
        } 

        cpu->opr1 = bus_peek(bus, cpu->ea);

        if (instructions[cpu->opc].act_type == ACTION_RD) {
            instructions[cpu->opc].act(cpu);
            return 1;
        } 

        return 0;
    case 4:
        bus_poke(bus, cpu->ea, cpu->opr1);
        instructions[cpu->opc].act(cpu);
        return 0;
    case 5:
        bus_poke(bus, cpu->ea, cpu->opr1);
        return 1;
    default:
        return 1;
    }
}

static int abl(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        cpu->opr1 = bus_peek(bus, cpu->pc++);
        return 0;
    case 2:
        cpu->opr2 = bus_peek(bus, cpu->pc++);
        return 0;
    case 3:
        cpu->ea = cpu->opr2;
        cpu->ea = (cpu->ea << 8) | cpu->opr1;

        if (instructions[cpu->opc].act_type == ACTION_WR) {
            instructions[cpu->opc].act(cpu);
            bus_poke(bus, cpu->ea, cpu->opr1);
            return 1;
        }

        cpu->opr1 = bus_peek(bus, cpu->ea);

        if (instructions[cpu->opc].act_type == ACTION_RD) {
            instructions[cpu->opc].act(cpu);
            return 1;
        }

        return 0;
    case 4:
        bus_poke(bus, cpu->ea, cpu->opr1);
        instructions[cpu->opc].act(cpu);
        return 0;
    case 5:
        bus_poke(bus, cpu->ea, cpu->opr1);
        return 1;
    default:
        return 1;
    }
}

static int abx(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        cpu->opr2 = bus_peek(bus, cpu->pc++);
        return 0;
    case 2:
        cpu->ea = bus_peek(bus, cpu->pc++);
        cpu->ea = (cpu->ea << 8) | (uint8_t)(cpu->opr2 + cpu->x);
        return 0;
    case 3:
        cpu->opr1 = bus_peek(bus, cpu->ea);

        if (instructions[cpu->opc].act_type == ACTION_RD
            && (uint16_t)cpu->opr2 + cpu->x <= 0xFF) {
            instructions[cpu->opc].act(cpu);
            return 1;
        }

        cpu->ea &= 0xFF00;
        cpu->ea += cpu->opr2 + cpu->x;
        return 0;
    case 4:
        if (instructions[cpu->opc].act_type == ACTION_WR) {
            instructions[cpu->opc].act(cpu);
            bus_poke(bus, cpu->ea, cpu->opr1);
            return 1;
        } 

        cpu->opr1 = bus_peek(bus, cpu->ea);

        if (instructions[cpu->opc].act_type == ACTION_RD) {
            instructions[cpu->opc].act(cpu);
            return 1;
        }

        return 0;
    case 5:
        bus_poke(bus, cpu->ea, cpu->opr1);
        instructions[cpu->opc].act(cpu);
        return 0;
    case 6:

        bus_poke(bus, cpu->ea, cpu->opr1);
        return 1;
    default:
        return 1;
    }
}

static int aby(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        cpu->opr2 = bus_peek(bus, cpu->pc++);
        return 0;
    case 2:
        cpu->ea = bus_peek(bus, cpu->pc++);
        cpu->ea = (cpu->ea << 8) | (uint8_t)(cpu->opr2 + cpu->y);
        return 0;
    case 3:
        cpu->opr1 = bus_peek(bus, cpu->ea);

        if (instructions[cpu->opc].act_type == ACTION_RD
            && (uint16_t)cpu->opr2 + cpu->y <= 0xFF) {
            instructions[cpu->opc].act(cpu);
            return 1;
        }

        cpu->ea &= 0xFF00;
        cpu->ea += cpu->opr2 + cpu->y;
        return 0;
    case 4:
        if (instructions[cpu->opc].act_type == ACTION_RD) {
            cpu->opr1 = bus_peek(bus, cpu->ea);
            instructions[cpu->opc].act(cpu);
        } else {
            instructions[cpu->opc].act(cpu);
            bus_poke(bus, cpu->ea, cpu->opr1);
        }

        return 1;
    default:
        return 1;
    }
}

static int idx(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        cpu->ea = bus_peek(bus, cpu->pc++);
        return 0;
    case 2:
        bus_peek(bus, cpu->ea);
        cpu->ea = (cpu->ea + cpu->x) & 0x00FF;
        return 0;
    case 3:
        cpu->opr1 = bus_peek(bus, cpu->ea);
        return 0;
    case 4:
        cpu->ea = bus_peek(bus, (cpu->ea + 1) & 0x00FF);
        cpu->ea = (cpu->ea << 8) | cpu->opr1;
        return 0;
    case 5:
        if (instructions[cpu->opc].act_type == ACTION_RD) {
            cpu->opr1 = bus_peek(bus, cpu->ea);
            instructions[cpu->opc].act(cpu);
        } else {
            instructions[cpu->opc].act(cpu);
            bus_poke(bus, cpu->ea, cpu->opr1);
        }

        return 1;
    default:
        return 1;
    }
}

static int idy(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        cpu->ea = bus_peek(bus, cpu->pc++);
        return 0;
    case 2:
        cpu->opr2 = bus_peek(bus, cpu->ea);
        return 0;
    case 3:
        cpu->ea = bus_peek(bus, (cpu->ea + 1) & 0x00FF);
        cpu->ea = (cpu->ea << 8) | (uint8_t)(cpu->opr2 + cpu->y);
        return 0;
    case 4:
        cpu->opr1 = bus_peek(bus, cpu->ea);

        if (instructions[cpu->opc].act_type == ACTION_RD
            && (uint16_t)cpu->opr2 + cpu->y <= 0xFF) {
            instructions[cpu->opc].act(cpu);
            return 1;
        }

        cpu->ea &= 0xFF00;
        cpu->ea += cpu->opr2 + cpu->y;
        return 0;
    case 5:
        if (instructions[cpu->opc].act_type == ACTION_RD) {
            cpu->opr1 = bus_peek(bus, cpu->ea);
            instructions[cpu->opc].act(cpu);
        } else {
            instructions[cpu->opc].act(cpu);
            bus_poke(bus, cpu->ea, cpu->opr1);
        }

        return 1;
    default:
        return 1;
    }
}

static int rel(struct cpu *cpu, const struct bus *bus) {
    switch (cpu->cycle) {
    case 1:
        cpu->opr2 = bus_peek(bus, cpu->pc++);
        instructions[cpu->opc].act(cpu);

        if (!cpu->opr1) {
            return 1;
        }

        return 0;
    case 2:
        bus_peek(bus, cpu->pc);
        cpu->ea = cpu->pc & 0x00FF;

        if (cpu->opr2 & 0x80) {
            cpu->ea -= ((uint8_t)~cpu->opr2) + 1;
        } else {
            cpu->ea += cpu->opr2;
        }

        if (!(cpu->ea & 0xFF00)) {
            cpu->ea |= cpu->pc & 0xFF00;
            cpu->pc = cpu->ea;
            return 1;
        }

        cpu->ea &= 0x00FF;
        cpu->ea |= cpu->pc & 0xFF00;
        return 0;
    case 3:
        bus_peek(bus, cpu->ea);

        if (cpu->opr2 & 0x80) {
            cpu->pc -= ((uint8_t)~cpu->opr2) + 1;
        } else {
            cpu->pc += cpu->opr2;
        }

        return 1;
    default:
        return 1;
    }
}

//
// Public functions
//

void cpu_init(struct cpu *cpu, uint16_t pc) {
    memset(cpu, 0, sizeof(struct cpu));
    cpu->pc = pc;
    cpu->sp = 0xFF;
}

void cpu_tick(struct cpu *cpu, const struct bus *bus) {
    if (cpu->intr & INTR_RESET) {
        if (cpu->cycle == 0) {
            cpu->cycle++;
            return;
        }

        if (rst(cpu, bus)) {
            cpu->cycle = 0;
        } else {
            cpu->cycle++;
        }

        return;
    }

    if (cpu->cycle == 0) {
        cpu->opc = bus_peek(bus, cpu->pc++);
        cpu->cycle++;
        return;
    } 

    struct instruction inst = instructions[cpu->opc];

    if (inst.proc(cpu, bus)) {
        cpu->cycle = 0;
    } else {
        cpu->cycle++;
    }
}

void cpu_step(struct cpu *cpu, const struct bus *bus) {
    do {
        cpu_tick(cpu, bus);
    } while (cpu->cycle != 0);
}

//
// Instruction table
//

static const struct instruction instructions[] = {
/* 0x00 */ { .proc = brk,     .act = NULL, .act_type = 0          },
/* 0x01 */ { .proc = idx,     .act = ora,  .act_type = ACTION_RD  },
/* 0x02 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x03 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x04 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x05 */ { .proc = zpg,     .act = ora,  .act_type = ACTION_RD  },
/* 0x06 */ { .proc = zpg,     .act = asl,  .act_type = ACTION_RMW },
/* 0x07 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x08 */ { .proc = php,     .act = NULL, .act_type = 0          },
/* 0x09 */ { .proc = imm,     .act = ora,  .act_type = ACTION_RD  },
/* 0x0A */ { .proc = acc,     .act = asl,  .act_type = ACTION_RMW },
/* 0x0B */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x0C */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x0D */ { .proc = abl,     .act = ora,  .act_type = ACTION_RD  },
/* 0x0E */ { .proc = abl,     .act = asl,  .act_type = ACTION_RMW },
/* 0x0F */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x10 */ { .proc = rel,     .act = bpl,  .act_type = 0          },
/* 0x11 */ { .proc = idy,     .act = ora,  .act_type = ACTION_RD  },
/* 0x12 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x13 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x14 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x15 */ { .proc = zpx,     .act = ora,  .act_type = ACTION_RD  },
/* 0x16 */ { .proc = zpx,     .act = asl,  .act_type = ACTION_RMW },
/* 0x17 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x18 */ { .proc = imp,     .act = clc,  .act_type = 0          },
/* 0x19 */ { .proc = aby,     .act = ora,  .act_type = ACTION_RD  },
/* 0x1A */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x1B */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x1C */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x1D */ { .proc = abx,     .act = ora,  .act_type = ACTION_RD  },
/* 0x1E */ { .proc = abx,     .act = asl,  .act_type = ACTION_RMW },
/* 0x1F */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x20 */ { .proc = jsr,     .act = NULL, .act_type = 0          },
/* 0x21 */ { .proc = idx,     .act = and,  .act_type = ACTION_RD  },
/* 0x22 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x23 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x24 */ { .proc = zpg,     .act = bit,  .act_type = ACTION_RD  },
/* 0x25 */ { .proc = zpg,     .act = and,  .act_type = ACTION_RD  },
/* 0x26 */ { .proc = zpg,     .act = rol,  .act_type = ACTION_RMW },
/* 0x27 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x28 */ { .proc = plp,     .act = NULL, .act_type = 0          },
/* 0x29 */ { .proc = imm,     .act = and,  .act_type = ACTION_RD  },
/* 0x2A */ { .proc = acc,     .act = rol,  .act_type = ACTION_RMW },
/* 0x2B */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x2C */ { .proc = abl,     .act = bit,  .act_type = ACTION_RD  },
/* 0x2D */ { .proc = abl,     .act = and,  .act_type = ACTION_RD  },
/* 0x2E */ { .proc = abl,     .act = rol,  .act_type = ACTION_RMW },
/* 0x2F */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x30 */ { .proc = rel,     .act = bmi,  .act_type = 0          },
/* 0x31 */ { .proc = idy,     .act = and,  .act_type = ACTION_RD  },
/* 0x32 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x33 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x34 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x35 */ { .proc = zpx,     .act = and,  .act_type = ACTION_RD  },
/* 0x36 */ { .proc = zpx,     .act = rol,  .act_type = ACTION_RMW },
/* 0x37 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x38 */ { .proc = imp,     .act = sec,  .act_type = 0          },
/* 0x39 */ { .proc = aby,     .act = and,  .act_type = ACTION_RD  },
/* 0x3A */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x3B */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x3C */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x3D */ { .proc = abx,     .act = and,  .act_type = ACTION_RD  },
/* 0x3E */ { .proc = abx,     .act = rol,  .act_type = ACTION_RMW },
/* 0x3F */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x40 */ { .proc = rti,     .act = NULL, .act_type = 0          },
/* 0x41 */ { .proc = idx,     .act = eor,  .act_type = ACTION_RD  },
/* 0x42 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x43 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x44 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x45 */ { .proc = zpg,     .act = eor,  .act_type = ACTION_RD  },
/* 0x46 */ { .proc = zpg,     .act = lsr,  .act_type = ACTION_RMW },
/* 0x47 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x48 */ { .proc = pha,     .act = NULL, .act_type = 0          },
/* 0x49 */ { .proc = imm,     .act = eor,  .act_type = ACTION_RD  },
/* 0x4A */ { .proc = acc,     .act = lsr,  .act_type = ACTION_RMW },
/* 0x4B */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x4C */ { .proc = jmp_abl, .act = NULL, .act_type = 0          },
/* 0x4D */ { .proc = abl,     .act = eor,  .act_type = ACTION_RD  },
/* 0x4E */ { .proc = abl,     .act = lsr,  .act_type = ACTION_RMW },
/* 0x4F */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x50 */ { .proc = rel,     .act = bvc,  .act_type = 0          },
/* 0x51 */ { .proc = idy,     .act = eor,  .act_type = ACTION_RD  },
/* 0x52 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x53 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x54 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x55 */ { .proc = zpx,     .act = eor,  .act_type = ACTION_RD  },
/* 0x56 */ { .proc = zpx,     .act = lsr,  .act_type = ACTION_RMW },
/* 0x57 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x58 */ { .proc = imp,     .act = cli,  .act_type = 0          },
/* 0x59 */ { .proc = aby,     .act = eor,  .act_type = ACTION_RD  },
/* 0x5A */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x5B */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x5C */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x5D */ { .proc = abx,     .act = eor,  .act_type = ACTION_RD  },
/* 0x5E */ { .proc = abx,     .act = lsr,  .act_type = ACTION_RMW },
/* 0x5F */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x60 */ { .proc = rts,     .act = NULL, .act_type = 0          },
/* 0x61 */ { .proc = idx,     .act = adc,  .act_type = ACTION_RD  },
/* 0x62 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x63 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x64 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x65 */ { .proc = zpg,     .act = adc,  .act_type = ACTION_RD  },
/* 0x66 */ { .proc = zpg,     .act = ror,  .act_type = ACTION_RMW },
/* 0x67 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x68 */ { .proc = pla,     .act = NULL, .act_type = 0          },
/* 0x69 */ { .proc = imm,     .act = adc,  .act_type = ACTION_RD  },
/* 0x6A */ { .proc = acc,     .act = ror,  .act_type = ACTION_RMW },
/* 0x6B */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x6C */ { .proc = jmp_ind, .act = NULL, .act_type = 0          },
/* 0x6D */ { .proc = abl,     .act = adc,  .act_type = ACTION_RD  },
/* 0x6E */ { .proc = abl,     .act = ror,  .act_type = ACTION_RMW },
/* 0x6F */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x70 */ { .proc = rel,     .act = bvs,  .act_type = 0          },
/* 0x71 */ { .proc = idy,     .act = adc,  .act_type = ACTION_RD  },
/* 0x72 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x73 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x74 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x75 */ { .proc = zpx,     .act = adc,  .act_type = ACTION_RD  },
/* 0x76 */ { .proc = zpx,     .act = ror,  .act_type = ACTION_RMW },
/* 0x77 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x78 */ { .proc = imp,     .act = sei,  .act_type = 0          },
/* 0x79 */ { .proc = aby,     .act = adc,  .act_type = ACTION_RD  },
/* 0x7A */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x7B */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x7C */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x7D */ { .proc = abx,     .act = adc,  .act_type = ACTION_RD  },
/* 0x7E */ { .proc = abx,     .act = ror,  .act_type = ACTION_RMW },
/* 0x7F */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x80 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x81 */ { .proc = idx,     .act = sta,  .act_type = ACTION_WR  },
/* 0x82 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x83 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x84 */ { .proc = zpg,     .act = sty,  .act_type = ACTION_WR  },
/* 0x85 */ { .proc = zpg,     .act = sta,  .act_type = ACTION_WR  },
/* 0x86 */ { .proc = zpg,     .act = stx,  .act_type = ACTION_WR  },
/* 0x87 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x88 */ { .proc = imp,     .act = dey,  .act_type = 0          },
/* 0x89 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x8A */ { .proc = imp,     .act = txa,  .act_type = 0          },
/* 0x8B */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x8C */ { .proc = abl,     .act = sty,  .act_type = ACTION_WR  },
/* 0x8D */ { .proc = abl,     .act = sta,  .act_type = ACTION_WR  },
/* 0x8E */ { .proc = abl,     .act = stx,  .act_type = ACTION_WR  },
/* 0x8F */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x90 */ { .proc = rel,     .act = bcc,  .act_type = 0          },
/* 0x91 */ { .proc = idy,     .act = sta,  .act_type = ACTION_WR  },
/* 0x92 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x93 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x94 */ { .proc = zpx,     .act = sty,  .act_type = ACTION_WR  },
/* 0x95 */ { .proc = zpx,     .act = sta,  .act_type = ACTION_WR  },
/* 0x96 */ { .proc = zpy,     .act = stx,  .act_type = ACTION_WR  },
/* 0x97 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x98 */ { .proc = imp,     .act = tya,  .act_type = 0          },
/* 0x99 */ { .proc = aby,     .act = sta,  .act_type = ACTION_WR  },
/* 0x9A */ { .proc = imp,     .act = txs,  .act_type = 0          },
/* 0x9B */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x9C */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x9D */ { .proc = abx,     .act = sta,  .act_type = ACTION_WR  },
/* 0x9E */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0x9F */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xA0 */ { .proc = imm,     .act = ldy,  .act_type = ACTION_RD  },
/* 0xA1 */ { .proc = idx,     .act = lda,  .act_type = ACTION_RD  },
/* 0xA2 */ { .proc = imm,     .act = ldx,  .act_type = ACTION_RD  },
/* 0xA3 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xA4 */ { .proc = zpg,     .act = ldy,  .act_type = ACTION_RD  },
/* 0xA5 */ { .proc = zpg,     .act = lda,  .act_type = ACTION_RD  },
/* 0xA6 */ { .proc = zpg,     .act = ldx,  .act_type = ACTION_RD  },
/* 0xA7 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xA8 */ { .proc = imp,     .act = tay,  .act_type = 0          },
/* 0xA9 */ { .proc = imm,     .act = lda,  .act_type = ACTION_RD  },
/* 0xAA */ { .proc = imp,     .act = tax,  .act_type = 0          },
/* 0xAB */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xAC */ { .proc = abl,     .act = ldy,  .act_type = ACTION_RD  },
/* 0xAD */ { .proc = abl,     .act = lda,  .act_type = ACTION_RD  },
/* 0xAE */ { .proc = abl,     .act = ldx,  .act_type = ACTION_RD  },
/* 0xAF */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xB0 */ { .proc = rel,     .act = bcs,  .act_type = 0          },
/* 0xB1 */ { .proc = idy,     .act = lda,  .act_type = ACTION_RD  },
/* 0xB2 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xB3 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xB4 */ { .proc = zpx,     .act = ldy,  .act_type = ACTION_RD  },
/* 0xB5 */ { .proc = zpx,     .act = lda,  .act_type = ACTION_RD  },
/* 0xB6 */ { .proc = zpy,     .act = ldx,  .act_type = ACTION_RD  },
/* 0xB7 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xB8 */ { .proc = imp,     .act = clv,  .act_type = 0          },
/* 0xB9 */ { .proc = aby,     .act = lda,  .act_type = ACTION_RD  },
/* 0xBA */ { .proc = imp,     .act = tsx,  .act_type = ACTION_RD  },
/* 0xBB */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xBC */ { .proc = abx,     .act = ldy,  .act_type = ACTION_RD  },
/* 0xBD */ { .proc = abx,     .act = lda,  .act_type = ACTION_RD  },
/* 0xBE */ { .proc = aby,     .act = ldx,  .act_type = ACTION_RD  },
/* 0xBF */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xC0 */ { .proc = imm,     .act = cpy,  .act_type = ACTION_RD  },
/* 0xC1 */ { .proc = idx,     .act = cmp,  .act_type = ACTION_RD  },
/* 0xC2 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xC3 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xC4 */ { .proc = zpg,     .act = cpy,  .act_type = ACTION_RD  },
/* 0xC5 */ { .proc = zpg,     .act = cmp,  .act_type = ACTION_RD  },
/* 0xC6 */ { .proc = zpg,     .act = dec,  .act_type = ACTION_RMW },
/* 0xC7 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xC8 */ { .proc = imp,     .act = iny,  .act_type = ACTION_RMW },
/* 0xC9 */ { .proc = imm,     .act = cmp,  .act_type = ACTION_RD  },
/* 0xCA */ { .proc = imp,     .act = dex,  .act_type = 0          },
/* 0xCB */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xCC */ { .proc = abl,     .act = cpy,  .act_type = ACTION_RD  },
/* 0xCD */ { .proc = abl,     .act = cmp,  .act_type = ACTION_RD  },
/* 0xCE */ { .proc = abl,     .act = dec,  .act_type = ACTION_RMW },
/* 0xCF */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xD0 */ { .proc = rel,     .act = bne,  .act_type = 0          },
/* 0xD1 */ { .proc = idy,     .act = cmp,  .act_type = ACTION_RD  },
/* 0xD2 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xD3 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xD4 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xD5 */ { .proc = zpx,     .act = cmp,  .act_type = ACTION_RD  },
/* 0xD6 */ { .proc = zpx,     .act = dec,  .act_type = ACTION_RMW },
/* 0xD7 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xD8 */ { .proc = imp,     .act = cld,  .act_type = 0          },
/* 0xD9 */ { .proc = aby,     .act = cmp,  .act_type = ACTION_RD  },
/* 0xDA */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xDB */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xDC */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xDD */ { .proc = abx,     .act = cmp,  .act_type = ACTION_RD  },
/* 0xDE */ { .proc = abx,     .act = dec,  .act_type = ACTION_RMW },
/* 0xDF */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xE0 */ { .proc = imm,     .act = cpx,  .act_type = ACTION_RD  },
/* 0xE1 */ { .proc = idx,     .act = sbc,  .act_type = ACTION_RD  },
/* 0xE2 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xE3 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xE4 */ { .proc = zpg,     .act = cpx,  .act_type = ACTION_RD  },
/* 0xE5 */ { .proc = zpg,     .act = sbc,  .act_type = ACTION_RD  },
/* 0xE6 */ { .proc = zpg,     .act = inc,  .act_type = ACTION_RMW },
/* 0xE7 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xE8 */ { .proc = imp,     .act = inx,  .act_type = ACTION_RMW },
/* 0xE9 */ { .proc = imm,     .act = sbc,  .act_type = ACTION_RD  },
/* 0xEA */ { .proc = imp,     .act = nop,  .act_type = 0          },
/* 0xEB */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xEC */ { .proc = abl,     .act = cpx,  .act_type = ACTION_RD  },
/* 0xED */ { .proc = abl,     .act = sbc,  .act_type = ACTION_RD  },
/* 0xEE */ { .proc = abl,     .act = inc,  .act_type = ACTION_RMW },
/* 0xEF */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xF0 */ { .proc = rel,     .act = beq,  .act_type = 0          },
/* 0xF1 */ { .proc = idy,     .act = sbc,  .act_type = ACTION_RD  },
/* 0xF2 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xF3 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xF4 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xF5 */ { .proc = zpx,     .act = sbc,  .act_type = ACTION_RD  },
/* 0xF6 */ { .proc = zpx,     .act = inc,  .act_type = ACTION_RMW },
/* 0xF7 */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xF8 */ { .proc = imp,     .act = sed,  .act_type = 0          },
/* 0xF9 */ { .proc = aby,     .act = sbc,  .act_type = ACTION_RD  },
/* 0xFA */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xFB */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xFC */ { .proc = illegal, .act = NULL, .act_type = 0          },
/* 0xFD */ { .proc = abx,     .act = sbc,  .act_type = ACTION_RD  },
/* 0xFE */ { .proc = abx,     .act = inc,  .act_type = ACTION_RMW },
/* 0xFF */ { .proc = illegal, .act = NULL, .act_type = 0          },
};
