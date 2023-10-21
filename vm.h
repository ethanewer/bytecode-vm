#ifndef vm_h
#define vm_h

#include "chunk.h"
#include "table.h"
#include "val.h"

#define STACK_MAX 256

struct VM {
	Chunk* chunk;
	uint8_t* pc;
	Val stack[STACK_MAX];
	Val* stack_top;
	Table globals;
	Table strings;
	Obj* objs;
};

enum InterpretResult {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
};

extern VM vm;

void init_vm();
void free_vm();
InterpretResult interpret(const char* source);
static InterpretResult run();
static void reset_stack();
void push(Val val);
Val pop();
static Val peek(int dist);
static void runtime_error(const char* msg);
static void concatenate();
ObjString* table_find_string(Table* table, const char* chars, int length, uint32_t hash);

#endif