#include <bits/stdint-uintn.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define INST_HALT   0x00    // Stop execution 

#define INST_MOV    0x01    // Rd = Rs
#define INST_MVI    0x02    // Rd = I

#define INST_LDA    0x03    // Rd = Ms
#define INST_STA    0x04    // Md = Rs

#define INST_ADD    0x05    // Rd = Rd + Rs
#define INST_ADDI   0x06    // Rd = Rd + I
#define INST_SUB    0x07    // Rd = Rd - Rs
#define INST_SUBI   0x08    // Rd = Rd - I
#define INST_MUL    0x09    // Rd = Rd * Rs
#define INST_MULI   0x0A    // Rd = Rd * I
#define INST_OR     0x0B    // Rd = Rd | Rs
#define INST_ORI    0x0C    // Rd = Rd | I
#define INST_AND    0x0D    // Rd = Rd & Rs
#define INST_ANDI   0x0E    // Rd = Rd & I
#define INST_XOR    0x0F    // Rd = Rd ^ Rs
#define INST_XORI   0x10    // Rd = Rd ^ I
#define INST_NOT    0x11    // Rd = !Rd
#define INST_INC    0x12    // Rd = Rd + 1
#define INST_DEC    0x13    // Rd = Rd - 1

#define INST_J      0x14    // Jump to
#define INST_JZ     0x15    // Jump to if zero-flag is 1
#define INST_JNZ    0x16    // Jump to if zero-flag is 0
#define INST_JF     0x17    // Jump forward 
#define INST_JFZ    0x18    // Jump forward if zero-flag is 1
#define INST_JFNZ   0x19    // Jump forward if zero-flag is 0
#define INST_JB     0x1A    // Jump backward 
#define INST_JBZ    0x1B    // Jump backward if zero-flag is 1
#define INST_JBNZ   0x1C    // Jump backward if zero-flag is 0

#define INST_POP    0x1D    // Pop register from the stack
#define INST_PUSH   0x1E    // Push register onto the stack
#define INST_GVAR   0x1F    // Get from stack after SP
#define INST_GARG   0x20    // Get from stack before SP
#define INST_SVAR   0x21    // Set in stack after SP
#define INST_SARG   0x22    // Set in stack before SP
#define INST_BUF    0x23    // Skip N bytes in stack (used to create arrays/buffers)

#define INST_PRNT 0x08

struct FLAGS {
    uint16_t zero:1;
    uint16_t ntzero:1;
    uint16_t carry:1;
    uint16_t odd:1;
    uint16_t sto:1;
    uint16_t stu:1;
};

#define SHORT_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"
#define SHORT_TO_BINARY(num)  \
  (num & 0x8000 ? '1' : '0'), \
  (num & 0x4000 ? '1' : '0'), \
  (num & 0x2000 ? '1' : '0'), \
  (num & 0x1000 ? '1' : '0'), \
  (num & 0x0800 ? '1' : '0'), \
  (num & 0x0400 ? '1' : '0'), \
  (num & 0x0200 ? '1' : '0'), \
  (num & 0x0100 ? '1' : '0'), \
  (num & 0x0080 ? '1' : '0'), \
  (num & 0x0040 ? '1' : '0'), \
  (num & 0x0020 ? '1' : '0'), \
  (num & 0x0010 ? '1' : '0'), \
  (num & 0x0008 ? '1' : '0'), \
  (num & 0x0004 ? '1' : '0'), \
  (num & 0x0002 ? '1' : '0'), \
  (num & 0x0001 ? '1' : '0') 


struct STATE {
    //   0000000000000000    
    //   ||||||||||||||||
    // 0 |||||||||||||||└ zero
    // 1 ||||||||||||||└ not-zero
    // 2 |||||||||||||└ carry out
    // 3 ||||||||||||└ odd
    // 4 |||||||||||└ even
    // 5 ||||||||||└ stack overflow ( unimplemented )
    // 6 |||||||||└ stack underflow ( unimplemented )
    // 7 ||||||||└ 
    // 8 |||||||└ 
    // 9 ||||||└ 
    // a |||||└ 
    // b ||||└ 
    // c |||└ 
    // d ||└ 
    // e |└ 
    // f └ 

    // uint16_t flags;
    struct FLAGS flags;
    
    // General purpose registers
    uint8_t  a;
    uint8_t  x;
    uint16_t i;

    uint8_t  b;
    uint8_t  y;
    uint16_t j;
    
    uint8_t  c;
    uint8_t  z;
    uint16_t k;
    
    uint8_t  d;
    uint8_t  w;
    uint16_t l;
    
    // Memory registers
    uint32_t pc;

    uint32_t mar;
    uint8_t mdr;
    uint32_t sp;
    uint32_t sf;

    uint8_t *memory;
};

enum DTYPE {
    I8 = 0, 
    I16 = 1, 
    I32 = 2 
};

struct REGISTER {
    enum DTYPE type;

    void* value;
};

void clock(struct STATE *state);

struct REGISTER referenceRegister(struct STATE *state, uint8_t reg);
void copyRegisterValue(struct STATE *state, struct REGISTER dest, struct REGISTER src);
void setRegisterValue(struct REGISTER dest, uint32_t val);
uint32_t getRegisterValue(struct REGISTER src);
struct REGISTER getRegister(struct STATE *state, uint8_t reg);
void updateArithmeticFlags(struct STATE *state, enum DTYPE type, uint32_t val);
void updateNumberFlags(struct STATE *state, enum DTYPE type, uint32_t val);

uint8_t *memory;
struct STATE *state;

int main(void) {
    FILE *bin = fopen("test.img", "r");

    // Create memory
    uint32_t memory_t = 4096;
    // memory_t = (fgetc(bin) << 24) | (fgetc(bin) << 16) | (fgetc(bin) << 8) | (fgetc(bin));

    printf("allocating %u bytes of memory\n", memory_t);

    // Read entire file into memory
    fseek(bin, 0, SEEK_END); // get offset at end of file
    size_t size = ftell(bin);
    printf("%lu bytes of data read\n", size);
    fseek(bin, 0, SEEK_SET); // seek back to beginning
    memory = malloc(size+1);
    memset(memory, 0, size+1);
    fread(memory, size, 1, bin);  // read in the file
    fclose(bin);
    
    state = malloc(sizeof(struct STATE));
    memset(state, 0, sizeof(struct STATE));
    state->memory = memory;

    // Start executing instructions
    for(int i = 0; i < 100; i ++) {
        clock(state);
    }

    free(state->memory);
}

void clock(struct STATE *state) {
    uint8_t *instruction = &state->memory[state->pc];

    // Print current flag register (bin)
    uint16_t flagsRegister = *(uint16_t*)&state->flags;
    printf("flags: ");
    printf(SHORT_TO_BINARY_PATTERN, SHORT_TO_BINARY(flagsRegister));
    printf("\n");
    // Print current instruction code (hex)
    printf("%02x: %02x\n", state->pc, *instruction);

    switch (*instruction) {
        // HALT
        case INST_HALT:
            exit(0);
            break;

        // MOV
        case INST_MOV:
            copyRegisterValue(
                state,
                referenceRegister(state, *(instruction+1)),
                referenceRegister(state, *(instruction+2))
            );
            state->pc += 3;
            break;
        // MVI
        case INST_MVI:
            {
                struct REGISTER reg;
                uint32_t val;

                setRegisterValue(
                    reg = referenceRegister(state, *(instruction+1)),
                    val = *(uint32_t*)(instruction+2)
                );

                updateNumberFlags(state, reg.type, val);

                state->pc += 6;
            }
            break;

        // LDA
        case INST_LDA:
            state->mdr = memory[state->mar];
            state->pc += 1;
            break;
        // STA
        case INST_STA:
            memory[state->mar] = state->mdr;
            state->pc += 1;
            break;

        // ADD
        case INST_ADD:
            {
                struct REGISTER reg;
                uint32_t val;

                setRegisterValue(
                    reg = referenceRegister(state, 0x00),
                    val = (
                        getRegisterValue(
                            referenceRegister(
                                state,
                                0
                            )
                        ) + getRegisterValue(
                            referenceRegister(
                                state,
                                *(instruction+1)
                            )
                        )
                    )
                );
                
                updateArithmeticFlags(state, reg.type, val);

                state->pc += 2;
            }
            break;
        // SUB
        case INST_SUB:
            {
                struct REGISTER reg;
                uint32_t val;

                setRegisterValue(
                    reg = referenceRegister(state, 0x00),
                    val = (
                        getRegisterValue(
                            referenceRegister(
                                state,
                                0
                            )
                        ) - getRegisterValue(
                            referenceRegister(
                                state,
                                *(instruction+1)
                            )
                        )
                    )
                );
                
                updateArithmeticFlags(state, reg.type, val);

                state->pc += 2;
            }
            break;
        // MUL
        case INST_MUL:
            {
                struct REGISTER reg;
                uint32_t val;
                uint32_t castVal;

                setRegisterValue(
                    reg = referenceRegister(state, 0x00),
                    val = (
                        getRegisterValue(
                            referenceRegister(
                                state,
                                0
                            )
                        ) * getRegisterValue(
                            referenceRegister(
                                state,
                                *(instruction+1)
                            )
                        )
                    )
                );

                switch (reg.type) {
                    case I8:
                        castVal = (uint8_t) val;
                        break;
                    case I16:
                        castVal = (uint16_t) val;
                        break;
                    case I32:
                        castVal = (uint32_t) val;
                        break;

                    default: exit(1); break;
                }

                state->flags.zero = val == 0;
                state->flags.ntzero = val != 0;
                state->flags.odd = val & 0x1;
                state->flags.carry = val > castVal;

                state->pc += 2;
            }
            break;

        // PRNT
        case INST_PRNT:
            printf("out: %02x\n", 
                getRegisterValue(
                    referenceRegister(
                        state,
                        *(instruction+1)
                    )
                )
            );
            state->pc += 2;
            break;
    }
}

struct REGISTER referenceRegister(struct STATE *state, uint8_t reg) {
    switch (reg) {
        case 0x00:  return (struct REGISTER) { I8, (uint32_t*)&state->a };         break;
        case 0x01:  return (struct REGISTER) { I8, (uint32_t*)&state->b };         break;
        case 0x02:  return (struct REGISTER) { I8, (uint32_t*)&state->c };         break;
        case 0x03:  return (struct REGISTER) { I8, (uint32_t*)&state->d };         break;

        case 0x04:  return (struct REGISTER) { I8, (uint32_t*)&state->x };         break;
        case 0x05:  return (struct REGISTER) { I8, (uint32_t*)&state->y };         break;
        case 0x06:  return (struct REGISTER) { I8, (uint32_t*)&state->z };         break;
        case 0x07:  return (struct REGISTER) { I8, (uint32_t*)&state->w };         break;

        case 0x08:  return (struct REGISTER) { I16, (uint32_t*)&state->i };        break;
        case 0x09:  return (struct REGISTER) { I16, (uint32_t*)&state->j };        break;
        case 0x0a:  return (struct REGISTER) { I16, (uint32_t*)&state->k };        break;
        case 0x0b:  return (struct REGISTER) { I16, (uint32_t*)&state->l };        break;

        case 0x0c:  return (struct REGISTER) { I16, (uint32_t*)&state->a };        break;
        case 0x0d:  return (struct REGISTER) { I16, (uint32_t*)&state->b };        break;
        case 0x0e:  return (struct REGISTER) { I16, (uint32_t*)&state->c };        break;
        case 0x0f:  return (struct REGISTER) { I16, (uint32_t*)&state->d };        break;

        case 0x10:  return (struct REGISTER) { I32, (uint32_t*)&state->a };        break;
        case 0x11:  return (struct REGISTER) { I32, (uint32_t*)&state->b };        break;
        case 0x12:  return (struct REGISTER) { I32, (uint32_t*)&state->c };        break;
        case 0x13:  return (struct REGISTER) { I32, (uint32_t*)&state->d };        break;

        case 0x14:  return (struct REGISTER) { I8,  (uint32_t*)&state->flags};     break;
        case 0x15:  return (struct REGISTER) { I32, (uint32_t*)&state->pc   };     break;
        case 0x16:  return (struct REGISTER) { I32, (uint32_t*)&state->mar  };     break;
        case 0x17:  return (struct REGISTER) { I8,  (uint32_t*)&state->mdr  };     break;
        case 0x18:  return (struct REGISTER) { I32, (uint32_t*)&state->sp   };     break;
        case 0x19:  return (struct REGISTER) { I32, (uint32_t*)&state->sf   };     break;
        
        default: exit(1); break;
    }
}

void copyRegisterValue(struct STATE *state, struct REGISTER dest, struct REGISTER src) {
    if(src.type > dest.type) {
        src.type = dest.type;
    }
    
    // Read value from src
    uint32_t val = 0;
    switch (src.type) {
        case I8:
            val = *(uint8_t*) src.value;
            break;

        case I16:
            val = *(uint16_t*) src.value;
            break;

        case I32:
            val = *(uint32_t*) src.value;
            break;
        
        default: exit(1); break;
    }

    // Write value to dest
    uint8_t* destI8  = (uint8_t*) dest.value;
    uint16_t* destI16 = (uint16_t*) dest.value;
    uint32_t* destI32 = (uint32_t*) dest.value;
    switch (dest.type) {
        case I8:
            *destI8 = val;
            updateNumberFlags(state, I8, val);
            break;

        case I16:
            *destI16 = val;
            updateNumberFlags(state, I16, val);
            break;

        case I32:
            *destI32 = val;
            updateNumberFlags(state, I32, val);
            break;
        
        default: exit(1); break;
    }
}

void setRegisterValue(struct REGISTER dest, uint32_t val) {
    // Write value to dest
    uint8_t* destI8  = (uint8_t*) dest.value;
    uint16_t* destI16 = (uint16_t*) dest.value;
    uint32_t* destI32 = (uint32_t*) dest.value;

    switch (dest.type) {
        case I8:
            *destI8 = val;
            break;

        case I16:
            *destI16 = val;
            break;

        case I32:
            *destI32 = val;
            break;
        
        default: exit(1); break;
    }
}

uint32_t getRegisterValue(struct REGISTER src) {
    // Read valeu from src
    switch (src.type) {
        case I8:
            return *(uint8_t*) src.value;
            break;

        case I16:
            return *(uint16_t*) src.value;
            break;

        case I32:
            return *(uint32_t*) src.value;
            break;
        
        default: exit(1); break;
    }
}

void updateArithmeticFlags(struct STATE *state, enum DTYPE type, uint32_t val) {
    uint32_t castVal = 0;

    switch (type) {
        case I8:
            castVal = (uint8_t) val;
            break;
        case I16:
            castVal = (uint16_t) val;
            break;
        case I32:
            castVal = (uint32_t) val;
            break;

        default: exit(1); break;
    }

    state->flags.zero = val == 0;
    state->flags.ntzero = val != 0;
    state->flags.odd = val & 0x1;
    state->flags.carry = val > castVal;
}

void updateNumberFlags(struct STATE *state, enum DTYPE type, uint32_t val) {
    uint32_t castVal = 0;

    switch (type) {
        case I8:
            castVal = (uint8_t) val;
            break;
        case I16:
            castVal = (uint16_t) val;
            break;
        case I32:
            castVal = (uint32_t) val;
            break;

        default: exit(1); break;
    }

    state->flags.zero = val == 0;
    state->flags.ntzero = val != 0;
    state->flags.odd = val & 0x1;
}
