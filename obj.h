#ifndef object_h
#define object_h

#include "common.h"
#include "chunk.h"
#include "val.h"

#define OBJ_TYPE(val) (AS_OBJ(val)->type)

#define IS_STRING(val) is_obj_type(val, OBJ_STRING)
#define IS_FN(val) is_obj_type(val, OBJ_FN)

#define AS_STRING(val) ((ObjString*) AS_OBJ(val))
#define AS_CSTRING(val) (((ObjString*) AS_OBJ(val))->chars)
#define AS_FN(val) ((ObjFn*) AS_OBJ(val))

enum ObjType {
  	OBJ_STRING,
	OBJ_FN,
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

struct ObjFn {
	Obj obj;
	int num_params;
	Chunk chunk;
	ObjString* name;
};

static inline bool is_obj_type(Val val, ObjType type) {
  return IS_OBJ(val) && AS_OBJ(val)->type == type;
}

ObjFn* new_fn();
ObjString* copy_string(const char* chars, int len);
ObjString* allocate_string(char* chars, int len, uint32_t hash);
void print_obj(Val val);
ObjString* take_string(char* chars, int len);
static Obj* allocate_object(size_t size, ObjType type);
static uint32_t hash_string(const char* key, int len);
static void print_fn(ObjFn* fn);
#endif