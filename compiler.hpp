#ifndef compiler_h
#define compiler_h

#include "object.hpp"
#include "vm.hpp"

ObjFunction* compile(const char* source);
void mark_compiler_roots();

#endif
