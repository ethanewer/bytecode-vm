#include <stdlib.h>
#include "mem.h"

void* reallocate(void* ptr, size_t old_size, size_t new_size) {
	if (new_size == 0) {
		free(ptr);
		return nullptr;
	}
	void* res = realloc(ptr, new_size);
	if (res == nullptr) exit(1);
	return res;
}
