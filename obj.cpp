#include <stdio.h>
#include <string.h>
#include "mem.h"
#include "obj.h"
#include "table.h"
#include "val.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, object_type) \
    (type*) allocate_object(sizeof(type), object_type)

ObjFn* new_fn() {
	ObjFn* fn = ALLOCATE_OBJ(ObjFn, OBJ_FN);
	fn->num_params = 0;
	fn->name = nullptr;
	init_chunk(&fn->chunk);
	return fn;
}

ObjNative* new_native(NativeFn fn) {
	ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
	native->fn = fn;
	return native;
}

ObjString* copy_string(const char* chars, int len) {
	uint32_t hash = hash_string(chars, len);
	ObjString* interned = table_find_string(&vm.strings, chars, len, hash);
	if (interned != nullptr) {
		return interned;
	}

	char* heap_chars = ALLOCATE(char, len + 1);
	memcpy(heap_chars, chars, len);
	heap_chars[len] = '\0';
	return allocate_string(heap_chars, len, hash);
}

ObjString* allocate_string(char* chars, int len, uint32_t hash) {
	ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	string->len = len;
	string->chars = chars;
	string->hash = hash;
	table_set(&vm.strings, string, NIL_VAL);
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
		case OBJ_NATIVE:
			printf("<native fn>");
			break;
	}
}

ObjString* take_string(char* chars, int len) {
	uint32_t hash = hash_string(chars, len);
  	return allocate_string(chars, len, hash);
}

static Obj* allocate_object(size_t size, ObjType type) {
	Obj* obj = (Obj*) reallocate(NULL, 0, size);
	obj->type = type;
	obj->next = vm.objs;
  	vm.objs = obj;
	return obj;
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
	printf("<fn %s>", fn->name->chars);
}