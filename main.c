#include <stdio.h>
#include <stdbool.h>
#include <process.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>
#include "rom.h"
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
	gb_display_t display;
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

	uint32_t rom_size = 32768 << header->rom_size;
	free(header);

	printf("rom_size: 0x%x\n", rom_size);
	fread(cpu.addressSpace, rom_size, 1, rom);

	FILE *bootloader = fopen("bootloader.bin", "rb");
	fread(cpu.addressSpace, 256, 1, bootloader);

	static SDL_Rect pixel;
	pixel.w = 3;
	pixel.h = 3;
	bool running = true;
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
			uint8_t instructionSize = 0;
			uint32_t instruction = fetch_opcode(&cpu, &instructionSize);
			printf("pc: 0x%04x, sp: 0x%04x inst: 0x%08x\n", cpu.pc, cpu.sp, instruction);
			execute_instruction(&cpu, instruction);
		}

		SDL_Delay(targetDelayTime);
	}
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(main_window);
	SDL_Quit();
	return 0;
}
