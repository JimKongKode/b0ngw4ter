#ifndef utils_h
#define utils_h

#include <stdio.h>
#include <stdint.h>

#define err(x) fprintf(stderr, x)

void *read_bytes(FILE *file, uint32_t offset, size_t size);

#endif