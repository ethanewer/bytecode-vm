#ifndef value_h
#define value_h

#include <string.h>
#include "common.hpp"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

#ifdef NAN_BOXING

#define SIGN_BIT  ((uint64_t)0x8000000000000000)
#define QNAN      ((uint64_t)0x7ffc000000000000)
#define TAG_NIL   1 
#define TAG_FALSE 2 
#define TAG_TRUE  3 

using Value = uint64_t;

#define IS_BOOL(value)      (((value) | 1) == TRUE_VAL)
#define IS_NIL(value)       ((value) == NIL_VAL)
#define IS_NUMBER(value)    (((value) & QNAN) != QNAN)
#define IS_OBJ(value)       (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value)      ((value) == TRUE_VAL)
#define AS_NUMBER(value)    value_to_num(value)
#define AS_OBJ(value)       reinterpret_cast<Obj*>((value) & ~(SIGN_BIT | QNAN))

#define BOOL_VAL(b)         (FALSE_VAL | (b))
#define FALSE_VAL           static_cast<Value>(QNAN | TAG_FALSE)
#define TRUE_VAL            static_cast<Value>(QNAN | TAG_TRUE)
#define NIL_VAL             static_cast<Value>(QNAN | TAG_NIL)
#define NUMBER_VAL(num)     num_to_value(num)
#define OBJ_VAL(obj)        static_cast<Value>(SIGN_BIT | QNAN | reinterpret_cast<uint64_t>(obj))

static inline double value_to_num(Value value) {
  return *reinterpret_cast<double*>(&value);
}

static inline Value num_to_value(double num) {
  return *reinterpret_cast<Value*>(&num);
}

#else

typedef enum {
  VAL_BOOL,
  VAL_NIL, 
  VAL_NUMBER,
  VAL_OBJ
} Value_type;

typedef struct {
  Value_type type;
  union {
    bool boolean;
    double number;
    Obj* obj;
  } as; 
} Value;

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

#define AS_OBJ(value)     ((value).as.obj)
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)

#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})

#endif

struct ValueArray {
  int capacity;
  int count;
  Value* values;

  ValueArray();
  void write(Value value);
  void clear();
};

bool values_equal(Value a, Value b);
void print_value(Value value);

#endif
