#ifndef object_h
#define object_h

#include "common.hpp"
#include "chunk.hpp"
#include "table.hpp"
#include "value.hpp"

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define IS_BOUND_METHOD(value)    is_obj_type(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value)           is_obj_type(value, OBJ_CLASS)
#define IS_CLOSURE(value)         is_obj_type(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)        is_obj_type(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)        is_obj_type(value, OBJ_INSTANCE)
#define IS_NATIVE(value)          is_obj_type(value, OBJ_NATIVE)
#define IS_NATIVE_INSTANCE(value) is_obj_type(value, OBJ_NATIVE_INSTANCE)
#define IS_STRING(value)          is_obj_type(value, OBJ_STRING)

#define AS_BOUND_METHOD(value)    static_cast<ObjBoundMethod*>(AS_OBJ(value))
#define AS_CLASS(value)           static_cast<ObjClass*>(AS_OBJ(value))
#define AS_CLOSURE(value)         static_cast<ObjClosure*>(AS_OBJ(value))
#define AS_FUNCTION(value)        static_cast<ObjFunction*>(AS_OBJ(value))
#define AS_INSTANCE(value)        static_cast<ObjInstance*>(AS_OBJ(value))
#define AS_NATIVE(value)          (static_cast<ObjNative*>(AS_OBJ(value))->function)
#define AS_NATIVE_INSTANCE(value) static_cast<ObjNativeInstance*>(AS_OBJ(value))
#define AS_STRING(value)          static_cast<ObjString*>(AS_OBJ(value))
#define AS_CSTRING(value)         (static_cast<ObjString*>(AS_OBJ(value))->chars)

enum ObjType {
  OBJ_BOUND_METHOD,
  OBJ_CLASS,
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_INSTANCE,
  OBJ_NATIVE,
  OBJ_NATIVE_INSTANCE,
  OBJ_STRING,
  OBJ_UPVALUE
};

struct Obj {
  ObjType type;
  bool is_marked;
  Obj* next;

  Obj(ObjType type);
};

struct ObjFunction : public Obj {
  int arity;
  int upvalue_count;
  Chunk chunk;
  ObjString* name;

  ObjFunction();
  void* operator new(size_t size);
};

using NativeFn = Value(*)(int arg_count, Value* args);

struct ObjNative : public Obj {
  NativeFn function;

  ObjNative(NativeFn function);
  void* operator new(size_t size);
};

struct ObjString : public Obj {
  int length;
  char* chars;
  uint32_t hash;

  ObjString(char* chars, int length, uint32_t hash);
  void* operator new(size_t size);
};

struct ObjUpvalue : public Obj {
  Value* location;
  Value closed;
  ObjUpvalue* next;

  ObjUpvalue(Value* slot);
  void* operator new(size_t size);
};

struct ObjClosure : public Obj {
  ObjFunction* function;
  ObjUpvalue** upvalues;
  int upvalue_count;

  ObjClosure(ObjFunction* function, ObjUpvalue** upvalues);
  void* operator new(size_t size);
};

struct ObjClass : public Obj {
  ObjString* name;
  Table methods;

  ObjClass(ObjString* name);
  void* operator new(size_t size);
};

struct ObjInstance : public Obj {
  ObjClass* klass;
  Table fields; 

  ObjInstance(ObjClass* klass);
  void* operator new(size_t size);
};

struct ObjBoundMethod : public Obj {
  Value receiver;
  ObjClosure* method;

  ObjBoundMethod(Value receiver, ObjClosure* method);
  void* operator new(size_t size);
};

enum NativeType {
  NATIVE_LIST
};

struct ObjNativeInstance : public Obj {
  NativeType native_type;

  ObjNativeInstance(NativeType native_type);
};

ObjString* take_string(char* chars, int length);
ObjString* copy_string(const char* chars, int length);
ObjUpvalue** make_upvalue_array(int count);
void print_object(Value value);

static inline bool is_obj_type(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
