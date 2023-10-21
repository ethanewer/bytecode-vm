#ifndef mem_h
#define mem_h

#include "obj.h"
#include "common.h"

#define GROW_CAP(cap) ((cap) < 8 ? 8 : (cap) * 2)

#define GROW_ARR(type, ptr, old_cap, new_cap) \
	(type*) reallocate(ptr, sizeof(type) * (old_cap), sizeof(type) * (new_cap))

#define FREE_ARR(type, ptr, old_cap) \
	reallocate((void*) ptr, sizeof(type) * (old_cap), 0)

#define ALLOCATE(type, count) \
    (type*) reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, ptr) reallocate(ptr, sizeof(type), 0)

void* reallocate(void* ptr, size_t old_size, size_t new_size);
void free_objs();
static void free_obj(Obj* obj);


#endif