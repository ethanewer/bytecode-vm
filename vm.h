#ifndef vm_h
#define vm_h

#include "chunk.h"
#include "val.h"

#define STACK_MAX 256

typedef struct {
	Chunk* chunk;
	uint8_t* pc;
	Val stack[STACK_MAX];
	Val* stack_top;
} VM;

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
} InterpretResult;

void init_vm();
void free_vm();
InterpretResult interpret(const char* source);
static InterpretResult run();
static void reset_stack();
void push(Val val);
Val pop();

#endif