#include <stdlib.h>
#include "mem.h"
#include "vm.h"

void* reallocate(void* ptr, size_t old_size, size_t new_size) {
	if (new_size == 0) {
		free(ptr);
		return nullptr;
	}
	void* res = realloc(ptr, new_size);
	if (res == nullptr) exit(1);
	return res;
}

static void free_obj(Obj* obj) {
	switch (obj->type) {
		case OBJ_STRING: {
			ObjString* string = (ObjString*) obj;
			FREE_ARR(char, string->chars, string->len + 1);
			FREE(ObjString, obj);
			break;
		}
		case OBJ_FN: {
			ObjFn* fn = (ObjFn*) obj;
			fn->chunk.clear();
			FREE(ObjFn, obj);
			break;
		}
		case OBJ_NATIVE:
			FREE(ObjNative, obj);
			break;
	}
}

void free_objs() {
	Obj* node = vm.objs;
	while (node != nullptr) {
		Obj* next = node->next;
		free_obj(node);
		node = next;
	}
}
