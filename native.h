#ifndef native_h
#define native_h

#include "value.h"


Value numberNative(int argCount, Value* args);
Value stringNative(int argCount, Value* args);
Value boolNative(int argCount, Value* args);
Value printNative(int argCount, Value* args);
Value printlnNative(int argCount, Value* args);
Value inputNative(int argCount, Value* val);
Value clockNative(int argCount, Value* args);


#endif