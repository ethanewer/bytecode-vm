#ifndef table_h
#define table_h

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
void table_add_all(Table* from, Table* to);
ObjString* table_find_string(Table* table, const char* chars, int len, uint32_t hash);
static Entry* find_entry(Entry* entries, int cap, ObjString* key);
static void adjust_cap(Table* table, int new_cap);

#endif