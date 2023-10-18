#ifndef value_h
#define value_h

#include "common.h"

typedef double Val;

typedef struct {
	int len;
	int cap;
	Val* vals;
} ValArr;

void init_val_arr(ValArr* arr);
void add_val_arr(ValArr* arr, Val val);
void free_val_arr(ValArr* arr);
void print_val(Val val);

#endif