#ifndef object_h
#define object_h

#include "common.hpp"
#include "chunk.hpp"
#include "table.hpp"
#include "value.hpp"

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define IS_BOUND_METHOD(value)    isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value)           isObjType(value, OBJ_CLASS)
#define IS_CLOSURE(value)         isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)        isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)        isObjType(value, OBJ_INSTANCE)
#define IS_NATIVE(value)          isObjType(value, OBJ_NATIVE)
#define IS_NATIVE_INSTANCE(value) isObjType(value, OBJ_NATIVE_INSTANCE)
#define IS_STRING(value)          isObjType(value, OBJ_STRING)

#define AS_BOUND_METHOD(value)    ((ObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value)           ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value)         ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value)        ((ObjFunction*)AS_OBJ(value))
#define AS_INSTANCE(value)        ((ObjInstance*)AS_OBJ(value))
#define AS_NATIVE(value)          (((ObjNative*)AS_OBJ(value))->function)
#define AS_NATIVE_INSTANCE(value) ((ObjNativeInstance*)AS_OBJ(value))
#define AS_STRING(value)          ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)         (((ObjString*)AS_OBJ(value))->chars)

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
  bool isMarked;
  Obj* next;

  Obj(ObjType type);
};

struct ObjFunction : public Obj {
  int arity;
  int upvalueCount;
  Chunk chunk;
  ObjString* name;

  ObjFunction();
  void* operator new(size_t size);
};

using NativeFn = Value(*)(int argCount, Value* args);

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
  int upvalueCount;

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
  NativeType nativeType;

  ObjNativeInstance(NativeType nativeType);
};

ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
ObjUpvalue** makeUpvalueArray(int count);
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
