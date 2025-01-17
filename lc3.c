#include <stdint.h>
#include <stdio.h>

#define MEMORY_MAX (1 << 16) //bitwise shift of 1 left by 16 positions, resulting in 10000000000000000(b) or 65536(d)
uint16_t memory[MEMORY_MAX]; 

//----- define CPU registers; 8 general, 1 for PC, 1 condition flag
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

//----- define COND flags
enum {
    FL_POS = 1 << 0, // Positive (P) i.e. 0**1 i.e. 1
    FL_ZRO = 1 << 1, // Zero (Z) i.e. 0*10 i.e. 2
    FL_NEG = 1 << 2, // Negative (N) i.e. *100 i.e. 4
};

//----- define avaialble opcodes
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

//----- define trap codes
enum {
    TRAP_GETC = 0x20, //get char from keyboard - no echo to terminal
    TRAP_OUT = 0x21, // output character
    TRAP_PUTS = 0x22, // output a word string
    TRAP_IN = 0x23, //get char from keyboard, echoed onto terminal
    TRAP_PUTSP = 0x24, // output byte string
    TRAP_HALT = 0x25 // halt program
};


//----- sign extension function -> used to fill up bits in a register for a given smaller signed int, while preserving sign
uint16_t sign_extend(uint16_t x, int bit_count){
    if ((x >> (bit_count - 1)) & 1){ // extract most significant bit of x
        x |= (0xFFFF << bit_count); // fill up the rest of the register with 1's if 1 above
    } // rest of register is already 0s if the loop gets skipped 
    return x;
}

//----- function to update flags to indicate sign of any value written to a register r
void update_flags(uint16_t r){
    if (reg[r] == 0){
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r] >> 15){ //bit shift MSB to LSB -> if this is one, there was a 1 in the leftmost register, indicating negative
        reg[R_COND] = FL_NEG;
    }
    else{ // not zero or negative
        reg[R_COND] = FL_POS;
    }
}

void read_image_file(FILE* file){
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t* p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    while (read-- > 0){
        *p = swap16(*p);
        ++p;
    }

}




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


        // TODO implement instruction logic (from spec)
        switch (op){
            case OP_ADD:
                {
                    // spec: contents (high bit position - low bit position)
                    //  0001(bits 15-12) DR(bits 11-9) SR1(bits 8-6) (OP_ADD will always have bits 15-12 be 0001)
                    //  followed by 0 or 1 for register mode or immediate mode at bit position 5
                    //   register mode: 00 (bits 4-3) then SR2 (bits 2-0)
                    //   immediate mode: imm5 (bits 4-0)   
                    uint16_t r0 = (instr >> 9) & 0x7; //DR - destination register
                    uint16_t r1 = (instr >> 6) & 0x7; //SR1 - source register 1 i.e. first operand
                    uint16_t imm_flag = (instr >> 5) & 0x1; // immediate mode 1 or register mode 0
                    if (imm_flag){
                        uint16_t imm5 = sign_extend(instr & 0x15, 5); //if immediate mode, sign extend the 5-bit number to 16 so we can add it to SR1
                        reg[r0] = reg[r1] + imm5;
                    } 
                    else {
                        uint16_t r2 = instr & 0x7; //if register mode we just get SR2 and add it, it's already 16 bit so no sign extension
                        reg[r0] = reg[r1] + reg[r2];
                    }
                    update_flags(r0);
                }
                break;
            
            case OP_AND: // bitwise and
                {
                    uint16_t r0 = (instr >> 9) & 0x7; // DR
                    uint16_t r1 = (instr >> 6) & 0x7; // SR
                    uint16_t imm_flag = (instr >> 5) & 0x1; //immediate mode or SR2

                    if (imm_flag){
                        uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                        reg[r0] = reg[r1] & imm5;
                    } 
                    else {
                        uint16_t r2 = instr & 0x7;
                        reg[r0]= reg[r1] & reg[r2];
                    }
                    update_flags(r0);
                }
                break;

            case OP_NOT: //bitwise not
                {
                    uint16_t r0 = (instr >> 9) & 0x7; //DR
                    uint16_t r1 = (instr >> 6) & 0x7; //SR
                    reg[0] = ~reg[r1]; // ~ is just a bitwise NOT
                    update_flags(r0);
                }
                break;

            case OP_BR: // branch
                {
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9); // get pc_offset from instruction register
                    // this is the relative distance to the target instruction
                    uint16_t cond_flag = (instr >> 9) & 0x7; // get cond flag from instruction register
                    if (cond_flag & reg[R_COND]){
                        reg[R_PC] += pc_offset;
                    }
                }
                break;

            case OP_JMP: //jump to given register; this handles RET too hence RET being blank later. RET is a specialized case of JMP when r1 is 7
                {
                    uint16_t r1 = (instr >> 6) & 0x7;
                    reg[R_PC] = reg[r1];
                }
                break;

            case OP_JSR: // jump to subroutine
            // stores the return address then bramches to the target subroutine
                {
                    uint16_t long_flag = (instr >> 11) & 1; // flag for if we use long PC offset or a specified register as the target for the subroutine
                    reg[R_R7] = reg[R_PC];

                    if (long_flag){ // if target subroutine is based on a long PC offset
                        uint16_t long_pc_offset = sign_extend(instr & 0x7FF, 11);
                        reg[R_PC] += long_pc_offset;
                    }
                    else { // target subroutine register defined as a register-based target
                        uint16_t r1 = (instr >> 6) & 0x7;
                        reg[R_PC] = reg[r1];
                    }
                }
                break;

            case OP_LD:
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    reg[r0] = mem_read(reg[R_PC] + pc_offset);
                    update_flags(r0);
                }
                break;

            case OP_LDI:
                
                //load indirect - just takes a value from memory into a register
                //encoding: 15-12: 1010 for opcode
                //          11-9: DR (destination register)
                //          8-0: PC offset PCoffset9  
                {
                    uint16_t r0 = (instr >> 9) & 0x7; //DR
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
                    update_flags(r0);
                }
                break;

            case OP_LDR:
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t r1 = (instr >> 6) & 0x7;
                    uint16_t offset = sign_extend(instr & 0x3F, 6);
                    reg[r0] = mem_read(reg[r1] + offset);
                    update_flags(r0);
                }
                break;

            case OP_LEA:
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    reg[r0] = reg[R_PC] + pc_offset;
                    update_flags(r0);
                }
                break;

            case OP_ST:
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    mem_write(reg[R_PC] + pc_offset, reg[r0]);
                }
                break;

            case OP_STI:
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    mem_write(mem_read(reg[R_PC] + pc_offset), reg[r0]);
                }
                break;

            case OP_STR:
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t r1 = (instr >> 6) & 0x7;
                    uint16_t offset = sign_extend(instr & 0x3F, 6);
                    mem_write(reg[r1] + offset, reg[r0]);
                }
                break;

            case OP_TRAP:
                {
                    reg[R_R7] = reg[R_PC];

                    switch (instr & 0xFF)
                    {
                        case TRAP_GETC:
                            {
                                reg[R_R0] = (uint16_t)getchar();
                                update_flags(R_R0);
                            }
                            break;
                        
                        case TRAP_OUT:
                            {
                                putc((char)reg[R_R0],stdout);
                                fflush(stdout);
                            }
                            break;

                        case TRAP_PUTS:
                            {
                                uint16_t* c = memory + reg[R_R0];
                                while (*c){
                                    putc((char)*c, stdout);
                                    ++c;
                                }
                                fflush(stdout);
                            }
                            break;
                        
                        case TRAP_IN:
                            {
                                printf("Enter a character: ");
                                char c = getchar();
                                putc(c,stdout);
                                fflush(stdout);
                                reg[R_R0] = (uint16_t)c;
                                update_flags(R_R0);
                            }
                            break;

                        case TRAP_PUTSP:
                            {
                                uint16_t* c = memory + reg[R_R0];
                                while (*c){
                                    char char1 = (*c) & 0xFF;
                                    putc(char1, stdout);
                                    char char2 = (*c) >> 8;
                                    if (char2) putc(char2, stdout);
                                    ++c;
                                }
                                fflush(stdout);
                            }
                            break;

                        case TRAP_HALT:
                            {
                                puts("HALT");
                                fflush(stdout);
                                running = 0;
                            }
                            break;
                    }
                }
                break;

            case OP_RES:
                abort(); // unused in spec

            case OP_RTI:
                abort(); // unused in spec

            default:
                // logic for bad opcode needs to go here in default
                break;
        }
    }
    // shutdown logic here
}