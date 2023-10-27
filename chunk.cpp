#include "chunk.h"
#include "gc.h"

void Chunk::push(uint8_t byte, int line) {
	if (len >= cap) {
		int prev_cap = cap;
		cap = cap < 8 ? 8 : 2 * cap;
		code = GROW_ARR(uint8_t, code, prev_cap, cap);
		lines = GROW_ARR(int, lines, prev_cap, cap);
		if (code == nullptr || lines == nullptr) exit(1);
	}
	code[len] = byte;
	lines[len] = line;
	len++;
}

int Chunk::push_constant(Val val) {
	if (constants_len >= constants_cap) {
		int prev_cap = constants_cap;
		constants_cap = constants_cap < 8 ? 8 : 2 * constants_cap;
		constants = GROW_ARR(Val, constants, prev_cap, constants_cap);
		if (constants == nullptr) exit(1);
	}
	constants[constants_len++] = val;
	return constants_len - 1;
}

int Chunk::size() {
	return len;
}




