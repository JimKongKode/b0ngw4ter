#include <stdio.h>
#include <stdbool.h>
#include <process.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>
#include "rom.h"
#include "ppu.h"
#include "cpu.h"
#include "utils.h"

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 432

int main(int argc, char *argv[]) {
	srand(time(NULL));
	SDL_Init(SDL_INIT_EVERYTHING);
	static const int buildNumber = 1;
	static const double targetDelayTime = 1000 / 60;
	char windowTitle[50];
	sprintf(windowTitle, "%s %i", "b0ngw4ter development build", buildNumber);
	SDL_Window *main_window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 480, 432, 0);
	if (!main_window) {
		err("Failed to create window.\n");
		return 1;
	}

	if (argc != 2) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Usage:\nbongwater <rom file>", main_window);
		return 1;
	}

	FILE *rom = fopen(argv[1], "rb");
	if (!rom) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Failed to open rom file.", main_window);
		return 1;
	}

	gb_cpu_t cpu;
	gb_ppu_t display;
	init_cpu(&cpu, &display);

	//cpu.display->renderer = 
	SDL_Renderer *renderer = SDL_CreateRenderer(main_window, 0, 0);
	display.renderer = renderer;
	struct rom_header *header = read_bytes(rom, 0x100, sizeof(struct rom_header));

	if (header->old_license_code == 0x33) {
		if (header->sgb_flag == 0x03)
			cpu.run_mode = 2;
		else
			cpu.run_mode = 1;
	}

	uint32_t rom_size = 0;
	if (header->rom_size <= 0x8)
		rom_size = 32768 << header->rom_size;
	else if (header->rom_size == 0x52)
		rom_size = 72 * 32768;
	else if (header->rom_size == 0x53)
		rom_size = 80 * 32768;
	else
		rom_size = 96 * 32768;
		
	free(header);

	printf("rom_size: 0x%x\n", rom_size);
	fread(cpu.addressSpace, rom_size, 1, rom);

	FILE *bootloader = fopen("bootloader.bin", "rb");
	fread(cpu.addressSpace, 256, 1, bootloader);

	bool running = true;
	FILE *dump = fopen("dump.txt", "w+");
	char x[] = "bcdehlza";
	while (running) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				running = false;
			}
		}

		if (cpu.pc > rom_size)
			break;
		
		for (int i = 0; i < 70224; i++) {
			if (cpu.instruction_wait_cycles == 0) {
				//printf("pc: 0x%04x ", cpu.pc);
				uint32_t instruction = fetch_opcode(&cpu);
				//printf("sp: 0x%04x, inst: 0x%08x, af: 0x%04x, bc: 0x%04x, de: 0x%04x, hl: 0x%04x\n", cpu.sp, instruction, *cpu.af, *cpu.bc, *cpu.de, *cpu.hl);
				fprintf(dump, "instruction: 0x%x, pc: 0x%x, sp:0x%x\n", instruction, cpu.pc, cpu.sp);
				for (int i = 0; i < 8; i++) {
					if (i != 7)
						fprintf(dump, "%c: 0x%x, ", x[i], *cpu.registers[i]);
					else
						fprintf(dump, "f, 0x%x, hl: 0x%x, af: 0x%x, bc: 0x%x, de: 0x%x ", cpu.f, *cpu.hl, *cpu.af, *cpu.bc, *cpu.de);
				}
				fprintf(dump, "\n\n");
				execute_instruction(&cpu, instruction);
			}
			cpu.instruction_wait_cycles--;
		}
		/*
		rect.x++;
		rect.y = 3;
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_RenderFillRect(renderer, &rect);
		SDL_RenderPresent(renderer);
		SDL_Delay(targetDelayTime);*/
	}
	fclose(dump);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(main_window);
	SDL_Quit();
	return 0;
}
