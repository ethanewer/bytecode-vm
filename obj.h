#ifndef object_h
#define object_h

#include "common.h"
#include "chunk.h"
#include "val.h"

#define OBJ_TYPE(val) (AS_OBJ(val)->type)

#define IS_STRING(val) is_obj_type(val, OBJ_STRING)
#define IS_FN(val) is_obj_type(val, OBJ_FN)
#define IS_CLOSURE(val) is_obj_type(val, OBJ_CLOSURE)
#define IS_NATIVE(val) is_obj_type(val, OBJ_NATIVE)

#define AS_STRING(val) ((ObjString*) AS_OBJ(val))
#define AS_CSTRING(val) (((ObjString*) AS_OBJ(val))->chars)
#define AS_FN(val) ((ObjFn*) AS_OBJ(val))
#define AS_CLOSURE(val) ((ObjClosure*) AS_OBJ(val))
#define AS_NATIVE(val) (((ObjNative*) AS_OBJ(val))->fn)

enum ObjType {
  	OBJ_STRING,
	OBJ_FN,
	OBJ_CLOSURE,
	OBJ_UPVALUE,
	OBJ_NATIVE
};

struct Obj {
  	ObjType type;
	Obj* next;
};

struct ObjString {
	Obj obj;
	int len;
	char* chars;
	uint32_t hash;
};

struct ObjUpvalue {
	Obj obj;
	Val* location;
	Val closed;
	ObjUpvalue* next;
};

struct ObjFn {
	Obj obj;
	int num_params;
  	int num_upvalues;
	Chunk chunk;
	ObjString* name;
};

struct ObjClosure {
	Obj obj;
	ObjFn* fn;
	ObjUpvalue** upvalues;
};

using NativeFn = Val (*)(int num_args, Val* args);

struct ObjNative {
	Obj obj;
	NativeFn fn;
};

static inline bool is_obj_type(Val val, ObjType type) {
  return IS_OBJ(val) && AS_OBJ(val)->type == type;
}

ObjFn* new_fn();
ObjClosure* new_closure(ObjFn* fn);
ObjUpvalue* new_upvalue(Val* slot);
ObjNative* new_native(NativeFn fn);
ObjString* copy_string(const char* chars, int len);
ObjString* allocate_string(char* chars, int len, uint32_t hash);
void print_obj(Val val);
ObjString* take_string(char* chars, int len);
static Obj* allocate_object(size_t size, ObjType type);
static uint32_t hash_string(const char* key, int len);
static void print_fn(ObjFn* fn);
#endif