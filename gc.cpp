#include "gc.h"
#include "compiler.h"
#include "vm.h"
#include <stdio.h>

void* reallocate(void* pointer, size_t old_size, size_t new_size) {
	vm.bytes_allocated += new_size - old_size;
	// if (vm.bytes_allocated > vm.next_gc) collect_garbage();

	#ifdef DEBUG_GC
		// collect_garbage();
	#endif

	if (new_size == 0) {
		free(pointer);
		return nullptr;
	}
	void* res = realloc(pointer, new_size);
	if (res == nullptr) exit(1);
	return res;
}

void free_chunk(Chunk* chunk) {
	FREE_ARR(uint8_t, chunk->code, chunk->cap);
	FREE_ARR(int, chunk->lines, chunk->cap);
	FREE_ARR(Val, chunk->constants, chunk->constants_cap);
}

void free_obj(Obj* obj) {
	#ifdef DEBUG_GC
		printf("%p free type %d\n", obj, obj->type);
	#endif

	switch (obj->type) {
		case OBJ_STRING: {
			ObjString* string = (ObjString*) obj;
			FREE_ARR(char, string->chars, string->len + 1);
			FREE(ObjString, obj);
			break;
		}
		case OBJ_FN: {
			ObjFn* fn = static_cast<ObjFn*>(obj);
			free_chunk(&fn->chunk);
			FREE(ObjFn, fn);
			break;
		}
		case OBJ_CLOSURE: {
			ObjClosure* closure = static_cast<ObjClosure*>(obj);
			FREE_ARR(ObjUpvalue*, closure->upvalues, closure->num_upvalues);
			FREE(ObjClosure, closure);
			break;
		}
		case OBJ_UPVALUE:
			FREE(ObjUpvalue, obj);
			break;
		case OBJ_NATIVE:
			FREE(ObjNative, obj);
			break;
	}
}
