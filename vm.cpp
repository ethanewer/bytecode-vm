#include <stdio.h>
#include <math.h>
#include <string.h>
#include "vm.h"
#include "debug.h"
#include "compiler.h"
#include "obj.h"
#include "mem.h"
#include "natives.h"

VM vm;

void init_vm() {
	reset_stack();
	init_table(&vm.globals);
	init_table(&vm.strings);
	vm.objs = nullptr;

	define_native("clock", clock_native);
}

void free_vm() {
	free_table(&vm.globals);
	free_table(&vm.strings);
	free_objs();
}

InterpretResult interpret(const char* source) {
	ObjFn* fn = compile(source);
	if (fn == nullptr) return INTERPRET_COMPILE_ERROR;	
	push(OBJ_VAL(fn));
	ObjClosure* closure = new_closure(fn);
	pop();
	push(OBJ_VAL(closure));
	call(closure, 0);
	return run();
}

static InterpretResult run() {
	CallFrame* frame = &vm.frames[vm.frames_len - 1];

	#define READ_BYTE() (*frame->pc++)

	#define READ_CONSTANT() (frame->closure->fn->chunk.constants.vals[READ_BYTE()])

	#define READ_STRING() AS_STRING(READ_CONSTANT())
	
	#define READ_SHORT() (frame->pc += 2, (uint16_t) ((frame->pc[-2] << 8) | frame->pc[-1]))
	
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

	#define SELF_BINARY_OP_LOCAL(op) \
		do { \
			uint8_t slot = READ_BYTE(); \
			Val val = frame->slots[slot]; \
			if (!IS_NUMBER(val) || !IS_NUMBER(peek(0))) { \
				runtime_error("Operands must be numbers."); \
				return INTERPRET_RUNTIME_ERROR; \
			} \
			double num = AS_NUMBER(peek(0)); \
			frame->slots[slot] = NUMBER_VAL(AS_NUMBER(val) op num); \
		} while (false)

	#define SELF_BINARY_OP_GLOBAL(op) \
		do { \
			ObjString* name = READ_STRING(); \
			if (!IS_NUMBER(peek(0))) { \
				runtime_error("Operands must be numbers."); \
				return INTERPRET_RUNTIME_ERROR; \
			} \
			double num = AS_NUMBER(peek(0)); \
			Val val; \
			if (!table_get(&vm.globals, name, &val)) { \
				runtime_error("Undefined variable."); \
				return INTERPRET_RUNTIME_ERROR; \
			} \
			if (!IS_NUMBER(val)) { \
				runtime_error("Operands must be numbers."); \
				return INTERPRET_RUNTIME_ERROR; \
			} \
			table_set(&vm.globals, name, NUMBER_VAL(AS_NUMBER(val) op num)); \
		} while (false)
	
	#define SELF_BINARY_OP_UPVALUE(op) \
		do { \
			uint8_t slot = READ_BYTE(); \
			Val val = *frame->closure->upvalues[slot]->location; \
			if (!IS_NUMBER(val) || !IS_NUMBER(peek(0))) { \
				runtime_error("Operands must be numbers."); \
				return INTERPRET_RUNTIME_ERROR; \
			} \
			double num = AS_NUMBER(peek(0)); \
			*frame->closure->upvalues[slot]->location = NUMBER_VAL(AS_NUMBER(val) op num); \
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
			disassemble_instruction(&frame->closure->fn->chunk, (int) (frame->pc - frame->fn->chunk.code));
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
			case OP_INT_DIVIDE: {
				if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
					runtime_error("Operands must be numbers.");
					return INTERPRET_RUNTIME_ERROR;
				}
				long b = (long) AS_NUMBER(pop());
				long a = (long) AS_NUMBER(pop());
				push(NUMBER_VAL((double) (a / b)));
				break;
			}
			case OP_POW: {
				if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
					runtime_error("Operands must be numbers.");
					return INTERPRET_RUNTIME_ERROR;
				}
				double b = AS_NUMBER(pop());
				double a = AS_NUMBER(pop());
				push(NUMBER_VAL(pow(a, b)));
				break;
			}
			case OP_PRINT:
				print_val(pop());
				printf("\n");
				break;
			case OP_POP: 
				pop(); 
				break;
			case OP_DEFINE_GLOBAL: {
				ObjString* name = READ_STRING();
				table_set(&vm.globals, name, peek(0));
				pop();
				break;
			}
			case OP_GET_LOCAL: {
				uint8_t slot = READ_BYTE();
				push(frame->slots[slot]);
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
			case OP_GET_UPVALUE: {
				uint8_t slot = READ_BYTE();
				push(*frame->closure->upvalues[slot]->location);
				break;
			}
			case OP_SET_LOCAL: {
				uint8_t slot = READ_BYTE();
				frame->slots[slot] = peek(0);
			}
			case OP_SET_GLOBAL: {
				ObjString* name = READ_STRING();
				if (table_set(&vm.globals, name, peek(0))) {
					table_remove(&vm.globals, name);
					runtime_error("Undefined variable.");
				}
				break;
			}
			case OP_SET_UPVALUE: {
				uint8_t slot = READ_BYTE();
				*frame->closure->upvalues[slot]->location = peek(0);
				break;
			}
			case OP_ADD_SELF_LOCAL: 
				SELF_BINARY_OP_LOCAL(+);
				break;
			case OP_SUBTRACT_SELF_LOCAL: 
				SELF_BINARY_OP_LOCAL(-);
				break;
			case OP_MULTIPLY_SELF_LOCAL: 
				SELF_BINARY_OP_LOCAL(*);
				break;
			case OP_DIVIDE_SELF_LOCAL: 
				SELF_BINARY_OP_LOCAL(/);
				break;
			case OP_INT_DIVIDE_SELF_LOCAL: {
				uint8_t slot = READ_BYTE();
				Val val = frame->slots[slot];
				if (!IS_NUMBER(val) || !IS_NUMBER(peek(0))) {
					runtime_error("Operands must be numbers.");
					return INTERPRET_RUNTIME_ERROR;
				}
				long num = (long) AS_NUMBER(peek(0));
				frame->slots[slot] = NUMBER_VAL((double) ((long) AS_NUMBER(val) / num));
				break;
			}
			case OP_POW_SELF_LOCAL: {
				uint8_t slot = READ_BYTE();
				Val val = frame->slots[slot];
				if (!IS_NUMBER(val) || !IS_NUMBER(peek(0))) {
					runtime_error("Operands must be numbers.");
					return INTERPRET_RUNTIME_ERROR;
				}
				double num = AS_NUMBER(peek(0));
				frame->slots[slot] = NUMBER_VAL(pow(AS_NUMBER(val), num));
				break;
			}
			case OP_ADD_SELF_GLOBAL: 
				SELF_BINARY_OP_GLOBAL(+);
				break;
			case OP_SUBTRACT_SELF_GLOBAL: 
				SELF_BINARY_OP_GLOBAL(-);
				break;
			case OP_MULTIPLY_SELF_GLOBAL: 
				SELF_BINARY_OP_GLOBAL(*);
				break;
			case OP_DIVIDE_SELF_GLOBAL: 
				SELF_BINARY_OP_GLOBAL(/);
				break;
			case OP_INT_DIVIDE_SELF_GLOBAL: {
				ObjString* name = READ_STRING();
				if (!IS_NUMBER(peek(0))) {
					runtime_error("Operands must be numbers.");
					return INTERPRET_RUNTIME_ERROR;
				}
				long num = (long) AS_NUMBER(peek(0));
				Val val;
				if (!table_get(&vm.globals, name, &val)) {
					runtime_error("Undefined variable.");
					return INTERPRET_RUNTIME_ERROR;
				}
				if (!IS_NUMBER(val)) {
					runtime_error("Operands must be numbers.");
					return INTERPRET_RUNTIME_ERROR;
				}
				table_set(&vm.globals, name, NUMBER_VAL((double) ((long) AS_NUMBER(val) / num)));
				break;
			}
			case OP_POW_SELF_GLOBAL: {
				ObjString* name = READ_STRING();
				if (!IS_NUMBER(peek(0))) {
					runtime_error("Operands must be numbers.");
					return INTERPRET_RUNTIME_ERROR;
				}
				double num = AS_NUMBER(peek(0));
				Val val;
				if (!table_get(&vm.globals, name, &val)) {
					runtime_error("Undefined variable.");
					return INTERPRET_RUNTIME_ERROR;
				}
				if (!IS_NUMBER(val)) {
					runtime_error("Operands must be numbers.");
					return INTERPRET_RUNTIME_ERROR;
				}
				table_set(&vm.globals, name, NUMBER_VAL(pow(AS_NUMBER(val), num)));
				break;
			}
			case OP_ADD_SELF_UPVALUE: 
				SELF_BINARY_OP_UPVALUE(+);
				break;
			case OP_SUBTRACT_SELF_UPVALUE: 
				SELF_BINARY_OP_UPVALUE(-);
				break;
			case OP_MULTIPLY_SELF_UPVALUE: 
				SELF_BINARY_OP_UPVALUE(*);
				break;
			case OP_DIVIDE_SELF_UPVALUE: 
				SELF_BINARY_OP_UPVALUE(/);
				break;
			case OP_INT_DIVIDE_SELF_UPVALUE: {
				uint8_t slot = READ_BYTE();
				Val val = *frame->closure->upvalues[slot]->location;
				if (!IS_NUMBER(val) || !IS_NUMBER(peek(0))) {
					runtime_error("Operands must be numbers.");
					return INTERPRET_RUNTIME_ERROR;
				}
				long num = (long) AS_NUMBER(peek(0));
				*frame->closure->upvalues[slot]->location = NUMBER_VAL((double) ((long) AS_NUMBER(val) / num));
				break;
			}
			case OP_POW_SELF_UPVALUE: {
				uint8_t slot = READ_BYTE();
				Val val = *frame->closure->upvalues[slot]->location;
				if (!IS_NUMBER(val) || !IS_NUMBER(peek(0))) {
					runtime_error("Operands must be numbers.");
					return INTERPRET_RUNTIME_ERROR;
				}
				double num = AS_NUMBER(peek(0));
				*frame->closure->upvalues[slot]->location = NUMBER_VAL(pow(AS_NUMBER(val), num));
				break;
			}
			case OP_JUMP: {
				uint16_t offset = READ_SHORT();
				frame->pc += offset;
				break;
			}
			case OP_JUMP_IF_FALSE: {
				uint16_t offset = READ_SHORT();
				if (!is_truthy(peek(0))) frame->pc += offset;
				break;
			}
			case OP_LOOP: {
				uint16_t offset = READ_SHORT();
				frame->pc -= offset;
				break;
			}
			case OP_CALL: {
				int num_args = READ_BYTE();
				if (!call_value(peek(num_args), num_args)) return INTERPRET_RUNTIME_ERROR;
				frame = &vm.frames[vm.frames_len - 1];
				break;
			}
			case OP_CLOSURE: {
				ObjFn* fn = AS_FN(READ_CONSTANT());
				ObjClosure* closure = new_closure(fn);
				push(OBJ_VAL(closure));
				for (int i = 0; i < closure->fn->num_upvalues; i++) {
					uint8_t is_local = READ_BYTE();
					uint8_t idx = READ_BYTE();
					if (is_local) {
						closure->upvalues[i] = capture_upvalue(frame->slots + idx);
					} else {
						closure->upvalues[i] = frame->closure->upvalues[idx];
					}
				}
				break;
			}
			case OP_RETURN: {
				Val res = pop();
				close_upvalues(frame->slots);
				vm.frames_len--;
				if (vm.frames_len == 0) {
					pop();
					return INTERPRET_OK;
				}
				vm.stack_top = frame->slots;
				push(res);
				frame = &vm.frames[vm.frames_len - 1];
				break;
			}
		}
	}
	#undef READ_BYTE
	#undef READ_CONSTANT
	#undef READ_STRING
	#undef READ_SHORT
	#undef BINARY_OP
	#undef SELF_BINARY_OP_LOCAL
	#undef SELF_BINARY_OP_GLOBAL
	#undef SELF_BINARY_OP_UPVALUE
}

static void reset_stack() {
	vm.stack_top = vm.stack;
	vm.frames_len = 0;
	vm.open_upvalues = nullptr;
}

static void push(Val val) {
	*vm.stack_top = val;
	vm.stack_top++;
}

static Val pop() {
	vm.stack_top--;
	return *vm.stack_top;
}

static Val peek(int dist) {
	return vm.stack_top[-1 - dist];
}

static void runtime_error(const char* msg) {
	fputs("\n", stderr);
	fprintf(stderr, "%s\n", msg);
	for (int i = vm.frames_len - 1; i >= 0; i--) {
		CallFrame* frame = &vm.frames[i];
		ObjFn* fn = frame->closure->fn;
		size_t instruction = frame->pc - fn->chunk.code - 1;
		fprintf(stderr, "[line %d] in ", fn->chunk.lines[instruction]);
		if (fn->name == nullptr) fprintf(stderr, "script\n");
		else fprintf(stderr, "%s()\n", fn->name->chars);
		reset_stack();
	}
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

static bool call_value(Val callee, int num_args) {
	if (IS_OBJ(callee)) {
		switch (OBJ_TYPE(callee)) {
			case OBJ_CLOSURE:
				return call(AS_CLOSURE(callee), num_args);
			case OBJ_NATIVE: {
				NativeFn native = AS_NATIVE(callee);
				push(native(num_args, vm.stack_top - num_args));
				return true;
			}
			default:
				break;
		}
	}
	runtime_error("Can only call functions and classes.");
	return false;
}

static ObjUpvalue* capture_upvalue(Val* local) {
	ObjUpvalue* prev = nullptr;
	ObjUpvalue* upvalue = vm.open_upvalues;
	while (upvalue != nullptr && upvalue->location > local) {
		prev = upvalue;
		upvalue = upvalue->next;
	}

	if (upvalue != nullptr && upvalue->location == local) return upvalue;

	ObjUpvalue* created = new_upvalue(local);
	created->next = upvalue;
	if (prev == nullptr) vm.open_upvalues = created;
	else prev->next = created;

	return created;
}

static void close_upvalues(Val* last) {
	while (vm.open_upvalues != nullptr && vm.open_upvalues->location >= last) {
		ObjUpvalue* upvalue = vm.open_upvalues;
		upvalue->closed = *upvalue->location;
		upvalue->location = &upvalue->closed;
		vm.open_upvalues = upvalue->next;
	}
}

static bool call(ObjClosure* closure, int num_args) {
	if (num_args != closure->fn->num_params) {
		runtime_error("Incorrect number of arguments.");
		return false;
	}
	if (vm.frames_len == FRAMES_MAX) {
		runtime_error("Stack overflow.");
		return false;
	}
	CallFrame* frame = &vm.frames[vm.frames_len++];
	frame->closure = closure;
	frame->pc = closure->fn->chunk.code;
	frame->slots = vm.stack_top - num_args - 1;
	return true;
}

static void define_native(const char* name, NativeFn fn) {
	push(OBJ_VAL(copy_string(name, (int) strlen(name))));
	push(OBJ_VAL(new_native(fn)));
	table_set(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
	pop();
	pop();
}