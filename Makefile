CC=gcc

main: cpu.c display.c main.c utils.c
	$(CC) -o b0ngw4ter $(wildcard *.c) -lmingw32 -lSDL2main -lSDL2 -I.