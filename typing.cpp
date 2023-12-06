#include <stdlib.h>
#include <stdio.h>
#include "object.hpp"
#include "vm.hpp"
#include "typing.hpp"

static Value make_string(const char* str) {
  int length = strlen(str);
  return OBJ_VAL(copy_string(str, length));
}

Value type_native(int arg_count, Value* args) {
  if (arg_count != 1) {
    vm.runtime_error("Expected 1 argument but got %d.", arg_count);
		vm.had_native_error = true;
    return NIL_VAL;
  }
  Value value = args[0];
  if (IS_NUMBER(value)) {
    return make_string("number");
  } else if (IS_STRING(value)) {
    return make_string("string");
  } else if (IS_BOOL(value)) {
   return make_string("bool");
  } else if (IS_OBJ(value)) {
    switch (OBJ_TYPE(value)) {
      case OBJ_BOUND_METHOD:
        return make_string("fn");
      case OBJ_CLASS:
        return make_string("fn");
      case OBJ_CLOSURE:
        return make_string("fn");
      case OBJ_FUNCTION:
        return make_string("fn");
      case OBJ_INSTANCE:
        return make_string(AS_INSTANCE(value)->klass->name->chars);
      case OBJ_UPVALUE:
        return make_string("_upvalue");
      case OBJ_NATIVE_INSTANCE:
        return make_string("_native_instance");
      case OBJ_NATIVE:
        return make_string("fn");
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