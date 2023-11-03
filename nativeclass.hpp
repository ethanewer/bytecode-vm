#ifndef nativeclass_h
#define nativeclass_h

#include "value.hpp"
#include "object.hpp"
#include "map.hpp"

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

struct ObjNativeMap : public ObjNativeInstance {
  Map map;

  ObjNativeMap();
  void* operator new(size_t size);
  Value call(ObjString* name, int arg_count, Value* args);
  void set(Value key, Value value);
  Value get(Value key);
  Value has(Value key);
  void remove(Value key);
  Value entries_list();
};

inline Value native_map(int arg_count, Value* args) {
	return OBJ_VAL(new ObjNativeMap());
}

#endif
