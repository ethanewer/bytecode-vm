#ifndef chunk_h
#define chunk_h

#include <vector>
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
	OP_INT_DIVIDE,
	OP_POW,
	OP_EQUAL,
	OP_GREATER,
	OP_LESS,
	OP_PRINT,
	OP_POP,
	OP_DEFINE_GLOBAL,
	OP_GET_GLOBAL,
	OP_SET_GLOBAL,
	OP_ADD_SELF_GLOBAL,
	OP_SUBTRACT_SELF_GLOBAL,
	OP_MULTIPLY_SELF_GLOBAL,
	OP_DIVIDE_SELF_GLOBAL,
	OP_INT_DIVIDE_SELF_GLOBAL,
	OP_POW_SELF_GLOBAL,
	OP_ADD_SELF_LOCAL,
	OP_SUBTRACT_SELF_LOCAL,
	OP_MULTIPLY_SELF_LOCAL,
	OP_DIVIDE_SELF_LOCAL,
	OP_INT_DIVIDE_SELF_LOCAL,
	OP_POW_SELF_LOCAL,
	OP_SET_LOCAL,
	OP_GET_LOCAL,
	OP_JUMP_IF_FALSE,
	OP_JUMP,
	OP_LOOP,
	OP_CALL
};

class Chunk {
public:
	uint8_t* code;
	int* lines;
	std::vector<Val> constants;

	Chunk() : code(nullptr), lines(nullptr), len(0), cap(0) {}
	void push(uint8_t byte, int line);
	int push_constant(Val val);
	int size();
	void clear();

private:
	int len;
	int cap;
};

#endif