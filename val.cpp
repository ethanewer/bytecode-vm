#include <stdio.h>
#include <string.h>
#include "obj.h"
#include "val.h"

void print_val(Val val) {
	switch (val.type) {
		case VAL_BOOL:
			printf(AS_BOOL(val) ? "true" : "false");
			break;
		case VAL_NIL: 
			printf("nil");
			break;
		case VAL_NUMBER:
			printf("%g", AS_NUMBER(val));
			break;
		case VAL_OBJ:
			print_obj(val);
			break;
	}
}

bool is_truthy(Val val) {
	if (IS_NIL(val)) return false;
	if (IS_BOOL(val)) return AS_BOOL(val);
	if (IS_NUMBER(val)) return AS_NUMBER(val) != 0;
	return true;
}

bool is_equal(Val val1, Val val2) {
	if (val1.type != val2.type) return false;
	switch (val1.type) {
		case VAL_BOOL: return AS_BOOL(val1) == AS_BOOL(val2);
		case VAL_NUMBER: return AS_NUMBER(val1) == AS_NUMBER(val2);
		case VAL_NIL: return true;
		case VAL_OBJ: return AS_OBJ(val1) == AS_OBJ(val2);
		default: return false;
	}
}


