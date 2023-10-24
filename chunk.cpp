#include <stdlib.h>
#include "chunk.h"

void Chunk::push(uint8_t byte, int line) {
	if (len == cap) {
		cap = cap < 8 ? 8 : 2 * cap;
		code = (uint8_t*) realloc(code, cap * sizeof(uint8_t));
		lines = (int*) realloc(lines, cap * sizeof(int));
		if (code == nullptr || lines == nullptr) exit(1);
	}
	code[len] = byte;
	lines[len] = line;
	len++;
}

int Chunk::push_constant(Val val) {
	constants.push_back(val);
	return constants.size() - 1;
}

int Chunk::size() {
	return len;
}

void Chunk::clear() {
	free(code);
	free(lines);
	constants.clear();
	code = nullptr;
	lines = nullptr;
	len = 0;
	cap = 0;
}


