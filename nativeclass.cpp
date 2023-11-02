#include <string.h>
#include <stdio.h>
#include "nativeclass.hpp"
#include "memory.hpp"
#include "vm.hpp"

Value native_instance_call(ObjNativeInstance* object, ObjString* name, int arg_count, Value* args) {
	switch (object->native_type) {
		case NATIVE_LIST: return (static_cast<ObjNativeList*>(object))->call(name, arg_count, args);
	}
}

ObjNativeList::ObjNativeList() : ObjNativeInstance(NATIVE_LIST) {}

void* ObjNativeList::operator new(size_t size) {
	return reallocate(nullptr, 0, size);
}

Value ObjNativeList::call(ObjString* name, int arg_count, Value* args) {
	if (name->length < 3) return NIL_VAL;;

	switch (name->chars[0]) {
		case 'p': {
			switch (name->chars[1]) {
				case 'u': {
					if (name->length != 4 || name->chars[2] != 's' || name->chars[3] != 'h') goto name_error;
					if (arg_count != 1) {
						runtime_error("Expected 1 arguments but got %d.", arg_count);
						vm.had_native_error = true;
						return NIL_VAL;
					}
					push(args[0]);
					return NIL_VAL;
				}
				case 'o': {
					if (name->length != 3 || name->chars[2] != 'p') goto name_error;
					if (arg_count != 0) return NIL_VAL;;
					return pop();
				}
			}
		}
		case 's': {
			if (name->length != 3 || name->chars[1] != 'e' || name->chars[2] != 't') goto name_error;
			if (arg_count != 2) {
				runtime_error("Expected 2 arguments but got %d.", arg_count);
				vm.had_native_error = true;
				return NIL_VAL;
			}
			set(args[0], args[1]);
			return NIL_VAL;
		}
		case 'g': {
			if (name->length != 3 || name->chars[1] != 'e' || name->chars[2] != 't') goto name_error;
			if (arg_count != 1) {
				runtime_error("Expected 1 arguments but got %d.", arg_count);
				vm.had_native_error = true;
				return NIL_VAL;
			}
			return get(args[0]);
		}
		case 'l': {
			if (name->length != 3 || name->chars[1] != 'e' || name->chars[2] != 'n') goto name_error;
			if (arg_count != 0) {
				runtime_error("Expected 0 arguments but got %d.", arg_count);
				vm.had_native_error = true;
				return NIL_VAL;
			}
			return NUMBER_VAL(list.count);
		}
	}

name_error:
	runtime_error("'%s' is not a method of 'List'.", name->chars);
	vm.had_native_error = true;
	return NIL_VAL;
}

void ObjNativeList::push(Value value) {
	list.write(value);
}

Value ObjNativeList::pop() {
	return list.values[--list.count];
}

void ObjNativeList::set(Value idx, Value value) {
	if (!IS_NUMBER(idx)) {
		return;
	}
	int i = (int)(AS_NUMBER(idx));
	if (i < 0 || i >= list.count) {
		return;
	}
	list.values[i] = value;
}

Value ObjNativeList::get(Value idx) {
	if (!IS_NUMBER(idx)) {
		return NIL_VAL;
	}
	int i = (int)(AS_NUMBER(idx));
	if (i < 0 || i >= list.count) {
		return NIL_VAL;
	}
	return list.values[i];
}