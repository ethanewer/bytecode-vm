#ifndef chunk_h
#define chunk_h

#include "common.h"
#include "val.h"

enum OpCode {
	OP_CONSTANT,
	OP_NIL,
	OP_TRUE,
	OP_FALSE,
	OP_NEGATE,
	OP_NOT,
	OP_RETURN,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_EQUAL,
	OP_GREATER,
	OP_LESS,
};

struct Chunk {
	int len;
	int cap;
	uint8_t* code;
	int* lines;
	ValArr constants;
};

void init_chunk(Chunk* chunk);
void add_chunk(Chunk* chunk, uint8_t byte, int line);
void free_chunk(Chunk* chunk);
int add_constant(Chunk* chunk, Val val);

#endif