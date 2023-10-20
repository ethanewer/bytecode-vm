#include <stdlib.h>
#include "chunk.h"
#include "mem.h"

void init_chunk(Chunk* chunk) {
	chunk->len = 0;
	chunk->cap = 0;
	chunk->code = nullptr;
	chunk->lines = nullptr;
	init_val_arr(&chunk->constants);
}

void add_chunk(Chunk* chunk, uint8_t byte, int line) {
	if (chunk->len >= chunk->cap) {
		int old_cap = chunk->cap;
		chunk->cap = GROW_CAP(old_cap);
		chunk->code = GROW_ARR(uint8_t, chunk->code, old_cap, chunk->cap);
		chunk->lines = GROW_ARR(int, chunk->lines, old_cap, chunk->cap);
	}
	chunk->code[chunk->len] = byte;
	chunk->lines[chunk->len] = line;
	chunk->len++;
}

void free_chunk(Chunk* chunk) {
	FREE_ARR(uint8_t, chunk->code, chunk->cap);
	FREE_ARR(uint8_t, chunk->lines, chunk->cap);
	free_val_arr(&chunk->constants);
	init_chunk(chunk);
}

int add_constant(Chunk* chunk, Val val) {
	add_val_arr(&chunk->constants, val);
	return chunk->constants.len - 1;
}


