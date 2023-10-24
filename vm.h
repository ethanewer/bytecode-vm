#ifndef vm_h
#define vm_h

#include "obj.h"
#include "table.h"
#include "val.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * (UINT8_MAX + 1))

struct CallFrame {
	ObjFn* fn;
	uint8_t* pc;
	Val* slots;
};

struct VM {
	CallFrame frames[FRAMES_MAX];
	int frames_len;
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
static void push(Val val);
static Val pop();
static Val peek(int dist);
static void runtime_error(const char* msg);
static void concatenate();
static bool call_value(Val callee, int num_args);
static bool call(ObjFn* fn, int num_args);
static void define_native(const char* name, NativeFn fn);
static void free_objs();

#endif