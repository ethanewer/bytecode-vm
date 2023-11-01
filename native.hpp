#ifndef native_h
#define native_h

#include "value.hpp"

Value number_native(int arg_count, Value* args);
Value string_native(int arg_count, Value* args);
Value bool_native(int arg_count, Value* args);
Value print_native(int arg_count, Value* args);
Value println_native(int arg_count, Value* args);
Value input_native(int arg_count, Value* val);
Value clock_native(int arg_count, Value* args);

#endif