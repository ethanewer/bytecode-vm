#include <string.h>
#include <stdio.h>
#include "nativeclass.hpp"
#include "memory.hpp"

Value native_instance_call(ObjNativeInstance* object, ObjString* name, int arg_count, Value* args) {
	switch (object->native_type) {
		case NATIVE_LIST: return ((Obj_native_list*)object)->call(name, arg_count, args);
	}
}

Obj_native_list::Obj_native_list() : ObjNativeInstance(NATIVE_LIST) {}

void* Obj_native_list::operator new(size_t size) {
	return reallocate(nullptr, 0, size);
}

Value Obj_native_list::call(ObjString* name, int arg_count, Value* args) {
	if (name->length < 3) return NIL_VAL;;

	switch (name->chars[0]) {
		case 'p': {
			switch (name->chars[1]) {
				case 'u': {
					if (name->length != 4 || name->chars[2] != 's' || name->chars[3] != 'h') return NIL_VAL;;
					if (arg_count != 1) return NIL_VAL;;
					push(args[0]);
					return NIL_VAL;
				}
				case 'o': {
					if (name->length != 3 || name->chars[2] != 'p') return NIL_VAL;;
					if (arg_count != 0) return NIL_VAL;;
					return pop();
				}
			}
		}
		case 's': {
			if (name->length != 3 || name->chars[1] != 'e' || name->chars[2] != 't') return NIL_VAL;;
			if (arg_count != 2) return NIL_VAL;;
			set(args[0], args[1]);
			return NIL_VAL;
		}
		case 'g': {
			if (name->length != 3 || name->chars[1] != 'e' || name->chars[2] != 't') return NIL_VAL;;
			if (arg_count != 1) return NIL_VAL;;
			return get(args[0]);
		}
		case 'l': {
			if (name->length != 3 || name->chars[1] != 'e' || name->chars[2] != 'n') return NIL_VAL;;
			if (arg_count != 0) return NIL_VAL;;
			return NUMBER_VAL(list.count);
		}
	}

	return NIL_VAL;
}

void Obj_native_list::push(Value value) {
	list.write(value);
}

Value Obj_native_list::pop() {
	return list.values[--list.count];
}

void Obj_native_list::set(Value idx, Value value) {
	if (!IS_NUMBER(idx)) {
		return;
	}
	int i = (int)(AS_NUMBER(idx));
	if (i < 0 || i >= list.count) {
		return;
	}
	list.values[i] = value;
}

Value Obj_native_list::get(Value idx) {
	if (!IS_NUMBER(idx)) {
		return NIL_VAL;
	}
	int i = (int)(AS_NUMBER(idx));
	if (i < 0 || i >= list.count) {
		return NIL_VAL;
	}
	return list.values[i];
}