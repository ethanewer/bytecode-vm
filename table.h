#ifndef table_h
#define table_h

#include "common.h"
#include "val.h"

struct Entry {
	ObjString* key;
	Val val;
};

class Table {
public:
	int size;
	int cap;
	Entry* entries;

	Table() : size(0), cap(0), entries(nullptr) {}
	bool set(ObjString* key, Val val);
	bool get(ObjString* key, Val* val);
	bool remove(ObjString* key);
	ObjString* find_string(const char* chars, int len, uint32_t hash);
	void clear();

private:
	Entry* find_entry(Entry* entries, int cap, ObjString* key);
 	void adjust_cap(int new_cap);
};

void table_add_all(Table* from, Table* to);

#endif