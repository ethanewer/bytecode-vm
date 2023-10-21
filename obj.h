#ifndef object_h
#define object_h

#include "common.h"
#include "val.h"

#define OBJ_TYPE(val) (AS_OBJ(val)->type)

#define IS_STRING(val) is_obj_type(val, OBJ_STRING)

#define AS_STRING(value) ((ObjString*) AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*) AS_OBJ(value))->chars)

enum ObjType {
  	OBJ_STRING,
};

struct Obj {
  	ObjType type;
	Obj* next;
};

struct ObjString {
	Obj obj;
	int len;
	char* chars;
};

static inline bool is_obj_type(Val val, ObjType type) {
  return IS_OBJ(val) && AS_OBJ(val)->type == type;
}

ObjString* copy_string(const char* chars, int len);
static ObjString* allocate_string(char* chars, int len);
void print_obj(Val val);
ObjString* take_string(char* chars, int len);

#endif