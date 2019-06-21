#include "cpu.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DATA_LEN 6
#define SP 7

unsigned char cpu_ram_read(struct cpu *cpu, unsigned char address) {
  return cpu->ram[address];
}

unsigned char cpu_ram_write(struct cpu *cpu, unsigned char address, unsigned char value) {
  return cpu->ram[address] = value;
}

unsigned char pop(struct cpu *cpu) {
  unsigned char value = cpu->ram[cpu->registers[SP]];

  cpu->registers[SP]++;
  return value;
}

void push(struct cpu *cpu, unsigned char value) {
  cpu->registers[SP]--;
  cpu_ram_write(cpu, cpu->registers[SP], value);
}

/**
 * Load the binary bytes from a .ls8 source file into a RAM array
 */
void cpu_load(struct cpu *cpu, int argc, char *argv[]) {
  FILE *fp;
  char line[128];

  if (argc != 2) {
    printf("usage: ls8 folder/filename\n");
    exit(1);
  }

  fp = fopen(argv[1], "r");

  if (fp == NULL) {
    printf("file %s does not exist\n", argv[1]);
    exit(1);
  }

  int address = 0;

  while (fgets(line, sizeof(line), fp) != NULL) {
    char *endptr;
    unsigned char value = strtoul(line, &endptr, 2);

    if (line == endptr) {
      continue;
    }

    cpu_ram_write(cpu, address, value);
    address++;
  }

  fclose(fp);
}

/**
 * ALU
 */
void alu(struct cpu *cpu, enum alu_op op, unsigned char regA, unsigned char regB) {
  switch (op) {
    case ALU_MUL:
      cpu->registers[regA] *= cpu->registers[regB];
      break;

    case ALU_ADD:
      cpu->registers[regA] += cpu->registers[regB];
      break;
  }
}

/**
 * Run the CPU
 */
void cpu_run(struct cpu *cpu) {
  int running = 1; // True until we get a HLT instruction

  while (running) {
    // 1. Get the value of the current instruction (in address PC).
    unsigned char IR = cpu_ram_read(cpu, cpu->PC);

    // 2. Figure out how many operands this next instruction requires
    unsigned int operands = (IR >> 6) + 1;

    // 3. Get the appropriate value(s) of the operands following this instruction
    unsigned char operandA = cpu_ram_read(cpu, (cpu->PC + 1) & 0xFF);
    unsigned char operandB = cpu_ram_read(cpu, (cpu->PC + 2) & 0xFF);

    // 4. switch() over it to decide on a course of action.
    switch (IR) {

      // 5. Do whatever the instruction should do according to the spec.
      case HLT:
        running = 0;
        break;

      case LDI:
        cpu->registers[operandA] = operandB;
        break;

      case PRN:
        printf("%d\n", cpu->registers[operandA]);
        break;

      case MUL:
        alu(cpu, ALU_MUL, operandA, operandB);
        break;

      case POP:
        cpu->registers[operandA] = pop(cpu);
        break;

      case PUSH:
        push(cpu, cpu->registers[operandA]);
        break;

      case ADD:
        alu(cpu, ALU_ADD, operandA, operandB);
        break;

      case CALL:
        push(cpu, cpu->PC + 2);
        cpu->PC = cpu->registers[operandA];
        operands = 0;
        break;

      case RET:
        cpu->PC = pop(cpu);
        operands = 0;
        break;
      
      default:
        printf("unrecognized instruction\n");
        exit(1);
        break; 
    }

    // 6. Move the PC to the next instruction.
    cpu->PC += operands;
  }
}

/**
 * Initialize a CPU struct
 */
void cpu_init(struct cpu *cpu) {
  // Initialize the PC and other special registers
  cpu->PC = 0;
  cpu->FL = 0;

  // Zero registers and RAM
  memset(cpu->ram, 0, sizeof(cpu->ram));
  memset(cpu->registers, 0, sizeof(cpu->registers));

  // Initialize SP (stack pointer)
  cpu->registers[7] = 0xF4;
}
