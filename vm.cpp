#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "vm.hpp"
#include "common.hpp"
#include "compiler.hpp"
#include "debug.hpp"
#include "object.hpp"
#include "memory.hpp"
#include "native.hpp"
#include "nativeclass.hpp"
#include "typing.hpp"

VM vm; 

VM::VM() {
  clear_stack();
  objects = nullptr;
  bytes_allocated = 0;
  next_gc = 1024 * 1024;
  gray_count = 0;
  gray_capacity = 0;
  gray_stack = nullptr;
  had_native_error = false;

  define_native("number", number_native);
  define_native("string", string_native);
  define_native("bool", bool_native);
  define_native("print", print_native);
  define_native("println", println_native);
  define_native("input", input_native);
  define_native("clock", clock_native);
  define_native("type", type_native);
  
  define_native("_List", native_list);
  define_native("_Map", native_map);
}

InterpretResult VM::interpret(const char* source) {
  ObjFunction* function = compile(source);
  if (function == nullptr) return INTERPRET_COMPILE_ERROR;
  push(OBJ_VAL(function));
  ObjClosure* closure = new ObjClosure(function, make_upvalue_array(function->upvalue_count));
  pop();
  push(OBJ_VAL(closure));
  call(closure, 0);
  return run();
}

void VM::clear() {
  globals.clear();
  strings.clear();
  free_objects();
}

void VM::clear_stack() {
  stack_top = stack;
  frame_count = 0;
  open_upvalues = nullptr;
}

void VM::push(Value value) {
  *stack_top = value;
  stack_top++;
}

Value VM::pop() {
  stack_top--;
  return *stack_top;
}

Value VM::peek(int distance) {
  return stack_top[-1 - distance];
}

void VM::runtime_error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);
  for (int i = frame_count - 1; i >= 0; i--) {
    CallFrame* frame = &frames[i];
    ObjFunction* function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
    if (function->name == nullptr) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  clear_stack();
}

bool VM::call(ObjClosure* closure, int arg_count) {
  if (arg_count != closure->function->arity) {
    runtime_error("Expected %d arguments but got %d.", closure->function->arity, arg_count);
    return false;
  }
  if (frame_count == FRAMES_MAX) {
    runtime_error("Stack overflow.");
    return false;
  }
  CallFrame* frame = &frames[frame_count++];
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
  frame->slots = stack_top - arg_count - 1;
  return true;
}

bool VM::call_value(Value callee, int arg_count) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_BOUND_METHOD: {
        ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
        stack_top[-arg_count - 1] = bound->receiver;
        return call(bound->method, arg_count);
      }
      case OBJ_CLASS: {
        ObjClass* klass = AS_CLASS(callee);
        stack_top[-arg_count - 1] = OBJ_VAL(new ObjInstance(klass));
        Value initializer;
        if (klass->methods.get(klass->name, &initializer)) {
          return call(AS_CLOSURE(initializer), arg_count);
        } else if (arg_count != 0) {
          runtime_error("Expected 0 arguments but got %d.", arg_count);
          return false;
        }
        return true;
      }
      case OBJ_CLOSURE:
        return call(AS_CLOSURE(callee), arg_count);
      case OBJ_NATIVE: {
        NativeFn native = AS_NATIVE(callee);
        Value result = native(arg_count, stack_top - arg_count);
        stack_top -= arg_count + 1;
        push(result);
        return !had_native_error;
      }
      default:
        break; 
    }
  }
  runtime_error("Can only call functions and classes.");
  return false;
}

bool VM::invoke_from_class(ObjClass* klass, ObjString* name, int arg_count) {
  Value method;
  if (!klass->methods.get(name, &method)) {
    runtime_error("Undefined property '%s'.", name->chars);
    return false;
  }
  return call(AS_CLOSURE(method), arg_count);
}


bool VM::invoke(ObjString* name, int arg_count) {
  Value receiver = peek(arg_count);
  
  if (IS_NATIVE_INSTANCE(receiver)) {
    Value result = native_instance_call(AS_NATIVE_INSTANCE(receiver), name, arg_count, stack_top - arg_count);
    stack_top -= arg_count + 1;
    push(result);
    return !had_native_error;
  }
  
  if (!IS_INSTANCE(receiver)) {
    runtime_error("Only instances have methods.");
    return false;
  }
  
  ObjInstance* instance = AS_INSTANCE(receiver);
  Value value;
  if (instance->fields.get(name, &value)) {
    stack_top[-arg_count - 1] = value;
    return call_value(value, arg_count);
  }
  return invoke_from_class(instance->klass, name, arg_count);
}

bool VM::bind_method(ObjClass* klass, ObjString* name) {
  Value method;
  if (!klass->methods.get(name, &method)) {
    runtime_error("Undefined property '%s'.", name->chars);
    return false;
  }
  ObjBoundMethod* bound = new ObjBoundMethod(peek(0), AS_CLOSURE(method));
  pop();
  push(OBJ_VAL(bound));
  return true;
}

ObjUpvalue* VM::capture_upvalue(Value* local) {
  ObjUpvalue* prev_upvalue = nullptr;
  ObjUpvalue* upvalue = open_upvalues;
  while (upvalue != nullptr && upvalue->location > local) {
    prev_upvalue = upvalue;
    upvalue = upvalue->next;
  }
  if (upvalue != nullptr && upvalue->location == local) {
    return upvalue;
  }
  ObjUpvalue* created_upvalue = new ObjUpvalue(local);
  created_upvalue->next = upvalue;
  if (prev_upvalue == nullptr) {
    open_upvalues = created_upvalue;
  } else {
    prev_upvalue->next = created_upvalue;
  }
  return created_upvalue;
}

void VM::close_upvalues(Value* last) {
  while (open_upvalues != nullptr && open_upvalues->location >= last) {
    ObjUpvalue* upvalue = open_upvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    open_upvalues = upvalue->next;
  }
}

void VM::define_method(ObjString* name) {
  Value method = peek(0);
  ObjClass* klass = AS_CLASS(peek(1));
  klass->methods.set(name, method);
  pop();
}

void VM::define_native(const char* name, NativeFn function) {
  push(OBJ_VAL(copy_string(name, (int)strlen(name))));
  push(OBJ_VAL(new ObjNative(function)));
  globals.set(AS_STRING(stack[0]), stack[1]);
  pop();
  pop();
}

void VM::concatenate() {
  ObjString* b = AS_STRING(peek(0));
  ObjString* a = AS_STRING(peek(1));

  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString* result = take_string(chars, length);

  pop();
  pop();
  push(OBJ_VAL(result));
}

InterpretResult VM::run() {
  CallFrame* frame = &frames[frame_count - 1];

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() (frame->ip += 2, static_cast<uint16_t>((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())

#define BINARY_OP(value_type, op) \
    do { \
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
        runtime_error("Operands must be numbers."); \
        return INTERPRET_RUNTIME_ERROR; \
      } \
      double b = AS_NUMBER(pop()); \
      double a = AS_NUMBER(pop()); \
      push(value_type(a op b)); \
    } while (false)

  for (;;) {

#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value* slot = stack; slot < stack_top; slot++) {
      printf("[ ");
      print_value(*slot);
      printf(" ]");
    }
    printf("\n");
    disassemble_instruction(&frame->closure->function->chunk, static_cast<int>(frame->ip - frame->closure->function->chunk.code));
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
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
      case OP_POP: 
        pop(); 
        break;
      case OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
        push(frame->slots[slot]);
        break;
      }
      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        frame->slots[slot] = peek(0);
        break;
      }
      case OP_GET_GLOBAL: {
        ObjString* name = READ_STRING();
        Value value;
        if (!globals.get(name, &value)) {
          runtime_error("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }
      case OP_DEFINE_GLOBAL: {
        ObjString* name = READ_STRING();
        globals.set(name, peek(0));
        pop();
        break;
      }
      case OP_SET_GLOBAL: {
        ObjString* name = READ_STRING();
        if (globals.set(name, peek(0))) {
          globals.remove(name); 
          runtime_error("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_GET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        push(*frame->closure->upvalues[slot]->location);
        break;
      }
      case OP_SET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        *frame->closure->upvalues[slot]->location = peek(0);
        break;
      }
      case OP_GET_PROPERTY: {
        if (!IS_INSTANCE(peek(0))) {
          runtime_error("Only instances have properties.");
          return INTERPRET_RUNTIME_ERROR;
        }
        ObjInstance* instance = AS_INSTANCE(peek(0));
        ObjString* name = READ_STRING();
        Value value;
        if (instance->fields.get(name, &value)) {
          pop(); 
          push(value);
          break;
        }
        if (!bind_method(instance->klass, name)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_SET_PROPERTY: {
        if (!IS_INSTANCE(peek(1))) {
          runtime_error("Only instances have fields.");
          return INTERPRET_RUNTIME_ERROR;
        }
        ObjInstance* instance = AS_INSTANCE(peek(1));
        instance->fields.set(READ_STRING(), peek(0));
        Value value = pop();
        pop();
        push(value);
        break;
      }
      case OP_GET_SUPER: {
        ObjString* name = READ_STRING();
        ObjClass* superclass = AS_CLASS(pop());
        
        if (!bind_method(superclass, name)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(values_equal(a, b)));
        break;
      }
      case OP_GREATER:  
        BINARY_OP(BOOL_VAL, >); 
        break;
      case OP_LESS:
        BINARY_OP(BOOL_VAL, <); 
        break;
      case OP_ADD: {
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(a + b));
        } else {
          runtime_error("Operands must be two numbers or two strings.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
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
        int64_t b = static_cast<int64_t>(AS_NUMBER(pop()));
        int64_t a = static_cast<int64_t>(AS_NUMBER(pop()));
        push(NUMBER_VAL(static_cast<double>(a / b)));
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
      case OP_NOT:
        push(BOOL_VAL(is_falsey(pop())));
        break;
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          runtime_error("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(NUMBER_VAL(-AS_NUMBER(pop())));
        break;
      case OP_PRINT: {
        print_value(pop());
        printf("\n");
        break;
      }
      case OP_JUMP: {
        uint16_t offset = READ_SHORT();
        frame->ip += offset;
        break;
      }
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        if (is_falsey(peek(0))) frame->ip += offset;
        break;
      }
      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
        frame->ip -= offset;
        break;
      }
      case OP_CALL: {
        int arg_count = READ_BYTE();
        if (!call_value(peek(arg_count), arg_count)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &frames[frame_count - 1];
        break;
      }
      case OP_INVOKE: {
        ObjString* method = READ_STRING();
        int arg_count = READ_BYTE();
        if (!invoke(method, arg_count)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &frames[frame_count - 1];
        break;
      }
      case OP_SUPER_INVOKE: {
        ObjString* method = READ_STRING();
        int arg_count = READ_BYTE();
        ObjClass* superclass = AS_CLASS(pop());
        if (!invoke_from_class(superclass, method, arg_count)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &frames[frame_count - 1];
        break;
      }
      case OP_CLOSURE: {
        ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
        ObjClosure* closure = new ObjClosure(function, make_upvalue_array(function->upvalue_count));
        push(OBJ_VAL(closure));
        for (int i = 0; i < closure->upvalue_count; i++) {
          uint8_t is_local = READ_BYTE();
          uint8_t index = READ_BYTE();
          if (is_local) {
            closure->upvalues[i] = capture_upvalue(frame->slots + index);
          } else {
            closure->upvalues[i] = frame->closure->upvalues[index];
          }
        }
        break;
      }
      case OP_CLOSE_UPVALUE:
        close_upvalues(stack_top - 1);
        pop();
        break;
      case OP_RETURN: {
        Value result = pop();
        close_upvalues(frame->slots);
        frame_count--;
        if (frame_count == 0) {
          pop();
          return INTERPRET_OK;
        }
        stack_top = frame->slots;
        push(result);
        frame = &frames[frame_count - 1];
        break;
      }
      case OP_CLASS:
        push(OBJ_VAL(new ObjClass(READ_STRING())));
        break;
      case OP_INHERIT: {
        Value superclass = peek(1);
        if (!IS_CLASS(superclass)) {
          runtime_error("Superclass must be a class.");
          return INTERPRET_RUNTIME_ERROR;
        }
        ObjClass* subclass = AS_CLASS(peek(0));
        table_add_all(&AS_CLASS(superclass)->methods, &subclass->methods);
        pop(); 
        break;
      }
      case OP_METHOD:
        define_method(READ_STRING());
        break;
    }
  }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

