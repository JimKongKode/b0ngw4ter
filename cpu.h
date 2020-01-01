#ifndef cpu_h
#define cpu_h

#include <stdint.h>
#include <stdbool.h>
#include "display.h"

#define C 4
#define H 5
#define N 6
#define Z 7

#define AF 0
#define BC 1
#define DE 2
#define HL 3

typedef struct {
	gb_display_t *display;
	uint8_t cycles;
	uint8_t run_mode;
	uint16_t *af;
	uint16_t *bc;
	uint16_t *de;
	uint16_t *hl;

	uint8_t a;
	uint8_t f;
	uint8_t b;
	uint8_t c;
	uint8_t d;
	uint8_t e;
	uint8_t h;
	uint8_t l;
	uint16_t sp;
	uint16_t pc;
	bool halt;
	bool interrupts;

	uint8_t addressSpace[0x10000];
	uint8_t *ram;
} gb_cpu_t;

static inline void set_flag(gb_cpu_t *cpu, uint8_t bit, bool status);
void init_cpu(gb_cpu_t *cpu, gb_display_t *display);
uint32_t fetch_opcode(gb_cpu_t *cpu, uint8_t *instructionSize);
void execute_instruction(gb_cpu_t *cpu, uint32_t instruction);
static void _execute_instruction(gb_cpu_t *cpu, uint32_t instruction);
static void _execute_prefix_instruction(gb_cpu_t *cpu, uint8_t next);

#endif

	/*switch (x) {
		case 0x0: {
			switch (z) {
				case 0x0: {
					switch (y) {
						case 0x0: {
							// nop
							cpu->cycles = 4;
							break;
						}
						
						case 0x1: {
							// ex af, af'
							break;
						}

						case 0x2: {
							// djnz d
							break;
						}

						case 0x3: {
							// jr d
							cpu->cycles = 12;
							break;
						}
						default: {
							// jr cc[y - 4], d
							break;
						}
					}

					break;
				}

				case 0x1: {
					if (!q) {
						// ld rp[p], nn
					} else {
						// add hl, rp[p]
						*(cpu->wideregptr[HL]) += *(rp[p]);
						cpu->cycles = 8;
					}

					break;
				}

				case 0x2: {
					if (!q) {
						switch (p) {
							case 0x0: {
								// ld (bc), a
								cpu->a = *(cpu->ram.ram + cpu->bc);
								cpu->cycles = 8;
								break;
							}
							case 0x1: {
								//ld (de), a
								cpu->cycles = 8;
								break;
							}
							case 0x2: {
								//ld (nn) hl
								break;
							}
							case 0x3: {
								//ld (nn), A
								break;
							}
						}
					} else {
						switch (p) {
							case 0x0: {
								//ld a, (bc)
								break;
							}
							case 0x1: {
								//ld a, (de)
								break;
							}
							case 0x2: {
								//ld hl, (nn)
								cycles
								break;
							}
							case 0x3: {
								//ld a, (nn)
								break;
							}
						}
					}
					
					break;
				}

				case 0x3: {
					if (!q) {
						//inc rp[p]
						cpu->cycles = 8;						
					} else {
						//dec rp[p]
						cpu->cycles = 8;
					}

					break;
				}

				case 0x4: {
					//inc r[y]
					cpu->cycles = 4;
					break;
				}

				case 0x5: {
					//dec r[y]
					cpu->cycles = 4;
					break;
				}

				case 0x6: {
					//ld r[y], n
					break;
				}
				case 0x7: {
					switch (y) {
						case 0x0: {
							// rlca
							cpu->cycles = 4;
							break;
						}
						case 0x1: {
							// rrca
							cpu->cycles = 4;
							break;
						}
						case 0x2: {
							// rla
							cpu->cycles = 4;
							break;
						}
						case 0x3: {
							// rra
							cpu->cycles = 4;
							break;
						}
						case 0x4: {
							// daa
							cpu->cycles = 4;
							break;
						}
						case 0x5: {
							// cpl
							cpu->a ~= cpu->a;
							set_flag(cpu, H, 1);
							set_flag(cpu, N, 1);
							cpu->cycles = 4;
							break;
						}
						case 0x6: {
							// scf
							set_flag(cpu, C, 1);
							set_flag(cpu, N, 0);
							set_flag(cpu, H, 0);
							cpu->cycles = 4;
							break;
						}
						case 0x7: {
							// ccf
							set_flag(cpu, C, ~get_flag_on(cpu, C));
							set_flag(cpu, N, 0);
							set_flag(cpu, H, 0);
							cpu->cycles = 4;
							break;
						}
					}
				}
			}
			
			break;
		}

		case 0x1: {
			if (z == 6 && y == 6) {
				cpu->halt = 1;
			} else {
				// ld r[y], r[z]
			}

			break;
		}

		case 0x2: {
			// alu[y] r[z]
			break;
		}

		case 0x3: {
			switch (z) {
				case 0x0: {
					// ret cc[y]
					break;
				}

				case 0x1: {
					if (!q) {
						// pop rp2[p]
					} else {
						switch (p) {
							case 0x0: {
								// ret
								break;
							}

							case 0x1: {
								// exx
								break;
							}

							case 0x2: {
								// jp hl
								cpu->pc = *(cpu->wideregptr[HL]);
								cpu->cycles = 4;
								break;
							}

							case 0x3: {
								// ld sp, hl
								cpu->sp = *(cpu->wideregptr[HL]);
								cpu->cycles = 8;
								break;
							}
						}
					}

					break;
				}

				case 0x2: {
					// jp cc[y], nn
					break;
				}

				case 0x3: {
					switch (y) {
						case 0x0: {
							// jp nn
							cpu->pc = 
							break;
						}

						case 0x1: {
							// (CB prefix)
							break;
						}

						case 0x2: {
							// out (n), A
							break;
						}

						case 0x3: {
							// in A, (n)
							break;
						}

						case 0x4: {		
							// ex (sp), hl
							register uint16_t temp = cpu->stack[cpu->sp];
							cpu->stack[cpu->sp] = *(cpu->wideregptr[HL]);
							*(cpu->wideregptr[HL]) = temp;
							break;
						}

						case 0x5: {
							// ex de, hl
							register uint16_t temp = *(cpu->wideregptr[DE]);
							*(cpu->wideregptr[DE]) = *(cpu->wideregptr[HL]);
							*(cpu->wideregptr[HL]) = temp;
							break;
						}

						case 0x6: {
							// di
							cpu->interrupts = 0;
							break;
						}

						case 0x7: {
							// ei
							cpu->interrupts = 1;
							break;
						}
					}

					break;
				}

				case 0x4: {
					// call cc[y], nn
					break;
				}

				case 0x5: {
					if (!q) {
						// push rp2[p]
					} else {
						switch (p) {
							case 0x0: {
								// call nn
								break;
							}

							case 0x1: {
								// (DD prefix)
								break;
							}
							
							case 0x2: {
								// (ED prefix)
								break;
							}

							case 0x3: {
								// (FD prefix)
								break;
							}
						}
					}
					
					break;
				}

				case 0x6: {
					//alu[y] n
					break;
				}
				
				case 0x7: {
					//rst y * 8
					break;
				}
			}

			break;
		}
	}*/