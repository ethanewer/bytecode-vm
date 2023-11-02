#ifndef vm_h
#define vm_h

#include "object.hpp"
#include "table.hpp"
#include "value.hpp"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

struct CallFrame {
  ObjClosure* closure;
  uint8_t* ip;
  Value* slots;
};

struct VM {
  CallFrame frames[FRAMES_MAX];
  int frame_count;
  Value stack[STACK_MAX];
  Value* stack_top;
  Table globals;
  Table strings;
  ObjUpvalue* open_upvalues;
  size_t bytes_allocated;
  size_t next_gc;
  Obj* objects;
  int gray_count;
  int gray_capacity;
  Obj** gray_stack;
  bool no_native_errors;

  VM();
  void clear_stack();
  void push(Value value);
  Value pop();
  Value peek(int distance);
};

enum InterpretResult {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
};

extern VM vm;

void runtime_error(const char* format, ...);
void free_vm();
InterpretResult interpret(const char* source);

#endif
