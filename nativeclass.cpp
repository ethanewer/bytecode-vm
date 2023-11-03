#include <string.h>
#include <stdio.h>
#include "nativeclass.hpp"
#include "memory.hpp"
#include "vm.hpp"

Value native_instance_call(ObjNativeInstance* object, ObjString* name, int arg_count, Value* args) {
	switch (object->native_type) {
		case NATIVE_LIST: return (static_cast<ObjNativeList*>(object))->call(name, arg_count, args);
		case NATIVE_MAP: return (static_cast<ObjNativeMap*>(object))->call(name, arg_count, args);
	}
}

ObjNativeList::ObjNativeList() : ObjNativeInstance(NATIVE_LIST) {}

void* ObjNativeList::operator new(size_t size) {
	return reallocate(nullptr, 0, size);
}

Value ObjNativeList::call(ObjString* name, int arg_count, Value* args) {
	if (name->length < 3) goto name_error;

	switch (name->chars[0]) {
		case 'p': {
			switch (name->chars[1]) {
				case 'u': {
					if (name->length != 4 || name->chars[2] != 's' || name->chars[3] != 'h') goto name_error;
					if (arg_count != 1) {
						vm.runtime_error("Expected 1 arguments but got %d.", arg_count);
						vm.had_native_error = true;
						return NIL_VAL;
					}
					push(args[0]);
					return NIL_VAL;
				}
				case 'o': {
					if (name->length != 3 || name->chars[2] != 'p') goto name_error;
					if (arg_count != 0) return NIL_VAL;
					return pop();
				}
			}
			break;
		}
		case 's': {
			if (name->length != 3 || name->chars[1] != 'e' || name->chars[2] != 't') goto name_error;
			if (arg_count != 2) {
				vm.runtime_error("Expected 2 arguments but got %d.", arg_count);
				vm.had_native_error = true;
				return NIL_VAL;
			}
			set(args[0], args[1]);
			return NIL_VAL;
		}
		case 'g': {
			if (name->length != 3 || name->chars[1] != 'e' || name->chars[2] != 't') goto name_error;
			if (arg_count != 1) {
				vm.runtime_error("Expected 1 arguments but got %d.", arg_count);
				vm.had_native_error = true;
				return NIL_VAL;
			}
			return get(args[0]);
		}
		case 'l': {
			if (name->length != 3 || name->chars[1] != 'e' || name->chars[2] != 'n') goto name_error;
			if (arg_count != 0) {
				vm.runtime_error("Expected 0 arguments but got %d.", arg_count);
				vm.had_native_error = true;
				return NIL_VAL;
			}
			return NUMBER_VAL(list.count);
		}
	}

name_error:
	vm.runtime_error("'%s' is not a method of 'List'.", name->chars);
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
		vm.runtime_error("Index must be a number.");
		vm.had_native_error = true;
		return;
	}
	int i = static_cast<int>(AS_NUMBER(idx));
	if (i < 0 || i >= list.count) {
		vm.runtime_error("Index out of bounds.");
		vm.had_native_error = true;
		return;
	}
	list.values[i] = value;
}

Value ObjNativeList::get(Value idx) {
	if (!IS_NUMBER(idx)) {
		vm.runtime_error("Index must be a number.");
		vm.had_native_error = true;
		return NIL_VAL;
	}
	int i = static_cast<int>(AS_NUMBER(idx));
	if (i < 0 || i >= list.count) {
		vm.runtime_error("Index out of bounds.");
		vm.had_native_error = true;
		return NIL_VAL;
	}
	return list.values[i];
}

ObjNativeMap::ObjNativeMap() : ObjNativeInstance(NATIVE_MAP) {}

void* ObjNativeMap::operator new(size_t size) {
	return reallocate(nullptr, 0, size);
}

Value ObjNativeMap::call(ObjString* name, int arg_count, Value* args) {
	if (name->length < 3) goto name_error;

	switch (name->chars[0]) {
		case 's': {
			switch (name->chars[1]) {
				case 'e': {
					if (name->length != 3 || name->chars[2] != 't') goto name_error;
					if (arg_count != 2) {
						vm.runtime_error("Expected 2 arguments but got %d.", arg_count);
						vm.had_native_error = true;
						return NIL_VAL;
					}
					set(args[0], args[1]);
					return NIL_VAL;
				}
				case 'i': {
					if (name->length != 4 || name->chars[2] != 'z' || name->chars[3] != 'e') goto name_error;
					if (arg_count != 0) {
						vm.runtime_error("Expected 0 arguments but got %d.", arg_count);
						vm.had_native_error = true;
						return NIL_VAL;
					}
					return NUMBER_VAL(map.count);
				}
			}
			
		}
		case 'g': {
			if (name->length != 3 || name->chars[1] != 'e' || name->chars[2] != 't') goto name_error;
			if (arg_count != 1) {
				vm.runtime_error("Expected 1 arguments but got %d.", arg_count);
				vm.had_native_error = true;
				return NIL_VAL;
			}
			return get(args[0]);
		}
		case 'h': {
			if (name->length != 3 || name->chars[1] != 'a' || name->chars[2] != 's') goto name_error;
			if (arg_count != 1) {
				vm.runtime_error("Expected 1 arguments but got %d.", arg_count);
				vm.had_native_error = true;
				return NIL_VAL;
			}
			return has(args[0]);
		}
		case 'r': {
			if (name->length != 6 || memcmp(name->chars, "remove", 6)) goto name_error;
			if (arg_count != 1) {
				vm.runtime_error("Expected 1 arguments but got %d.", arg_count);
				vm.had_native_error = true;
				return NIL_VAL;
			}
			remove(args[0]);
			return NIL_VAL;
		}
		case 'e': {
			if (name->length != 7 || memcmp(name->chars, "entries", 7)) goto name_error;
			if (arg_count != 0) {
				vm.runtime_error("Expected 0 arguments but got %d.", arg_count);
				vm.had_native_error = true;
				return NIL_VAL;
			}
			return entries_list();
		}
	}

name_error:
	vm.runtime_error("'%s' is not a method of 'Map'.", name->chars);
	vm.had_native_error = true;
	return NIL_VAL;
}

void ObjNativeMap::set(Value key, Value value) {
	if (key == NIL_VAL) {
		vm.runtime_error("Key cannot be nil.");
		vm.had_native_error = true;
	}
	map.set(key, value);
}

Value ObjNativeMap::get(Value key) {
	if (key == NIL_VAL) {
		vm.runtime_error("Key cannot be nil.");
		vm.had_native_error = true;
		return NIL_VAL;
	}
	Value value;
	if (!map.get(key, &value)) {
		vm.runtime_error("Invalid key.");
		vm.had_native_error = true;
		return NIL_VAL;
	}
	return value;
}

Value ObjNativeMap::has(Value key) {
	if (key == NIL_VAL) {
		vm.runtime_error("Key cannot be nil.");
		vm.had_native_error = true;
		return NIL_VAL;
	}
	Value value;
	return BOOL_VAL(map.get(key, &value));
}

void ObjNativeMap::remove(Value key) {
	if (key == NIL_VAL) {
		vm.runtime_error("Key cannot be nil.");
		vm.had_native_error = true;
	}
	map.remove(key);
}

Value ObjNativeMap::entries_list() {
	ObjNativeList* list = new ObjNativeList();
	vm.push(OBJ_VAL(list));
	for (int i = 0; i < map.capacity; i++) {
		if (map.entries[i].key != NIL_VAL) {
			ObjNativeList* entry = new ObjNativeList();
			vm.push(OBJ_VAL(entry));
			entry->push(map.entries[i].key);
			entry->push(map.entries[i].value);
			list->push(OBJ_VAL(entry));
			vm.pop();
		}
	}
	vm.pop();
	return OBJ_VAL(list);
}
