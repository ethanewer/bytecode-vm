#ifndef nativeclass_h
#define nativeclass_h

#include "value.hpp"
#include "object.hpp"

Value native_instance_call(ObjNativeInstance* object, ObjString* name, int arg_count, Value* args);

struct ObjNativeList : public ObjNativeInstance {
  ValueArray list;

  ObjNativeList();
  void* operator new(size_t size);
  Value call(ObjString* name, int arg_count, Value* args);
  void push(Value value);
  Value pop();
  void set(Value idx, Value value);
  Value get(Value idx);
};

inline Value native_list(int arg_count, Value* args) {
	return OBJ_VAL(new ObjNativeList());
}

#endif
