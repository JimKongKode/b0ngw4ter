#include <stdint.h>
#include <stdlib.h>
#include "utils.h"

void *read_bytes(FILE *file, uint32_t offset, size_t size) {
	void *buffer = calloc(1, size);
	fseek(file, offset, SEEK_SET);
	fread(buffer, size, 1, file);
	return buffer;
}