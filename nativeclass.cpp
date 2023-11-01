#include <string.h>
#include <stdio.h>
#include "nativeclass.hpp"
#include "memory.hpp"

Value nativeInstanceCall(ObjNativeInstance* object, ObjString* name, int argCount, Value* args) {
	switch (object->nativeType) {
		case NATIVE_LIST: return ((ObjNativeList*)object)->call(name, argCount, args);
	}
}

ObjNativeList::ObjNativeList() : ObjNativeInstance(NATIVE_LIST) {}

void* ObjNativeList::operator new(size_t size) {
	return reallocate(nullptr, 0, size);
}

Value ObjNativeList::call(ObjString* name, int argCount, Value* args) {
	if (name->length < 3) return NIL_VAL;;

	switch (name->chars[0]) {
		case 'p': {
			switch (name->chars[1]) {
				case 'u': {
					if (name->length != 4 || name->chars[2] != 's' || name->chars[3] != 'h') return NIL_VAL;;
					if (argCount != 1) return NIL_VAL;;
					push(args[0]);
					return NIL_VAL;
				}
				case 'o': {
					if (name->length != 3 || name->chars[2] != 'p') return NIL_VAL;;
					if (argCount != 0) return NIL_VAL;;
					return pop();
				}
			}
		}
		case 's': {
			if (name->length != 3 || name->chars[1] != 'e' || name->chars[2] != 't') return NIL_VAL;;
			if (argCount != 2) return NIL_VAL;;
			set(args[0], args[1]);
			return NIL_VAL;
		}
		case 'g': {
			if (name->length != 3 || name->chars[1] != 'e' || name->chars[2] != 't') return NIL_VAL;;
			if (argCount != 1) return NIL_VAL;;
			return get(args[0]);
		}
		case 'l': {
			if (name->length != 3 || name->chars[1] != 'e' || name->chars[2] != 'n') return NIL_VAL;;
			if (argCount != 0) return NIL_VAL;;
			return NUMBER_VAL(list.count);
		}
	}

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