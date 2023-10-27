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

#define AS_STRING(val) (static_cast<ObjString*>(AS_OBJ(val)))
#define AS_CSTRING(val) (static_cast<ObjString*>(AS_OBJ(val))->chars)
#define AS_FN(val) (static_cast<ObjFn*>(AS_OBJ(val)))
#define AS_CLOSURE(val) (static_cast<ObjClosure*>(AS_OBJ(val)))
#define AS_NATIVE(val) (static_cast<ObjNative*>(AS_OBJ(val))->fn)

enum ObjType {
  	OBJ_STRING,
	OBJ_FN,
	OBJ_CLOSURE,
	OBJ_UPVALUE,
	OBJ_NATIVE
};

struct Obj {
  	ObjType type;
	bool marked;
	Obj* next;

	Obj(ObjType type);
};

struct ObjString : Obj {
	char* chars;
	int len;
	uint32_t hash;

	ObjString(char* chars, int len, uint32_t hash);
};

struct ObjFn : Obj {
	int num_params;
	int num_upvalues;
	Chunk chunk;
	ObjString* name;

	ObjFn();
};

struct ObjUpvalue : Obj {
	Val* location;
	Val closed;
	ObjUpvalue* next;

	ObjUpvalue(Val* location);
};

struct ObjClosure : Obj {
	ObjFn* fn;
	ObjUpvalue** upvalues;
	int num_upvalues;

	ObjClosure(ObjFn* fn);
};

using NativeFn = Val (*)(int num_args, Val* args);

struct ObjNative : Obj{
	NativeFn fn;

	ObjNative(NativeFn fn);
};

static inline bool is_obj_type(Val val, ObjType type) {
  	return IS_OBJ(val) && AS_OBJ(val)->type == type;
}

ObjString* copy_string(const char* chars, int len);
ObjString* take_string(char* chars, int len);
void print_obj(Val val);
static uint32_t hash_string(const char* key, int len);
static void print_fn(ObjFn* fn);
#endif