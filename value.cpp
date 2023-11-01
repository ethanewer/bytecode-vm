#include <stdio.h>
#include <string.h>
#include "object.hpp"
#include "memory.hpp"
#include "value.hpp"

ValueArray::ValueArray() : capacity(0), count(0), values(nullptr) {}

void ValueArray::write(Value value) {
  if (capacity < count + 1) {
    int old_capacity = capacity;
    capacity = GROW_CAPACITY(old_capacity);
    values = GROW_ARRAY(Value, values, old_capacity, capacity);
  }
  values[count++] = value;
}

void ValueArray::clear() {
  FREE_ARRAY(Value, values, capacity);
  capacity = 0;
  count = 0;
  values = nullptr;
}

void print_value(Value value) {
#ifdef NAN_BOXING
  if (IS_BOOL(value)) {
    printf(AS_BOOL(value) ? "true" : "false");
  } else if (IS_NIL(value)) {
    printf("nil");
  } else if (IS_NUMBER(value)) {
    printf("%g", AS_NUMBER(value));
  } else if (IS_OBJ(value)) {
    print_object(value);
  }
#else
  switch (value.type) {
    case VAL_BOOL:
      printf(AS_BOOL(value) ? "true" : "false");
      break;
    case VAL_NIL: printf("nil"); break;
    case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;

    case VAL_OBJ: print_object(value); break;

  }
#endif
}

bool values_equal(Value a, Value b) {
#ifdef NAN_BOXING
  return a == b;
#else
  if (a.type != b.type) return false;
  switch (a.type) {
    case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:    return true;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ:    return AS_OBJ(a) == AS_OBJ(b);
    default:         return false; 
  }

#endif
}

