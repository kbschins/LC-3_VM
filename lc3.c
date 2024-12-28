#include <stdint.h>

#define MEMORY_MAX (1 << 16) //bitwise shift of 1 left by 16 positions, resulting in 10000000000000000(b) or 65536(d)
uint16_t memory[MEMORY_MAX]; 

// define CPU registers; 8 general, 1 for PC, 1 condition flag
enum {
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC, // program counter
    R_COND, // condition flag
    R_COUNT // R_COUNT is not a register, it is the number of registers we have
};
uint16_t reg[R_COUNT]; // store registers in array

// define COND flags
enum {
    FL_POS = 1 << 0, // Positive (P) i.e. 0**1 i.e. 1
    FL_ZRO = 1 << 1, // Zero (Z) i.e. 0*10 i.e. 2
    FL_NEG = 1 << 2, // Negative (N) i.e. *100 i.e. 4
};

// define avaialble opcodes
// each opcode is 16 bits = leftmost 4 bits are the opcode itself, remaining 12 are params
enum {
    OP_BR = 0, // tranfser control based on cond code
    OP_ADD, // addition operation between 2 registers or register and constant
    OP_LD, // load from memory to destination register
    OP_ST, // store into memory from given register
    OP_JSR, // "jump to subroutine" save PC to link register and jumps to new address
    OP_AND, // bitwise and
    OP_LDR, // load register
    OP_STR, // store register
    OP_RTI, // "return from interrupt"
    OP_NOT, // bitwise not
    OP_LDI, // "load indirect"
    OP_STI, // "store indirect"
    OP_JMP, // "jump" i.e. direct address transfer
    OP_RES, // reserved, don't think this will get used
    OP_LEA, // "load effective address"
    OP_TRAP // executes trap routine; trap vector specifies what to do
}; 

int main(int argc, const char* argv[]){

    // need to check if we have enough args (argc>2) this will result in the program having at least one memory image file argv[1]
    if (argc < 2) {
        printf("lc3 [image-file1] ...\n");
        exit(2);
    }

    // start at 1 to skip argv[0] = program name; attempt to lead the image file using read_image
    // where image files are some 
    for (int j = 1; j < argc; ++j){
        if (!read_image(argv[j])){
            printf("failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }

    //TODO setup

    // need to have the COND flag be zero before loading PC into register
    // to avoid improper init state
    reg[R_COND] = FL_ZRO; 

    // set program counter to the default starting position of 0x3000
    enum { PC_START = 0x3000 };
    reg[R_PC] = PC_START;

    int running = 1;
    while (running) {
        // fetch
        uint16_t instr = mem_read(reg[R_PC]++); // get instruction in PC (instr=) and increment to next instruction
        uint16_t op = instr >> 12; // bitwise shift to the right by 12, so that the opcode now occupies the upper 4 bits


        // TODO implement instruction logic 
        switch (op){
            case OP_ADD:
            // logic for each opcode needs to repalce the comments in here, just building out structure for now
                break;
            case OP_AND:
            //
                break;
            case OP_NOT:
            //
                break;
            case OP_BR:
            //
                break;
            case OP_JMP:
            //
                break;
            case OP_JSR:
            //
                break;
            case OP_LD:
            //
                break;
            case OP_LDI:
            //
                break;
            case OP_LDR:
            //
                break;
            case OP_LEA:
            //
                break;
            case OP_ST:
            //
                break;
            case OP_STI:
            //
                break;
            case OP_STR:
            //
                break;
            case OP_TRAP:
            //
                break;
            case OP_RES:
            case OP_RTI:
            default:
                // logic for bad opcode needs to go here in default
                break;
        }
    }
    // shutdown logic here
}