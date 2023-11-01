#ifndef nativeclass_h
#define nativeclass_h

#include "value.hpp"
#include "object.hpp"

Value nativeInstanceCall(ObjNativeInstance* object, ObjString* name, int argCount, Value* args);

struct ObjNativeList : public ObjNativeInstance {
  ValueArray list;

  ObjNativeList();
  void* operator new(size_t size);
  Value call(ObjString* name, int argCount, Value* args);
  void push(Value value);
  Value pop();
  void set(Value idx, Value value);
  Value get(Value idx);
};

inline Value nativeList(int argCount, Value* args) {
	return OBJ_VAL(new ObjNativeList());
}

#endif
