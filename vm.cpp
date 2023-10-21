#include <stdio.h>
#include <string.h>
#include "vm.h"
#include "debug.h"
#include "compiler.h"
#include "obj.h"
#include "mem.h"

VM vm;

void init_vm() {
	reset_stack();
	init_table(&vm.globals);
	init_table(&vm.strings);
	vm.objs = nullptr;
}

void free_vm() {
	free_table(&vm.globals);
	free_table(&vm.strings);
	free_objs();
}

InterpretResult interpret(const char* source) {
	Chunk chunk;
	init_chunk(&chunk);
	if (!compile(source, &chunk)) {
		free_chunk(&chunk);
		return INTERPRET_COMPILE_ERROR;
	}
	vm.chunk = &chunk;
	vm.pc = vm.chunk->code;
	InterpretResult result = run();
	free_chunk(&chunk);
	return result;
}

static InterpretResult run() {
	#define READ_BYTE() (*vm.pc++)
	#define READ_CONSTANT() (vm.chunk->constants.vals[READ_BYTE()])
	#define BINARY_OP(val_type, op) \
		do { \
			if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
				runtime_error("Operands must be numbers."); \
				return INTERPRET_RUNTIME_ERROR; \
			} \
			double b = AS_NUMBER(pop()); \
			double a = AS_NUMBER(pop()); \
			push(val_type(a op b)); \
		} while (false)

	#define READ_STRING() AS_STRING(READ_CONSTANT())

	for (;;) {
		#ifdef DEBUG_TRACE_EXECUTION
			printf("          ");
			for (Val* slot = vm.stack; slot < vm.stack_top; slot++) {
				printf("[ ");
				print_val(*slot);
				printf(" ]");
			}
			printf("\n");
			disassemble_instruction(vm.chunk, (int) (vm.pc - vm.chunk->code));
		#endif
		uint8_t instruction;
		switch (instruction = READ_BYTE()) {
			case OP_CONSTANT: {
				Val constant = READ_CONSTANT();
				push(constant);
				break;
			}
			case OP_NIL:
				push(NIL_VAL);
				break;
			case OP_TRUE:
				push(BOOL_VAL(true)); 
				break;
			case OP_FALSE:
				push(BOOL_VAL(false));
				break;
			case OP_NOT:
				push(BOOL_VAL(!is_truthy(pop())));
				break;
			case OP_EQUAL:
				push(BOOL_VAL(is_equal(pop(), pop())));
				break;
			case OP_GREATER:
				BINARY_OP(BOOL_VAL, >);
				break;
      		case OP_LESS:
				BINARY_OP(BOOL_VAL, <);
				break;
			case OP_NEGATE: 
				if (!IS_NUMBER(peek(0))) {
					runtime_error("Operand must be a number.");
					return INTERPRET_RUNTIME_ERROR;
				}
				push(NUMBER_VAL(-AS_NUMBER(pop()))); 
				break;
			case OP_ADD:
					if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
						concatenate();
					} else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
						push(NUMBER_VAL(AS_NUMBER(pop()) + AS_NUMBER(pop())));
					} else {
						runtime_error("Operands must be two numbers or two strings.");
						return INTERPRET_RUNTIME_ERROR;
					}
					break;
			case OP_SUBTRACT:
				BINARY_OP(NUMBER_VAL, -);
				break;
			case OP_MULTIPLY:
				BINARY_OP(NUMBER_VAL, *); 
				break;
			case OP_DIVIDE:
				BINARY_OP(NUMBER_VAL, /);
				break;
			case OP_PRINT:
				print_val(pop());
				printf("\n");
			case OP_POP: 
				pop(); 
				break;
			case OP_DEFINE_GLOBAL: {
				ObjString* name = READ_STRING();
				table_set(&vm.globals, name, peek(0));
				pop();
				break;
			}
			case OP_GET_GLOBAL: {
				ObjString* name = READ_STRING();
				Val val;
				if (!table_get(&vm.globals, name, &val)) {
					runtime_error("Undefined variable.");
					return INTERPRET_RUNTIME_ERROR;
				}
				push(val);
				break;
			}
			case OP_SET_GLOBAL: {
				ObjString* name = READ_STRING();
				if (table_set(&vm.globals, name, peek(0))) {
					table_remove(&vm.globals, name);
					runtime_error("Undefined variable.");
				}
				break;
			}
			case OP_RETURN:
				return INTERPRET_OK;
		}
	}
	#undef READ_BYTE
	#undef READ_CONSTANT
	#undef BINARY_OP
	#undef READ_STRING
}

static void reset_stack() {
	vm.stack_top = vm.stack;
}

void push(Val val) {
	*vm.stack_top = val;
	vm.stack_top++;
}

Val pop() {
	vm.stack_top--;
	return *vm.stack_top;
}

static Val peek(int dist) {
	return vm.stack_top[-1 - dist];
}

static void runtime_error(const char* msg) {
	fputs("\n", stderr);
	size_t instruction = vm.pc - vm.chunk->code - 1;
	int line = vm.chunk->lines[instruction];
	fprintf(stderr, "%s ", msg);
	fprintf(stderr, "[line %d] in script\n", line);
	reset_stack();
}

static void concatenate() {
	ObjString* b = AS_STRING(pop());
	ObjString* a = AS_STRING(pop());

	int len = a->len + b->len;
	char* chars = ALLOCATE(char, len + 1);
	memcpy(chars, a->chars, a->len);
	memcpy(chars + a->len, b->chars, b->len);
	chars[len] = '\0';

	ObjString* result = take_string(chars, len);
	push(OBJ_VAL(result));
}