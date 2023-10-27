#include "common.h"
#include "table.h"

#define GC_HEAP_GROW_FACTOR 2

#define ALLOCATE(type, count) \
    (type*) reallocate(nullptr, 0, (count) * sizeof(type))

#define FREE(type, ptr) reallocate(ptr, sizeof(type), 0)

#define GROW_ARR(type, ptr, old_len, new_len) \
    (type*) reallocate(ptr, sizeof(type) * (old_len), sizeof(type) * (new_len))

#define FREE_ARR(type, ptr, old_len) \
    reallocate(ptr, sizeof(type) * (old_len), 0)

void* reallocate(void* pointer, size_t oldSize, size_t newSize);
void free_obj(Obj* obj);
void collect_garbage();
void mark_val(Val val);
void mark_obj(Obj* obj);
void mark_table(Table* table);