#include <cstring>
#include <functional>
#include <stdio.h>
#include <unordered_map>
#include <cstdint>
#include <inttypes.h>
#include <variant>

bool debug = false;

struct MACHINE;


// #########################
// Register abstractions
// #########################

typedef enum __attribute__((packed)) REG {
    REG_A = 0xAA,
    REG_B = 0xBB,
    REG_C = 0xCC,
    REG_D = 0xDD,

    REG_CA = 0xCA,
    REG_CB = 0xCB,

    REG_PC = 0x8c,
    REG_MAR = 0x8a,
    REG_MDR = 0x8d,
} REG;

typedef union __attribute__((packed)) IMMEDIATE {
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
} IMMEDIATE;

using VALUE = std::variant<std::monostate, uint8_t, uint16_t, uint32_t>;

typedef enum REG_SIZE {
    U8 = 1,
    U16 = 2,
    U32 = 4,
} REG_SIZE;




class MetaRegister {
public:
    REG_SIZE size;
    bool generalPurpose;

    MetaRegister() : size(U8), generalPurpose(false) {}
    MetaRegister(REG_SIZE size, bool generalPurpose) : size(size), generalPurpose(generalPurpose) {}
};

std::unordered_map<REG, MetaRegister> REGISTERS_META = {
    { REG_A, MetaRegister(U32, true) },
    { REG_B, MetaRegister(U32, true) },
    { REG_C, MetaRegister(U32, true) },
    { REG_D, MetaRegister(U32, true) },

    { REG_CA, MetaRegister(U8, true) },
    { REG_CB, MetaRegister(U8, true) },

    { REG_PC, MetaRegister(U32, false) },
    { REG_MAR, MetaRegister(U32, false) },
    { REG_MDR, MetaRegister(U8, false) },
};



struct REGISTER {
public:

    MetaRegister meta;
    IMMEDIATE value = { .u32 = 0 };

    REGISTER() {}
    REGISTER(MetaRegister meta) : meta(meta) {}

    uint32_t to_u32() {
        switch (meta.size) {
        case U8: return (uint32_t)value.u8;
        case U16: return (uint32_t)value.u16;
        case U32: return (uint32_t)value.u32;
        default:
            return 0;
        }
    }

    void set(VALUE val) {
        if(val.index() == 0) {
            auto v = std::get<uint8_t>(val);

            if(meta.size != U8) {
                printf("ERROR: Setting register to wrong size\n");
                return;
            }

            value.u8 = v;
        } else if(val.index() == 1) {
            auto v = std::get<uint16_t>(val);

            if(meta.size != U16) {
                printf("ERROR: Setting register to wrong size\n");
                return;
            }

            value.u16 = v;
        } else if(val.index() == 2) {
            auto v = std::get<uint16_t>(val);

            if(meta.size != U32) {
                printf("ERROR: Setting register to wrong size\n");
                return;
            }

            value.u32 = v;
        } else {
            printf("ERROR: Got std::monostate in Register::operator=\n");
            return;
        }
    }

    void set(REGISTER reg) {
        this->set(reg.value);
    }

    void set(IMMEDIATE imm) {
        switch (meta.size) {
        case U8: value.u8 = imm.u8; break;
        case U16: value.u16 = imm.u16; break;
        case U32: value.u32 = imm.u32; break;
        }
    }

    void set(uint32_t v) {
        switch (meta.size) {
        case U8: value.u8 = (uint8_t)v; break;
        case U16: value.u16 = (uint16_t)v; break;
        case U32: value.u32 = (uint32_t)v; break;
        }
    }

    void add(IMMEDIATE imm) {
        switch (meta.size) {
        case U8: value.u8 += imm.u8; break;
        case U16: value.u16 += imm.u16; break;
        case U32: value.u32 += imm.u32; break;
        }
    }

    void add(uint32_t val) {
        this->add(IMMEDIATE{ .u32 = val });
    }

    void add(REGISTER reg) {
        this->add(reg.value);
    }

    void subtract(IMMEDIATE imm) {
        switch (meta.size) {
        case U8: value.u8 -= imm.u8; break;
        case U16: value.u16 -= imm.u16; break;
        case U32: value.u32 -= imm.u32; break;
        }
    }

    void subtract(uint32_t val) {
        this->subtract(IMMEDIATE{ .u32 = val });
    }

    void subtract(REGISTER reg) {
        this->subtract(reg.value);
    }

    uint64_t compare(IMMEDIATE imm) {
        switch (meta.size) {
        case U8: return (int64_t)value.u8 - (int64_t)imm.u8;
        case U16: return (int64_t)value.u16 - (int64_t)imm.u16;
        case U32: return (int64_t)value.u32 - (int64_t)imm.u32;
        default: return false;
        }
    }

    bool compare(REGISTER reg) {
        return this->compare(reg.value);
    }

    bool compare(uint32_t val) {
        return this->compare(IMMEDIATE{ .u32 = val });
    }

    void shfl(uint8_t val) {
        switch (meta.size) {
        case U8: value.u8 <<= val; break;
        case U16: value.u16 <<= val; break;
        case U32: value.u32 <<= val; break;
        }
    }

    void shfr(uint8_t val) {
        switch (meta.size) {
        case U8: value.u8 >>= val; break;
        case U16: value.u16 >>= val; break;
        case U32: value.u32 >>= val; break;
        }
    }

};





// #########################
// Instruction windows
// #########################

typedef enum __attribute__((packed)) INSTS {
    MOV = 0x01,
    MVI = 0x02,
} INSTS;

typedef struct __attribute__((packed)) WIN {
    uint32_t size() {
        printf("WARNING: Returning 0 from base ::size()\n");
        return 0;
    };
} WIN;





typedef struct __attribute__((packed)) WIN_MOV : WIN {
    REG dest;
    REG src;
    uint32_t size() { return sizeof(*this); }
} WIN_MOV;

typedef struct __attribute__((packed)) WIN_MVI : WIN {
    REG dest;
    IMMEDIATE val;
    uint32_t size() { return sizeof(REG) + REGISTERS_META[dest].size; }
} WIN_MVI;

typedef struct __attribute__((packed)) WIN_JMP : WIN {
    uint32_t addr;
    uint32_t size() { return sizeof(*this); }
} WIN_JMP;

typedef struct __attribute__((packed)) WIN_ADD : WIN {
    REG dest;
    REG src;
    uint32_t size() { return sizeof(*this); }
} WIN_ADD;

typedef struct __attribute__((packed)) WIN_ADDI : WIN {
    REG dest;
    IMMEDIATE val;
    uint32_t size() { return sizeof(REG) + REGISTERS_META[dest].size; }
} WIN_ADDI;

typedef struct __attribute__((packed)) WIN_OUT : WIN {
    REG reg;
    uint32_t size() { return sizeof(*this); }
} WIN_OUT;

typedef struct __attribute__((packed)) WIN_IN : WIN {
    REG reg;
    uint32_t size() { return sizeof(*this); }
} WIN_IN;

typedef struct __attribute__((packed)) WIN_JMPZ : WIN {
    REG reg;
    uint32_t addr;
    uint32_t size() { return sizeof(*this); }
} WIN_JMPZ;

typedef struct __attribute__((packed)) WIN_JMPE : WIN {
    REG left;
    REG right;
    uint32_t addr;
    uint32_t size() { return sizeof(*this); }
} WIN_JMPE;

typedef struct __attribute__((packed)) WIN_JMPI : WIN {
    REG reg;
    uint32_t addr;
    IMMEDIATE value;
    uint32_t size() { return sizeof(REG) + REGISTERS_META[reg].size + sizeof(uint32_t); }
} WIN_JMPI;

typedef struct __attribute__((packed)) WIN_OUTI : WIN {
    uint8_t chr;
    uint32_t size() { return sizeof(*this); }
} WIN_OUTI;

typedef struct __attribute__((packed)) WIN_JMPR : WIN {
    REG addr;
    uint32_t size() { return sizeof(*this); }
} WIN_JMPR;

typedef struct __attribute__((packed)) WIN_SHFL : WIN {
    REG reg;
    uint8_t chr;
    uint32_t size() { return sizeof(*this); }
} WIN_SHFL;

typedef struct __attribute__((packed)) WIN_SHFR : WIN {
    REG reg;
    uint8_t chr;
    uint32_t size() { return sizeof(*this); }
} WIN_SHFR;

typedef struct __attribute__((packed)) WIN_JMPL : WIN {
    REG reg;
    uint32_t addr;
    IMMEDIATE value;
    uint32_t size() { return sizeof(REG) + REGISTERS_META[reg].size + sizeof(uint32_t); }
} WIN_JMPL;

typedef struct __attribute__((packed)) WIN_JMPS : WIN {
    REG reg;
    uint32_t addr;
    IMMEDIATE value;
    uint32_t size() { return sizeof(REG) + REGISTERS_META[reg].size + sizeof(uint32_t); }
} WIN_JMPS;

typedef struct __attribute__((packed)) WIN_SUB : WIN {
    REG dest;
    REG src;
    uint32_t size() { return sizeof(*this); }
} WIN_SUB;

typedef struct __attribute__((packed)) WIN_SUBI : WIN {
    REG dest;
    IMMEDIATE val;
    uint32_t size() { return sizeof(REG) + REGISTERS_META[dest].size; }
} WIN_SUBI;





// #########################
// Processor abstraction
// #########################




struct CORE {
    std::unordered_map<REG, REGISTER> registers;

    CORE() {
        for(auto pair : REGISTERS_META) {
            REGISTER reg = REGISTER(pair.second);
            registers[pair.first] = reg;
        }
    }

    void skip_args(uint32_t count) {
        registers[REG_PC].add(count + 1);
    }

    void clock(MACHINE* machine);
};

struct MACHINE {
    CORE core0;
    uint8_t* memory;
    bool halted = false;

    MACHINE(uint8_t* memory) : memory(memory) {}
};

// #########################
// Instruction abstractions
// #########################




using CLKFunction = std::function<void(MACHINE*, CORE*, uint8_t*)>;

// Create an unordered_map to associate opcodes with CLK functions as lambda functions
std::unordered_map<uint8_t, CLKFunction> opcodeToCLKFunction = {

    // NOOP
    { 0xff, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        core->skip_args(0);
    }},

    // HALT
    { 0x00, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: HALT\n");

        machine->halted = true;
        
        core->skip_args(0);
    }},

    // MVI
    { 0x01, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: MVI\n");

        WIN_MVI* window = (WIN_MVI*)mem;

        core->registers[window->dest].set(window->val);

        core->skip_args(window->size());
    }},

    // MOV
    { 0x02, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: MOV\n");

        WIN_MOV* window = (WIN_MOV*)mem;
        
        core->registers[window->dest].set(core->registers[window->src]);

        core->skip_args(window->size());
    }},

    // ADD
    { 0x03, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: ADD\n");

        WIN_ADD* window = (WIN_ADD*)mem;
        
        core->registers[window->dest].add(core->registers[window->src]);

        core->skip_args(window->size());
    }},

    // ADDI
    { 0x04, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: ADDI\n");

        WIN_ADDI* window = (WIN_ADDI*)mem;
        if(debug) printf("addi: before: %d (%x)\n", core->registers[window->dest].value.u32, core->registers[window->dest].value.u32);
        core->registers[window->dest].add(window->val);
        if(debug) printf("addi: after: %d (%x)\n", core->registers[window->dest].value.u32, core->registers[window->dest].value.u32);

        core->skip_args(window->size());
    }},

    // SHFL
    { 0x05, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: SHFL\n");

        WIN_SHFL* window = (WIN_SHFL*)mem;
        core->registers[window->reg].shfl(window->chr);

        core->skip_args(window->size());
    }},

    // SHFR
    { 0x06, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: SHFR\n");

        WIN_SHFR* window = (WIN_SHFR*)mem;
        core->registers[window->reg].shfr(window->chr);

        core->skip_args(window->size());
    }},

    // SUB
    { 0x07, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: SUB\n");

        WIN_SUB* window = (WIN_SUB*)mem;
        
        core->registers[window->dest].subtract(core->registers[window->src]);

        core->skip_args(window->size());
    }},

    // SUBI
    { 0x08, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: SUBI\n");

        WIN_SUBI* window = (WIN_SUBI*)mem;
        core->registers[window->dest].subtract(window->val);

        core->skip_args(window->size());
    }},

    // ========== 0x20 MEMORY

    // LDA
    { 0x20, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: LDA\n");
        
        const uint32_t addr = core->registers[REG_MAR].to_u32();
        uint32_t v = machine->memory[addr];
        if(debug) printf("got %d (%x) @ %d (%x)\n", v, v, addr, addr);
        core->registers[REG_MDR].set(v);

        core->skip_args(0);
    }},

    // STA
    { 0x21, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: STA\n");
        
        const uint32_t addr = core->registers[REG_MAR].to_u32();
        uint8_t v = core->registers[REG_MDR].value.u8;
        if(debug) printf("writing %d (%x) to %d (%x)\n", v, v, addr, addr);
        machine->memory[addr] = v;

        core->skip_args(0);
    }},

    // ========== 0x40 JUMPS

    // JMP
    { 0x40, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: JMP\n");

        WIN_JMP* window = (WIN_JMP*)mem;

        if(debug) printf("jumping to: %d (%x)\n", window->addr, window->addr);
        
        core->registers[REG_PC].set(window->addr);
    }},

    // JMPZ
    { 0x41, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: JMPZ\n");

        WIN_JMPZ* window = (WIN_JMPZ*)mem;

        if(debug) printf("reg: %d (%x)\n", core->registers[window->reg].to_u32(), core->registers[window->reg].to_u32());

        if(core->registers[window->reg].to_u32() == 0) {
            core->registers[REG_PC].set(window->addr);
            if(debug) printf("jumping to: %d (%x)\n", window->addr, window->addr);
            return;
        }
        
        if(debug) printf("skipped jump\n");
        core->skip_args(window->size());
    }},

    // JMPE
    { 0x42, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: JMPE\n");

        WIN_JMPE* window = (WIN_JMPE*)mem;

        if(debug) printf("left: %d (%x)\n", core->registers[window->left].to_u32(), core->registers[window->left].to_u32());
        if(debug) printf("right: %d (%x)\n", core->registers[window->right].to_u32(), core->registers[window->right].to_u32());

        if(core->registers[window->left].to_u32() == core->registers[window->right].to_u32()) {
            core->registers[REG_PC].set(window->addr);
            if(debug) printf("jumping to: %d (%x)\n", window->addr, window->addr);
            return;
        }
        
        if(debug) printf("skipped jump\n");
        core->skip_args(window->size());
    }},

    // JMPI
    { 0x43, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: JMPI\n");

        WIN_JMPI* window = (WIN_JMPI*)mem;

        if(debug) printf("reg: %d (%x)\n", core->registers[window->reg].to_u32(), core->registers[window->reg].to_u32());
        if(debug) printf("cmp value: %d (%x)\n", window->value.u32, window->value.u32);

        if(core->registers[window->reg].compare(window->value) == 0) {
            core->registers[REG_PC].set(window->addr);
            if(debug) printf("jumping to: %d (%x)\n", window->addr, window->addr);
            return;
        }
        
        if(debug) printf("skipped jump\n");
        core->skip_args(window->size());
    }},

    // JMPR
    { 0x44, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: JMPR\n");

        WIN_JMPR* window = (WIN_JMPR*)mem;

        if(debug) printf("reg: %d (%x)\n", core->registers[window->addr].to_u32(), core->registers[window->addr].to_u32());
        
        core->registers[REG_PC].set(core->registers[window->addr].to_u32());
    }},

    // JMPL
    { 0x43, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: JMPL\n");

        WIN_JMPL* window = (WIN_JMPL*)mem;

        if(core->registers[window->reg].compare(window->value) > 0) {
            core->registers[REG_PC].set(window->addr);
            return;
        }
        
        core->skip_args(window->size());
    }},

    // JMPS
    { 0x43, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: JMPS\n");

        WIN_JMPS* window = (WIN_JMPS*)mem;

        if(core->registers[window->reg].compare(window->value) < 0) {
            core->registers[REG_PC].set(window->addr);
            return;
        }
        
        core->skip_args(window->size());
    }},

    // ========== 0xe0-0xf0 IO/HID

    // OUT
    { 0xe0, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: OUT\n");

        WIN_OUT* window = (WIN_OUT*)mem;
        const uint8_t c = core->registers[window->reg].to_u32();
        if(debug) printf("printing out: %c (%d, %x)\n", c, c, c);
        putc(c, stdout);

        core->skip_args(window->size());
    }},

    // OUTI
    { 0xe1, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: OUTI\n");

        WIN_OUTI* window = (WIN_OUTI*)mem;
        const uint8_t c = window->chr;
        if(debug) printf("printing out: %c (%d, %x)\n", c, c, c);
        putc(c, stdout);

        core->skip_args(window->size());
    }},

    // IN
    { 0xf0, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: IN\n");

        WIN_IN* window = (WIN_IN*)mem;

        const uint8_t c = getc(stdin);
        if(debug) printf("got c: %c (%d, %x)\n", c, c, c);
        core->registers[window->reg].set(c);

        core->skip_args(window->size());
    }},

    // INV
    { 0xf1, [](MACHINE* machine, CORE* core, uint8_t* mem) {
        if(debug) printf("IN: INV\n");

        const uint8_t c = getc(stdin);
        if(debug) printf("got c: %c (%d, %x)\n", c, c, c);

        core->skip_args(0);
    }},
    


};



void CORE::clock(MACHINE* machine) {
    const uint32_t pc = this->registers[REG_PC].to_u32();
    uint8_t* relative_memory = &machine->memory[pc + 1];
    uint8_t instruction = machine->memory[pc];

    opcodeToCLKFunction[instruction](machine, this, relative_memory);
}

uint8_t* load_memory(const char* filename) {
    FILE *bin = fopen(filename, "r");
    fseek(bin, 0, SEEK_END); // get offset at end of file
    size_t size = ftell(bin);
    if(debug) printf("%lu bytes of data read\n", size);
    fseek(bin, 0, SEEK_SET); // seek back to beginning
    uint8_t* memory = (uint8_t*) malloc(size+1);
    memset(memory, 0, size+1);
    fread(memory, size, 1, bin);  // read in the file
    fclose(bin);
    return memory;
}

int main(int argc, char** argv) {

    // Arguments

    if(argc < 3) { printf("ERROR: Arguments: <memory filename> <clock cycles>\n"); return 1; }
    const char* filename = argv[1];
    const uint32_t clock_cycles = atoi(argv[2]);
    if(argc == 4) if(!strcmp(argv[3], "debug")) debug = 1;

    // Setting up virtual machine

    uint8_t* memory = load_memory(argv[1]);

    MACHINE machine = MACHINE(memory);

    // Starting clock loop

    for(uint32_t i = 0; i < clock_cycles && !machine.halted; i++) {
        if(debug) printf("cycle [%d], PC = %d (%x)\n", i, machine.core0.registers[REG_PC].to_u32(), machine.core0.registers[REG_PC].to_u32());
        machine.core0.clock(&machine);
        if(debug) {
            uint32_t v;
            v = machine.core0.registers[REG_A].value.u32;
            printf("REG_A: %d (%x), ", v, v);
            v = machine.core0.registers[REG_B].value.u32;
            printf("REG_B: %d (%x), ", v, v);
            v = machine.core0.registers[REG_C].value.u32;
            printf("REG_C: %d (%x), ", v, v);
            v = machine.core0.registers[REG_D].value.u32;
            printf("REG_D: %d (%x), ", v, v);
            v = machine.core0.registers[REG_CA].value.u8;
            printf("REG_CA: %d (%x), ", v, v);
            v = machine.core0.registers[REG_CB].value.u8;
            printf("REG_CB: %d (%x), ", v, v);
            v = machine.core0.registers[REG_PC].value.u32;
            printf("REG_PC: %d (%x), ", v, v);
            v = machine.core0.registers[REG_MAR].value.u32;
            printf("REG_MAR: %d (%x), ", v, v);
            v = machine.core0.registers[REG_MDR].value.u8;
            printf("REG_MDR: %d (%x)\n", v, v);
        }
    }

    free(memory);
    return 0;
}