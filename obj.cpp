#include <stdio.h>
#include <string.h>
#include "obj.h"
#include "table.h"
#include "val.h"
#include "vm.h"
#include "gc.h"

Obj::Obj(ObjType type) : type(type), marked(false) {
	#ifdef DEBUG_GC
		printf("%p allocating type %d\n", this, type);
	#endif

	this->next = vm.objs;
  	vm.objs = this;
}

ObjString::ObjString(char* chars, int len, uint32_t hash) : Obj(OBJ_STRING), chars(chars), len(len), hash(hash) {
	vm.strings.set(this, NIL_VAL);
}

ObjFn::ObjFn() : Obj(OBJ_FN), num_params(0), num_upvalues(0), name(nullptr) {}

ObjClosure::ObjClosure(ObjFn* fn) : Obj(OBJ_CLOSURE), fn(fn), num_upvalues(fn->num_upvalues) {
	upvalues = ALLOCATE(ObjUpvalue*, num_upvalues);
	for (int i = 0; i < num_upvalues; i++) upvalues[i] = nullptr;
}

ObjUpvalue::ObjUpvalue(Val* location) : Obj(OBJ_UPVALUE), location(location), closed(NIL_VAL), next(nullptr) {}

ObjNative::ObjNative(NativeFn fn) : Obj(OBJ_NATIVE) {
	this->fn = fn;
}

ObjString* copy_string(const char* chars, int len) {
	uint32_t hash = hash_string(chars, len);
	ObjString* interned = vm.strings.find_string(chars, len, hash);
	if (interned != nullptr) {
		return interned;
	}

	char* heap_chars = ALLOCATE(char, len + 1);
	memcpy(heap_chars, chars, len);
	heap_chars[len] = '\0';
	ObjString* string = ALLOCATE(ObjString, 1);
	new (string) ObjString(heap_chars, len, hash);
	return string;
}

ObjString* take_string(char* chars, int len) {
	uint32_t hash = hash_string(chars, len);
  	ObjString* string = ALLOCATE(ObjString, 1);
	new (string) ObjString(chars, len, hash);
	return string;
}

void print_obj(Val val) {
	switch (OBJ_TYPE(val)) {
		case OBJ_STRING:
			printf("%s", AS_CSTRING(val));
			break;
		case OBJ_FN:
			print_fn(AS_FN(val));
			break;
		case OBJ_CLOSURE:
			print_fn(AS_CLOSURE(val)->fn);
			break;
		case OBJ_UPVALUE:
			printf("upvalue");
			break;
		case OBJ_NATIVE:
			printf("<native fn>");
			break;
	}
}

static uint32_t hash_string(const char* key, int len) {
	uint32_t hash = 2166136261u;
	for (int i = 0; i < len; i++) {
		hash ^= (uint8_t) key[i];
		hash *= 16777619;
	}
	return hash;
}

static void print_fn(ObjFn* fn) {
	if (fn->name != nullptr) printf("<fn %s>", fn->name->chars);
	else  printf("<fn nullptr>");
}