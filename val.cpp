#include <stdio.h>
#include "mem.h"
#include "val.h"

void init_val_arr(ValArr* arr) {
	arr->len = 0;
	arr->cap = 0;
	arr->vals = nullptr;
}

void add_val_arr(ValArr* arr, Val val) {
	if (arr->len >= arr->cap) {
		int old_cap = arr->cap;
		arr->cap = GROW_CAP(old_cap);
		arr->vals = GROW_ARR(Val, arr->vals, old_cap, arr->cap);
	}
	arr->vals[arr->len++] = val;
}

void free_val_arr(ValArr* arr) {
	FREE_ARR(Val, arr->vals, arr->cap);
	init_val_arr(arr);
}

void print_val(Val val) {
	printf("%g", val);
}


