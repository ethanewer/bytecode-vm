#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "native.hpp"
#include "object.hpp"
#include "vm.hpp"

Value numberNative(int argCount, Value* args) {
  if (argCount != 1) {
    runtimeError("Expected 1 argument but got %d.", argCount);
    return NIL_VAL;
  }
  Value value = args[0];
  if (IS_NUMBER(value)) {
    return value;
  } else if (IS_STRING(value)) {
    char* s = AS_CSTRING(value);
    double n = strtod(s, nullptr);
    if (n == 0 && s[0] != '0') {
      runtimeError("Cannot convert to number.");
      return NIL_VAL;
    }
    return NUMBER_VAL(n);
  } else if (IS_BOOL(value)) {
    bool b = AS_BOOL(value);
    return NUMBER_VAL(b ? 1 : 0);
  } else {
    runtimeError("Cannot convert to number.");
    return NIL_VAL;
  }
}

Value stringNative(int argCount, Value* args) {
  if (argCount != 1) {
    runtimeError("Expected 1 argument but got %d.", argCount);
    return NIL_VAL;
  }
  Value value = args[0];
  if (IS_NUMBER(value)) {
    double n = AS_NUMBER(value);
    char* buffer =(char*)malloc(30 * sizeof(char));
    size_t length = snprintf(buffer, sizeof(buffer), "%g", n);
    printf("%zu %zu\n", length, sizeof(buffer));
    if (length > sizeof(buffer)) {
      free(buffer);
      runtimeError("Buffer too small to store the string.");
      return NIL_VAL;
    }
    return OBJ_VAL(takeString(buffer, length));
  } else if (IS_STRING(value)) {
    return value;
  } else if (IS_BOOL(value)) {
    bool b = AS_BOOL(value);
    if (b) {
      char* buffer = (char*)malloc(5 * sizeof(char));
      memcpy(buffer, "true", 5);
      return OBJ_VAL(takeString(buffer, 4));
    } else {
      char* buffer = (char*)malloc(6 * sizeof(char));
      memcpy(buffer, "false", 6);
      return OBJ_VAL(takeString(buffer, 5));
    }
  } else {
    runtimeError("Cannot convert to number.");
    return NIL_VAL;
  }
}

Value boolNative(int argCount, Value* args) {
  if (argCount != 1) runtimeError("Expected 1 argument but got %d.", argCount);
  Value value = args[0];
  return BOOL_VAL(!(IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value))));
}

Value printNative(int argCount, Value* args) {
  if (argCount > 0) {
    for (int i = 0; i < argCount - 1; i++) {
      printValue(args[i]);
      printf(" ");
    }
    printValue(args[argCount - 1]);
  }
  return NIL_VAL;
}

Value printlnNative(int argCount, Value* args) {
  if (argCount > 0) {
    for (int i = 0; i < argCount - 1; i++) {
      printValue(args[i]);
      printf(" ");
    }
    printValue(args[argCount - 1]);
  }
  printf("\n");
  return NIL_VAL;
}

Value inputNative(int argCount, Value* val) {
  char* buffer = nullptr;
  size_t length = 0;
  ssize_t charactersRead = getline(&buffer, &length, stdin);
  if (charactersRead == -1) return NIL_VAL;
  for (int i = length - 1; i >= 0; i--) {
    if (buffer[i] == '\n') {
      buffer[i] = '\0';
      break;
    }
  }
  return OBJ_VAL(takeString(buffer, length));
}

Value clockNative(int argCount, Value* args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}