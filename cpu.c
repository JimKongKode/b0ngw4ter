#include "cpu.h"
static inline void set_flag(gb_cpu_t *cpu, uint8_t bit, bool status);

void init_cpu(gb_cpu_t *cpu, gb_display_t *display) {
	for (int i = 0; i < 0x10000; i++) {
		cpu->addressSpace[i] = 0;
	}
	cpu->ram = cpu->addressSpace + 0xC000;
	cpu->display = display;
	cpu->cycles = 0;
	cpu->run_mode = 0;
	cpu->halt = 0;
	cpu->pc = 0;

	uint8_t *registers[] = {&cpu->a, &cpu->f, &cpu->b, &cpu->c, &cpu->d, &cpu->e, &cpu->h, &cpu->l};
	cpu->af = &cpu->a;
	cpu->bc = &cpu->b;
	cpu->de = &cpu->d;
	cpu->hl = &cpu->h;
	for (int i = 0; i < 8; i++)
		*registers[i] = 0;
}

uint32_t fetch_opcode(gb_cpu_t *cpu, uint8_t *instructionSize) {

	static uint8_t _cycle_count_table_1byte[16][16] = {
		{4, 12, 8, 8, 4, 4, 8, 4, 20, 8, 8, 8, 4, 4, 8, 4},
		{4, 12, 8, 8, 4, 4, 8, 4, 12, 8, 8, 8, 4, 4, 8, 4},
		{12|8, 12, 8, 8, 4, 4, 8, 4, 12|8, 8, 8, 8, 4, 4, 8, 4},
		{12|8, 12, 8, 8, 12, 12, 12, 4, 12|8, 8, 8, 8, 4, 4, 8, 4},
		{4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4},
		{4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4},
		{4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4},
		{4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 8, 4},
		{4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4},
		{4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4},
		{4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4},
		{4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4},
		{20|8, 12, 16|12, 16, 24|12, 16, 8, 16, 20|8, 16, 16|12, 4, 24|12, 24, 8, 16},
		{20|8, 12, 16|12, 0, 24|12, 16, 8, 16, 20|8, 16, 16|12, 0, 24|12, 0, 8, 16},
		{12, 12, 8, 0, 0, 16, 8, 16, 16, 4, 16, 0, 0, 0, 8, 16},
		{12, 12, 8, 4, 0, 16, 8, 16, 12, 8, 16, 4, 0, 0, 8, 16}
	};

	static uint8_t _instruction_byte_size[16][16] = {
		{1, 3, 1, 1, 1, 1, 2, 1, 3, 1, 1, 1, 1, 1, 2, 1},
		{2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1},
		{2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1},
		{2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1},
		{1, 1, 3, 0, 3, 1, 2, 1, 1, 1, 3, 0, 3, 0, 2, 1},
		{2, 1, 2, 0, 0, 1, 2, 1, 2, 1, 3, 0, 0, 0, 3, 1},
		{2, 1, 2, 1, 0, 1, 2, 1, 2, 1, 3, 1, 0, 0, 2, 1}
	};

	uint8_t opcode = cpu->addressSpace[cpu->pc];
	uint8_t opcodeHeader = opcode >> 4; // 0x0011
	uint8_t opcodeFooter = opcode & 0xf;
	*instructionSize = _instruction_byte_size[opcodeHeader][opcodeFooter];
	uint32_t final = opcode << 24; //0011 0000 0000 0000

	if (*instructionSize > 1) {
		final |= cpu->addressSpace[cpu->pc + 1] << 16;
		if (*instructionSize > 2)
			final |= cpu->addressSpace[cpu->pc + 2] << 8;
	}

	cpu->pc += *instructionSize;
	return final;
}

void execute_instruction(gb_cpu_t *cpu, uint32_t instruction) {
	if (instruction & 0xff00 == 0xcb00)
		_execute_prefix_instruction(cpu, instruction & 0x00ff0000 >> 16);
	else
		_execute_instruction(cpu, instruction);
}

static inline void set_flag(gb_cpu_t *cpu, uint8_t bit, bool status) {
	cpu->f ^= (-status ^ cpu->f) & (1UL << bit);	
}
static inline bool get_flag_on(gb_cpu_t *cpu, uint8_t bit) {
	return (cpu->f >> bit) & 1U;
}
static inline bool get_bit_on(uint8_t value, uint8_t bit) {
	return (value >> bit) & 1U;
}

static void _stop() {
	//halt cpu & lcd display until button pressed
	for (;;) {

	}
}
static void _execute_instruction(gb_cpu_t *cpu, uint32_t instruction) {
	register uint8_t n1 = (instruction & 0xf0000000) >> 28;
	register uint8_t n2 = (instruction & 0x0f000000) >> 24;

	uint16_t immw = instruction & 0x00ffff00 >> 8;
	uint8_t imm1 = instruction & 0x00ff0000 >> 16;
	//uint8_t imm2 = instruction & 0x0000ff00 >> 8;
	int8_t simm1 = (int8_t)imm1;
	// https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html

	switch (n1) {
		case 0x0: {
			switch (n2) {
				case 0x0: {
					//nop
					break;
				}
				case 0x1: {
					//ld bc, d16
					*cpu->bc = immw;
					break;
				}
				case 0x2: {
					*(cpu->addressSpace + *cpu->bc) = cpu->a;
					break;
				}
				case 0x3: {
					(*cpu->bc)++;
					break;
				}
				case 0x4: {
					cpu->b++;
					break;
				}
				case 0x5: {
					cpu->b--;
					break;
				}
				case 0x6: {
					cpu->b = imm1;
					break;
				}
				case 0x7: {
					//RLCA

					set_flag(cpu, C, get_bit_on(cpu->a, 7));
					set_flag(cpu, H, 0);
					set_flag(cpu, N, 0);
					cpu->a << 1;
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x8: {
					*(cpu->addressSpace + immw) = cpu->sp;
					break;
				}
				case 0x9: {
					*cpu->hl += *cpu->bc;
					break;
				}
				case 0xA: {
					cpu->a = *(cpu->addressSpace + *cpu->bc);
					break;
				}
				case 0xB: {
					(*cpu->bc)--;
					break;
				}
				case 0xC: {
					cpu->c++;
					break;
				}
				case 0xD: {
					cpu->c--;
					break;
				}
				case 0xE: {
					cpu->c = imm1;
					break;
				}
				case 0xF: {
					//RRCA
					set_flag(cpu, C, get_bit_on(cpu->a, 0));
					set_flag(cpu, H, 0);
					set_flag(cpu, N, 0);
					cpu->a >>= 1;
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
			}
			break;
		}
		case 0x1: {
			switch (n2) {
				case 0x0: {
					_stop();
					break;
				}
				case 0x1: {
					*cpu->de = immw;
					break;
				}
				case 0x2: {
					*(cpu->addressSpace + *cpu->de) = cpu->a;
					break;
				}
				case 0x3: {
					(*cpu->de)++;
					break;
				}
				case 0x4: {
					cpu->d++;
					break;
				}
				case 0x5: {
					cpu->d--;
					break;
				}
				case 0x6: {
					cpu->d = imm1;
					break;
				}
				case 0x7: {
					set_flag(cpu, C, get_bit_on(cpu->a, 7));
					set_flag(cpu, N, 0);
					set_flag(cpu, H, 0);
					cpu->a <<= 1;
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x8: {
					cpu->pc += simm1;
					break;
				}
				case 0x9: {
					*cpu->hl += *(cpu->de);
					break;
				}
				case 0xA: {
					cpu->a = *(cpu->addressSpace + *cpu->bc);
					break;
				}
				case 0xB: {
					(*cpu->bc)--;
					break;
				}
				case 0xC: {
					set_flag(cpu, N, 0);
					set_flag(cpu, H, 0);
					cpu->c++;
					set_flag(cpu, Z, cpu->c == 0);
					break;
				}
				case 0xD: {
					set_flag(cpu, N, 1);
					cpu->e--;
					set_flag(cpu, Z, cpu->e == 0);
					break;
				}
				case 0xE: {
					cpu->e = imm1;
					break;
				}
				case 0xF: {
					set_flag(cpu, C, get_bit_on(cpu->a, 0));
					set_flag(cpu, N, 0);
					set_flag(cpu, H, 0);
					cpu->a >>= get_flag_on(cpu, C);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
			}
			break;
		}
		case 0x2: {
			switch (n2) {
				case 0x0: {
					if (!get_flag_on(cpu, Z))
						cpu->pc += simm1;

					break;
				}
				case 0x1: {
					*(cpu->hl) = immw;
					break;
				}
				case 0x2: {
					*(cpu->hl) = cpu->a;
					*(cpu->hl)++;
					break;
				}
				case 0x3: {
					*(cpu->hl)++;
					break;
				}
				case 0x4: {
					register uint16_t orig = cpu->h++;

					if (get_bit_on(cpu->h, 4) ^ get_bit_on(orig, 4) == 0)
						set_flag(cpu, H, 1);

					if (!cpu->h)
						set_flag(cpu, Z, 1);

					set_flag(cpu, N, 0);
					break;
				}
				case 0x5: {
					register uint16_t orig = cpu->h--;
					
					if (get_bit_on(cpu->h, 4) ^ get_bit_on(orig, 4))
						set_flag(cpu, H, 1);

					if (!cpu->h)
						set_flag(cpu, Z, 1);

					set_flag(cpu, N, 1);
					break;
				}
				case 0x6: {
					cpu->h = imm1;
					break;
				}
				case 0x7: {
					if (!cpu->a)
						set_flag(cpu, Z, 1);

					register bool needed = 0;
					if (cpu->a & 0xf > 0x9 || get_flag_on(cpu, H)) {
						cpu->a += 0x6;
						needed = 1;
					}

					if (cpu->a & 0xf0 >> 0xf > 0x9 || get_flag_on(cpu, C)) {
						cpu->a += 0x60;
						needed = 1;
					}

					set_flag(cpu, C, needed);
					set_flag(cpu, H, 0);
					break;
				}
				case 0x8: {
					if (get_flag_on(cpu, Z))
						cpu->pc += simm1;
						
					break;
				}
				case 0x9: {
					register uint32_t add = *(cpu->hl) * 2;
					*(cpu->hl) *= 2;
					
					if (get_bit_on(add, 16))
						set_flag(cpu, C, 1);

					if (get_bit_on(add, 4) & get_bit_on(*(cpu->hl), 4))
						set_flag(cpu, H, 1);

					set_flag(cpu, N, 0);
					break;
				}
				case 0xA: {
					cpu->a = *(cpu->hl);
					*(cpu->hl)++;
					break;
				}
				case 0xB: {
					*(cpu->hl)--;
					break;
				}
				case 0xC: {
					register uint16_t orig = cpu->l++;

					if (get_bit_on(cpu->l, 4) ^ get_bit_on(orig, 4) == 0)
						set_flag(cpu, H, 1);

					if (!cpu->l)
						set_flag(cpu, Z, 1);

					set_flag(cpu, N, 0);
					break;
				}
				case 0xD: {
					register uint16_t orig = cpu->l--;

					if (get_bit_on(cpu->l, 4) ^ get_bit_on(orig, 4))
						set_flag(cpu, H, 1);

					if (!cpu->l)
						set_flag(cpu, Z, 1);

					set_flag(cpu, N, 0);
					break;
				}
				case 0xE: {
					cpu->l = imm1;
					break;
				}
				case 0xF: {
					cpu->a = ~cpu->a;
					set_flag(cpu, N, 1);
					set_flag(cpu, H, 1);
					break;
				}
			}
			break;
		}
		case 0x3: {
			switch (n2) {
				case 0x0: {
					if (!get_flag_on(cpu, C))
						cpu->pc += simm1;

					break;
				}
				case 0x1: {
					cpu->sp = immw;
					break;
				}
				case 0x2: {
					if (!get_flag_on(cpu, C))
					
					break;
				}
				case 0x3: {
					cpu->sp++;
					break;
				}
				case 0x4: {
					break;
				}
				case 0x5: {
					break;
				}
				case 0x6: {
					break;
				}
				case 0x7: {
					break;
				}
				case 0x8: {
					if (get_flag_on(cpu, C))
						cpu->pc += simm1;
						
					break;
				}
				case 0x9: {
					break;
				}
				case 0xA: {
					break;
				}
				case 0xB: {
					break;
				}
				case 0xC: {
					break;
				}
				case 0xD: {
					break;
				}
				case 0xE: {
					break;
				}
				case 0xF: {
					set_flag(cpu, C, ~get_flag_on(cpu, C));
					set_flag(cpu, N, 0);
					set_flag(cpu, H, 0);
					cpu->cycles = 4;
					break;
				}
			}
			break;
		}
		case 0x4: {
			switch (n2) {
				case 0x0: {
					cpu->b = cpu->b;
					break;
				}
				case 0x1: {
					cpu->b = cpu->c;
					break;
				}
				case 0x2: {
					cpu->b = cpu->d;
					break;
				}
				case 0x3: {
					cpu->b = cpu->e;
					break;
				}
				case 0x4: {
					cpu->b = cpu->h;
					break;
				}
				case 0x5: {
					cpu->b = cpu->l;
					break;
				}
				case 0x6: {
					cpu->b = cpu->addressSpace[*(cpu->hl)];
					break;
				}
				case 0x7: {
					cpu->b = cpu->a;
					break;
				}
				case 0x8: {
					cpu->c = cpu->b;
					break;
				}
				case 0x9: {
					cpu->c = cpu->c;
					break;
				}
				case 0xA: {
					cpu->c = cpu->d;
					break;
				}
				case 0xB: {
					cpu->c = cpu->e;
					break;
				}
				case 0xC: {
					cpu->c = cpu->h;
					break;
				}
				case 0xD: {
					cpu->c = cpu->l;
					break;
				}
				case 0xE: {
					cpu->c = cpu->addressSpace[*(cpu->hl)];
					break;
				}
				case 0xF: {
					cpu->c = cpu->a;
					break;
				}
			}
			break;
		}
		case 0x5: {
			switch (n2) {
				case 0x0: {
					cpu->d = cpu->b;
					break;
				}
				case 0x1: {
					cpu->d = cpu->c;
					break;
				}
				case 0x2: {
					cpu->d = cpu->d;
					break;
				}
				case 0x3: {
					cpu->d = cpu->e;
					break;
				}
				case 0x4: {
					cpu->d = cpu->h;
					break;
				}
				case 0x5: {
					cpu->d = cpu->l;
					break;
				}
				case 0x6: {
					cpu->d = cpu->addressSpace[*(cpu->hl)];
					break;
				}
				case 0x7: {
					cpu->d = cpu->a;
					break;
				}
				case 0x8: {
					cpu->e = cpu->b;
					break;
				}
				case 0x9: {
					cpu->e = cpu->c;
					break;
				}
				case 0xA: {
					cpu->e = cpu->d;
					break;
				}
				case 0xB: {
					cpu->e = cpu->e;
					break;
				}
				case 0xC: {
					cpu->e = cpu->h;
					break;
				}
				case 0xD: {
					cpu->e = cpu->l;
					break;
				}
				case 0xE: {
					cpu->e = cpu->addressSpace[*(cpu->hl)];
					break;
				}
				case 0xF: {
					cpu->e = cpu->a;
					break;
				}
			}
			break;
		}
		case 0x6: {
			switch (n2) {
				case 0x0: {
					cpu->h = cpu->b;
					break;
				}
				case 0x1: {
					cpu->h = cpu->c;
					break;
				}
				case 0x2: {
					cpu->h = cpu->d;
					break;
				}
				case 0x3: {
					cpu->h = cpu->e;
					break;
				}
				case 0x4: {
					cpu->h = cpu->h;
					break;
				}
				case 0x5: {
					cpu->h = cpu->l;
					break;
				}
				case 0x6: {
					cpu->h = cpu->addressSpace[*(cpu->hl)];
					break;
				}
				case 0x7: {
					cpu->h = cpu->a;
					break;
				}
				case 0x8: {
					cpu->l = cpu->b;
					break;
				}
				case 0x9: {
					cpu->l = cpu->c;
					break;
				}
				case 0xA: {
					cpu->l = cpu->d;
					break;
				}
				case 0xB: {
					cpu->l = cpu->e;
					break;
				}
				case 0xC: {
					cpu->l = cpu->h;
					break;
				}
				case 0xD: {
					cpu->l = cpu->l;
					break;
				}
				case 0xE: {
					cpu->l = cpu->addressSpace[*(cpu->hl)];
					break;
				}
				case 0xF: {
					cpu->l = cpu->a;
					break;
				}
			}
			break;
		}
		case 0x7: {
			switch (n2) {
				case 0x0: {
					cpu->addressSpace[*(cpu->hl)] = cpu->b;
					break;
				}
				case 0x1: {
					cpu->addressSpace[*(cpu->hl)] = cpu->c;
					break;
				}
				case 0x2: {
					cpu->addressSpace[*(cpu->hl)] = cpu->d;
					break;
				}
				case 0x3: {
					cpu->addressSpace[*(cpu->hl)] = cpu->e;
					break;
				}
				case 0x4: {
					cpu->addressSpace[*(cpu->hl)] = cpu->h;
					break;
				}
				case 0x5: {
					cpu->addressSpace[*(cpu->hl)] = cpu->l;
					break;
				}
				case 0x6: {
					// halt
					break;
				}
				case 0x7: {
					cpu->addressSpace[*(cpu->hl)] = cpu->a;
					break;
				}
				case 0x8: {
					cpu->a = cpu->b;
					break;
				}
				case 0x9: {
					cpu->a = cpu->c;
					break;
				}
				case 0xA: {
					cpu->a = cpu->d;
					break;
				}
				case 0xB: {
					cpu->a = cpu->e;
					break;
				}
				case 0xC: {
					cpu->a = cpu->h;
					break;
				}
				case 0xD: {
					cpu->a = cpu->l;
					break;
				}
				case 0xE: {
					cpu->a = cpu->addressSpace[*(cpu->hl)];
					break;
				}
				case 0xF: {
					cpu->a = cpu->a;
					break;
				}
			}
			break;
		}
		case 0x8: {
			switch (n2) {
				case 0x0: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a += cpu->b;
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x1: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a += cpu->c;
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x2: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a += cpu->d;
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x3: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a += cpu->e;
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x4: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a += cpu->h;
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x5: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a += cpu->l;
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x6: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a += *(cpu->addressSpace + *cpu->hl);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x7: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a += cpu->a;
					set_flag(cpu, Z, cpu->a == 0);
					break;

				}
				case 0x8: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a += cpu->b + get_flag_on(cpu, C);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x9: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a += cpu->c + get_flag_on(cpu, C);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0xA: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a += cpu->d + get_flag_on(cpu, C);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0xB: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a += cpu->e + get_flag_on(cpu, C);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0xC: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a += cpu->h + get_flag_on(cpu, C);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0xD: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a += cpu->l + get_flag_on(cpu, C);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0xE: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a += *(cpu->addressSpace + *cpu->hl) + get_flag_on(cpu, C);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0xF: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a += cpu->a + get_flag_on(cpu, C);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
			}

			//set_flag(cpu, N, 0);			
			break;
		}
		case 0x9: {
			switch (n2) {
				case 0x0: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a -= cpu->b;
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x1: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a -= cpu->c;
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x2: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a -= cpu->d;
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x3: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a -= cpu->e;
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x4: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a -= cpu->h;
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x5: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a -= cpu->l;
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x6: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a -= *(cpu->addressSpace + *cpu->hl);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x7: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a -= cpu->a;
					set_flag(cpu, Z, cpu->a == 0);
					break;

				}
				case 0x8: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a -= cpu->b + get_flag_on(cpu, C);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0x9: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a -= cpu->c + get_flag_on(cpu, C);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0xA: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a -= cpu->d + get_flag_on(cpu, C);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0xB: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a -= cpu->e + get_flag_on(cpu, C);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0xC: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a -= cpu->h + get_flag_on(cpu, C);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0xD: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a -= cpu->l + get_flag_on(cpu, C);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0xE: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a -= *(cpu->addressSpace + *cpu->hl) + get_flag_on(cpu, C);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
				case 0xF: {
					set_flag(cpu, N, 0);
					//todo: set c and h
					cpu->a -= cpu->a + get_flag_on(cpu, C);
					set_flag(cpu, Z, cpu->a == 0);
					break;
				}
			}

			set_flag(cpu, N, 1);
			break;
		}
		case 0xA: {
			switch (n2) {
				case 0x0: {
					break;
				}
				case 0x1: {
					break;
				}
				case 0x2: {
					break;
				}
				case 0x3: {
					break;
				}
				case 0x4: {
					break;
				}
				case 0x5: {
					break;
				}
				case 0x6: {
					break;
				}
				case 0x7: {
					break;
				}
				case 0x8: {
					break;
				}
				case 0x9: {
					break;
				}
				case 0xA: {
					break;
				}
				case 0xB: {
					break;
				}
				case 0xC: {
					break;
				}
				case 0xD: {
					break;
				}
				case 0xE: {
					break;
				}
				case 0xF: {
					break;
				}
			}

			set_flag(cpu, N, 0);
			set_flag(cpu, H, n1 < 0x8 ? 1 : 0);
			set_flag(cpu, C, 0);
			break;
		}
		case 0xB: {
			switch (n2) {
				case 0x0: {
					break;
				}
				case 0x1: {
					break;
				}
				case 0x2: {
					break;
				}
				case 0x3: {
					break;
				}
				case 0x4: {
					break;
				}
				case 0x5: {
					break;
				}
				case 0x6: {
					break;
				}
				case 0x7: {
					break;
				}
				case 0x8: {
					break;
				}
				case 0x9: {
					break;
				}
				case 0xA: {
					break;
				}
				case 0xB: {
					break;
				}
				case 0xC: {
					break;
				}
				case 0xD: {
					break;
				}
				case 0xE: {
					break;
				}
				case 0xF: {
					break;
				}
			}

			set_flag(cpu, N, n1 > 0x8 ? 0 : 1);
			if (n1 >= 0x8)
			{
				set_flag(cpu, H, 1);
				set_flag(cpu, C, 0);
			}
			
			break;
		}
		case 0xC: {
			switch (n2) {
				case 0x0: {
					if (!get_flag_on(cpu, Z)) {
						cpu->pc = cpu->addressSpace[cpu->sp] << 8 | cpu->addressSpace[cpu->sp + 1];
						cpu->sp += 2;
					}

					break;
				}
				case 0x1: {
					cpu->b = cpu->addressSpace[cpu->sp];
					cpu->c = cpu->addressSpace[cpu->sp + 1];
					cpu->sp += 2;
					break;
				}
				case 0x2: {
					if (!get_flag_on(cpu, Z))
						cpu->pc = immw;
						
					break;
				}
				case 0x3: {
					cpu->pc = immw;
					break;
				}
				case 0x4: {
					if (!get_flag_on(cpu, Z)) {
						cpu->sp -= 2;
						cpu->addressSpace[cpu->sp] = cpu->pc & 0xf0 >> 8;
						cpu->addressSpace[cpu->sp + 1] = cpu->pc & 0xf;

						cpu->pc = immw;
					}

					break;
				}
				case 0x5: {
					cpu->sp -= 2;
					cpu->addressSpace[cpu->sp] = cpu->b;
					cpu->addressSpace[cpu->sp + 1] = cpu->c;
					break;
				}
				case 0x6: {
					// add a, d8
					break;
				}
				case 0x7: {
					cpu->sp -= 2;
					cpu->addressSpace[cpu->sp] = cpu->pc & 0xf0 >> 8;
					cpu->addressSpace[cpu->sp + 1] = cpu->pc & 0xf;

					cpu->pc = 0;
					break;
				}
				case 0x8: {
					if (get_flag_on(cpu, Z)) {
						uint16_t jmpAddress = cpu->addressSpace[cpu->sp];
						jmpAddress >>= 8;
						jmpAddress |= cpu->addressSpace[cpu->sp + 1];
						cpu->sp += 2;
						cpu->pc = jmpAddress;
					}	
					break;
				}
				case 0x9: {
					uint16_t jmpAddress = cpu->addressSpace[cpu->sp];					
					cpu->sp += 2;
					cpu->pc = jmpAddress;
					break;
				}
				case 0xA: {
					if (get_flag_on(cpu, C))
						cpu->pc = imm1;
						
					break;
				}
				case 0xB: {
					// CB prefix, should never get here
					break;
				}
				case 0xC: {
					if (get_flag_on(cpu, Z)) {
						cpu->sp -= 2;
						cpu->addressSpace[cpu->sp] = cpu->pc & 0xf0 >> 8;
						cpu->addressSpace[cpu->sp + 1] = cpu->pc & 0xf;

						cpu->pc = immw;
					}

					break;
				}
				case 0xD: {
					cpu->sp -= 2;
					cpu->addressSpace[cpu->sp] = cpu->pc & 0xf0 >> 8;
					cpu->addressSpace[cpu->sp + 1] = cpu->pc & 0xf;

					cpu->pc = immw;
					break;
				}
				case 0xE: {
					// adc a, d8
					break;
				}
				case 0xF: {
					cpu->sp -= 2;
					cpu->addressSpace[cpu->sp] = cpu->pc & 0xf0 >> 8;
					cpu->addressSpace[cpu->sp + 1] = cpu->pc & 0xf;

					cpu->pc = 0x8;
					break;
				}
			}
			break;
		}
		case 0xD: {
			switch (n2) {
				case 0x0: {
					if (!get_flag_on(cpu, C)) {
						cpu->pc = cpu->addressSpace[cpu->sp] << 8 | cpu->addressSpace[cpu->sp + 1];
						cpu->sp += 2;
					}
					break;
				}
				case 0x1: {
					cpu->d = cpu->addressSpace[cpu->sp];
					cpu->e = cpu->addressSpace[cpu->sp + 1];
					cpu->sp += 2;
					break;
				}
				case 0x2: {
					if (!get_flag_on(cpu, C))
						cpu->pc = imm1;

					break;
				}
				case 0x3: {
					// nothing
					break;
				}
				case 0x4: {
					if (!get_flag_on(cpu, C)) {
						cpu->sp -= 2;
						cpu->addressSpace[cpu->sp] = cpu->pc & 0xf0 >> 8;
						cpu->addressSpace[cpu->sp + 1] = cpu->pc & 0xf;

						cpu->pc = immw;
					}
					break;
				}
				case 0x5: {
					cpu->sp -= 2;
					cpu->addressSpace[cpu->sp] = cpu->d;
					cpu->addressSpace[cpu->sp + 1] = cpu->e;
					break;
				}
				case 0x6: {
					// sub d8
					break;
				}
				case 0x7: {
					cpu->sp -= 2;
					cpu->addressSpace[cpu->sp] = cpu->pc & 0xf0 >> 8;
					cpu->addressSpace[cpu->sp + 1] = cpu->pc & 0xf;

					cpu->pc = 0x10;
					break;
				}
				case 0x8: {
					if (get_flag_on(cpu, C)) {
						cpu->pc = cpu->addressSpace[cpu->sp] << 8 | cpu->addressSpace[cpu->sp + 1];
						cpu->sp += 2;
					}

					break;
				}
				case 0x9: {
					cpu->pc = cpu->addressSpace[cpu->sp] << 8 | cpu->addressSpace[cpu->sp + 1];
					cpu->sp += 2;

					cpu->interrupts = 1;
					break;
				}
				case 0xA: {
					if (get_flag_on(cpu, C))
						cpu->pc = imm1;
						
					break;
				}
				case 0xB: {
					// nothing
					break;
				}
				case 0xC: {
					if (get_flag_on(cpu, C)) {
						cpu->sp -= 2;
						cpu->addressSpace[cpu->sp] = cpu->pc & 0xf0 >> 8;
						cpu->addressSpace[cpu->sp + 1] = cpu->pc & 0xf;

						cpu->pc = immw;
					}

					break;
				}
				case 0xD: {
					// nothing
					break;
				}
				case 0xE: {
					// adc a, d8
					break;
				}
				case 0xF: {
					cpu->sp -= 2;
					cpu->addressSpace[cpu->sp] = cpu->pc & 0xf0 >> 8;
					cpu->addressSpace[cpu->sp + 1] = cpu->pc & 0xf;

					cpu->pc = 0x18;
					break;
				}
			}
			break;
		}
		case 0xE: {
			switch (n2) {
				case 0x0: {
					cpu->addressSpace[0xff00 + imm1] = cpu->a;
					break;
				}
				case 0x1: {
					cpu->h = cpu->addressSpace[cpu->sp];
					cpu->l = cpu->addressSpace[cpu->sp + 1];
					cpu->sp += 2;
					break;
				}
				case 0x2: {
					cpu->a = cpu->addressSpace[cpu->c];
					break;
				}
				case 0x3: {
					// nothing
					break;
				}
				case 0x4: {
					// nothing
					break;
				}
				case 0x5: {
					cpu->sp -= 2;
					cpu->addressSpace[cpu->sp] = cpu->h;
					cpu->addressSpace[cpu->sp + 1] = cpu->l;
					break;
				}
				case 0x6: {
					// and d8
					break;
				}
				case 0x7: {
					cpu->sp -= 2;
					cpu->addressSpace[cpu->sp] = cpu->pc & 0xf0 >> 8;
					cpu->addressSpace[cpu->sp + 1] = cpu->pc & 0xf;

					cpu->pc = 0x20;
					break;
				}
				case 0x8: {
					// add sp, r8
					break;
				}
				case 0x9: {
					cpu->pc = cpu->addressSpace[*(cpu->hl)];
					break;
				}
				case 0xA: {
					cpu->addressSpace[immw] = cpu->a;
					break;
				}
				case 0xB: {
					// nothing
					break;
				}
				case 0xC: {
					// nothing
					break;
				}
				case 0xD: {
					// nothing
					break;
				}
				case 0xE: {
					// xor d8
					break;
				}
				case 0xF: {
					cpu->sp -= 2;
					cpu->addressSpace[cpu->sp] = cpu->pc & 0xf0 >> 8;
					cpu->addressSpace[cpu->sp + 1] = cpu->pc & 0xf;

					cpu->pc = 0x28;
					break;
				}
			}
			break;
		}
		case 0xF: {
			switch (n2) {
				case 0x0: {
					cpu->a = cpu->addressSpace[0xff00 + imm1];
					break;
				}
				case 0x1: {
					cpu->a = cpu->addressSpace[cpu->sp];
					cpu->f = cpu->addressSpace[cpu->sp + 1];
					cpu->sp += 2;
					break;
				}
				case 0x2: {
					cpu->addressSpace[cpu->c] = cpu->a;
					break;
				}
				case 0x3: {
					cpu->interrupts = 0;
					break;
				}
				case 0x4: {
					// nothing
					break;
				}
				case 0x5: {
					cpu->sp -= 2;
					cpu->addressSpace[cpu->sp] = cpu->a;
					cpu->addressSpace[cpu->sp + 1] = cpu->f;
					break;
				}
				case 0x6: {
					// or d8
					break;
				}
				case 0x7: {
					cpu->sp -= 2;
					cpu->addressSpace[cpu->sp] = cpu->pc & 0xf0 >> 8;
					cpu->addressSpace[cpu->sp + 1] = cpu->pc & 0xf;

					cpu->pc = 0x30;
					break;
				}
				case 0x8: {
					// ld hl, sp + r8
					break;
				}
				case 0x9: {
					cpu->sp = *(cpu->hl);
					break;
				}
				case 0xA: {
					cpu->a = cpu->addressSpace[immw];
					break;
				}
				case 0xB: {
					cpu->interrupts = 1;
					break;
				}
				case 0xC: {
					// nothing
					break;
				}
				case 0xD: {
					// nothing
					break;
				}
				case 0xE: {
					// cp d8
					break;
				}
				case 0xF: {
					cpu->sp -= 2;
					cpu->addressSpace[cpu->sp] = cpu->pc & 0xf0 >> 8;
					cpu->addressSpace[cpu->sp + 1] = cpu->pc & 0xf;

					cpu->pc = 0x38;
					break;
				}
			}
			break;
		}
	}

}

static void _execute_prefix_instruction(gb_cpu_t *cpu, uint8_t opcode) {
	//register uint8_t opnext & 0b11000000 >> 6
	switch (opcode & 0b11000000 >> 6) {
		case 0x0: {
			// rot[y] r[z]
			break;
		}

		case 0x1: {
			// bit y, r[z]
			break;
		}

		case 0x2: {
			// res y, r[z]
			break;
		}

		case 0x3: {
			// set y, r[z]
			break;
		}
	}
}