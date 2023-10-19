#include <stdio.h>
#include "vm.h"
#include "debug.h"
#include "compiler.h"

VM vm;

void init_vm() {
	reset_stack();
}

void free_vm() {

}

InterpretResult interpret(const char* source) {
	compile(source);
	return INTERPRET_OK;
}

static InterpretResult run() {
	#define READ_BYTE() (*vm.pc++)
	#define READ_CONSTANT() (vm.chunk->constants.vals[READ_BYTE()])
	#define BINARY_OP(op) \
		do { \
			double b = pop(); \
			double a = pop(); \
			push(a op b); \
		} while (false)

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
			case OP_NEGATE: 
				push(-pop()); 
				break;
			
			case OP_ADD:
				BINARY_OP(+); 
				break;
			case OP_SUBTRACT:
				BINARY_OP(-);
				break;
			case OP_MULTIPLY:
				BINARY_OP(*); 
				break;
			case OP_DIVIDE:
				BINARY_OP(/);
				break;
			case OP_RETURN:
				print_val(pop());
				printf("\n");
				return INTERPRET_OK;
		}
	}
	#undef READ_BYTE
	#undef READ_CONSTANT
	#undef BINARY_OP
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