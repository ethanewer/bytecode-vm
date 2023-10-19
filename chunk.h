#ifndef chunk_h
#define chunk_h

#include "common.h"
#include "val.h"

typedef enum {
	OP_CONSTANT,
	OP_NEGATE,
	OP_RETURN,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
} OpCode;

typedef struct {
	int len;
	int cap;
	uint8_t* code;
	int* lines;
	ValArr constants;
} Chunk;

void init_chunk(Chunk* chunk);
void add_chunk(Chunk* chunk, uint8_t byte, int line);
void free_chunk(Chunk* chunk);
int add_constant(Chunk* chunk, Val val);

#endif