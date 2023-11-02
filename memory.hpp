#ifndef memory_h
#define memory_h

#include "common.hpp"
#include "object.hpp"

#define ALLOCATE(type, count) static_cast<type*>(reallocate(NULL, 0, sizeof(type) * (count)))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, old_count, new_count) \
  static_cast<type*>(reallocate(pointer, sizeof(type) * (old_count), sizeof(type) * (new_count)))

#define FREE_ARRAY(type, pointer, old_count) reallocate(pointer, sizeof(type) * (old_count), 0)

void* reallocate(void* pointer, size_t old_size, size_t new_size);
void mark_object(Obj* object);
void mark_value(Value value);
void collect_garbage();
void free_objects();

#endif
