#include <stdio.h>
#include <string.h>
#include "memory.hpp"
#include "object.hpp"
#include "table.hpp"
#include "value.hpp"
#include "vm.hpp"

Obj::Obj(ObjType type) : type(type), isMarked(false) {
  next = vm.objects;
  vm.objects = this;
}

ObjFunction::ObjFunction() : Obj(OBJ_FUNCTION), arity(0), upvalueCount(0), name(nullptr) {
  initChunk(&chunk);
}

void* ObjFunction::operator new(size_t size) {
  return reallocate(nullptr, 0, size);
}

ObjNative::ObjNative(NativeFn function) : Obj(OBJ_NATIVE), function(function) {}

void* ObjNative::operator new(size_t size) {
  return reallocate(nullptr, 0, size); 
}

ObjString::ObjString(char* chars, int length, uint32_t hash) 
  : Obj(OBJ_STRING), length(length), chars(chars), hash(hash) {

  push(OBJ_VAL(this));
  vm.strings.set(this, NIL_VAL);
  pop();
}

void* ObjString::operator new(size_t size) {
  return reallocate(nullptr, 0, size);
}

ObjUpvalue::ObjUpvalue(Value* slot) : Obj(OBJ_UPVALUE), location(slot), closed(NIL_VAL), next(nullptr) {} 

void* ObjUpvalue::operator new(size_t size) {
  return reallocate(nullptr, 0, size);
}

ObjClosure::ObjClosure(ObjFunction* function, ObjUpvalue** upvalues) 
  : Obj(OBJ_CLOSURE), function(function), upvalues(upvalues), upvalueCount(function->upvalueCount) {}

void* ObjClosure::operator new(size_t size) {
  return reallocate(nullptr, 0, size);
}

ObjClass::ObjClass(ObjString* name) : Obj(OBJ_CLASS), name(name) {}

void* ObjClass::operator new(size_t size) {
  return reallocate(nullptr, 0, size);
}

ObjInstance::ObjInstance(ObjClass* klass) : Obj(OBJ_INSTANCE), klass(klass) {}

void* ObjInstance::operator new(size_t size) {
  return reallocate(nullptr, 0, size);
}

ObjBoundMethod::ObjBoundMethod(Value receiver, ObjClosure* method) 
  : Obj(OBJ_BOUND_METHOD), receiver(receiver), method(method) {}

void* ObjBoundMethod::operator new(size_t size) {
  return reallocate(nullptr, 0, size);
}

static uint32_t hashString(const char* key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

ObjString* takeString(char* chars, int length) {
  uint32_t hash = hashString(chars, length);
  ObjString* interned = vm.strings.findString(chars, length, hash);
  if (interned != nullptr) {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }
  return new ObjString(chars, length, hash);
}

ObjString* copyString(const char* chars, int length) {
  uint32_t hash = hashString(chars, length);
  ObjString* interned = vm.strings.findString(chars, length, hash);
  if (interned != nullptr) return interned;
  char* heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  return new ObjString(heapChars, length, hash);
}

ObjUpvalue** makeUpvalueArray(int count) {
  ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, count);
  for (int i = 0; i < count; i++) {
    upvalues[i] = nullptr;
  }
  return upvalues;
}

static void printFunction(ObjFunction* function) {
  if (function->name == nullptr) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", function->name->chars);
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_BOUND_METHOD:
      printFunction(AS_BOUND_METHOD(value)->method->function);
      break;
    case OBJ_CLASS:
      printf("%s", AS_CLASS(value)->name->chars);
      break;
    case OBJ_CLOSURE:
      printFunction(AS_CLOSURE(value)->function);
      break;
    case OBJ_FUNCTION:
      printFunction(AS_FUNCTION(value));
      break;
    case OBJ_INSTANCE:
      printf("%s instance",  AS_INSTANCE(value)->klass->name->chars);
      break;
    case OBJ_NATIVE:
      printf("<native fn>");
      break;
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
    case OBJ_UPVALUE:
      printf("upvalue");
      break;
  }
}

