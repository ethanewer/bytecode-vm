#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "val.h"

struct Entry {
	ObjString* key;
	Val val;
};

struct Table {
	int size;
	int cap;
	Entry* entries;
};

void init_table(Table* table);
void free_table(Table* table);
bool table_set(Table* table, ObjString* key, Val val);
bool table_get(Table* table, ObjString* key, Val* val);
bool table_remove(Table* table, ObjString* key);
static Entry* find_entry(Entry* entries, int cap, ObjString* key);
static void adjust_cap(Table* table, int new_cap);
void table_add_all(Table* from, Table* to);

#endif