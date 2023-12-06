#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "native.hpp"
#include "object.hpp"
#include "vm.hpp"

Value number_native(int arg_count, Value* args) {
  if (arg_count != 1) {
    vm.runtime_error("Expected 1 argument but got %d.", arg_count);
		vm.had_native_error = true;
    return NIL_VAL;
  }
  Value value = args[0];
  if (IS_NUMBER(value)) {
    return value;
  } else if (IS_STRING(value)) {
    char* s = AS_CSTRING(value);
    double n = strtod(s, nullptr);
    if (n == 0 && s[0] != '0') {
      vm.runtime_error("Cannot convert to number.");
		  vm.had_native_error = true;
      return NIL_VAL;
    }
    return NUMBER_VAL(n);
  } else if (IS_BOOL(value)) {
    bool b = AS_BOOL(value);
    return NUMBER_VAL(b ? 1 : 0);
  } else {
    vm.runtime_error("Cannot convert to number.");
		vm.had_native_error = true;
    return NIL_VAL;
  }
}

static Value make_string(const char* str) {
  int length = strlen(str);
  return OBJ_VAL(copy_string(str, length));
}

Value string_native(int arg_count, Value* args) {
  if (arg_count != 1) {
    vm.runtime_error("Expected 1 argument but got %d.", arg_count);
		vm.had_native_error = true;
		vm.had_native_error = true;
    return NIL_VAL;
  }
  Value value = args[0];
  if (IS_NUMBER(value)) {
    double n = AS_NUMBER(value);
    char* buffer = static_cast<char*>(malloc(30 * sizeof(char)));
    size_t length = snprintf(buffer, sizeof(buffer), "%g", n);
    if (length > sizeof(buffer)) {
      free(buffer);
      vm.runtime_error("Buffer too small to store the string.");
		  vm.had_native_error = true;
      return NIL_VAL;
    }
    return OBJ_VAL(take_string(buffer, length));
  } else if (IS_STRING(value)) {
    return value;
  } else if (IS_BOOL(value)) {
    bool b = AS_BOOL(value);
    if (b) return make_string("true");
    else return make_string("false");
  } else if (IS_OBJ(value)) {
    switch (OBJ_TYPE(value)) {
      case OBJ_BOUND_METHOD:
        return make_string(AS_BOUND_METHOD(value)->method->function->name->chars);
      case OBJ_CLASS:
        return make_string(AS_CLASS(value)->name->chars);
      case OBJ_CLOSURE:
        return make_string(AS_CLOSURE(value)->function->name->chars);
      case OBJ_FUNCTION:
        return make_string(AS_FUNCTION(value)->name->chars);
      case OBJ_INSTANCE:
        return make_string(AS_INSTANCE(value)->klass->name->chars);
      case OBJ_UPVALUE:
        return make_string("upvalue");
      case OBJ_NATIVE_INSTANCE:
        return make_string("native instance");
      case OBJ_NATIVE:
        return make_string("<native fn>");
      default:
        vm.runtime_error("Cannot convert to string.");
		    vm.had_native_error = true;
        return NIL_VAL;
    }
  } else {
    vm.runtime_error("Cannot convert to string.");
		vm.had_native_error = true;
    return NIL_VAL;
  }
}

Value bool_native(int arg_count, Value* args) {
  if (arg_count != 1) {
    vm.runtime_error("Expected 1 argument but got %d.", arg_count);
		vm.had_native_error = true;
    return NIL_VAL;
  }
  Value value = args[0];
  return BOOL_VAL(!is_falsey(value));
}

Value print_native(int arg_count, Value* args) {
  if (arg_count > 0) {
    for (int i = 0; i < arg_count - 1; i++) {
      print_value(args[i]);
      printf(" ");
    }
    print_value(args[arg_count - 1]);
  }
  return NIL_VAL;
}

Value println_native(int arg_count, Value* args) {
  if (arg_count > 0) {
    for (int i = 0; i < arg_count - 1; i++) {
      print_value(args[i]);
      printf(" ");
    }
    print_value(args[arg_count - 1]);
  }
  printf("\n");
  return NIL_VAL;
}

Value input_native(int arg_count, Value* val) {
  char* buffer = nullptr;
  size_t length = 0;
  ssize_t characters_read = getline(&buffer, &length, stdin);
  if (characters_read == -1) return NIL_VAL;
  for (int i = length - 1; i >= 0; i--) {
    if (buffer[i] == '\n') {
      buffer[i] = '\0';
      break;
    }
  }
  return OBJ_VAL(take_string(buffer, length));
}

Value clock_native(int arg_count, Value* args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}
