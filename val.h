#ifndef value_h
#define value_h

#include "common.h"

struct Obj;
struct ObjString;

enum ValType {
	VAL_BOOL,
	VAL_NUMBER,
	VAL_NIL,
	VAL_OBJ,
};

struct Val {
	ValType type;
	union {
		bool boolean;
		double number;
		Obj* obj;
	} as;	
};

#define BOOL_VAL(val) ((Val) {VAL_BOOL, {.boolean = val}})
#define NUMBER_VAL(val) ((Val) {VAL_NUMBER, {.number = val}})
#define NIL_VAL ((Val) {VAL_NIL, {.number = 0}})
#define OBJ_VAL(object) ((Val) {VAL_OBJ, {.obj = (Obj*) object}})

#define AS_BOOL(val) ((val).as.boolean)
#define AS_NUMBER(val) ((val).as.number)
#define AS_OBJ(val) ((val).as.obj)

#define IS_BOOL(val) ((val).type == VAL_BOOL)
#define IS_NUMBER(val) ((val).type == VAL_NUMBER)
#define IS_NIL(val) ((val).type == VAL_NIL)
#define IS_OBJ(val) ((val).type == VAL_OBJ)

void print_val(Val val);
bool is_truthy(Val val);
bool is_equal(Val val1, Val val2);

#endif