#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "obj.h"
#include "table.h"
#include "val.h"

#define TABLE_MAX_LOAD 0.75

void init_table(Table* table) {
	table->size = 0;
	table->cap = 0;
	table->entries = NULL;
}

void free_table(Table* table) {
	FREE_ARR(Entry, table->entries, table->cap);
	init_table(table);
}

bool table_set(Table* table, ObjString* key, Val val) {
	if (table->size >= table->cap * TABLE_MAX_LOAD) adjust_cap(table, GROW_CAP(table->cap));

	Entry* entry = find_entry(table->entries, table->cap, key);
	bool is_new_key = entry->key == nullptr;
	if (is_new_key && IS_NIL(entry->val)) table->size++;

	entry->key = key;
	entry->val = val;
	return is_new_key;
}

bool table_get(Table* table, ObjString* key, Val* val) {
	if (table->size == 0) return false;
	Entry* entry = find_entry(table->entries, table->cap, key);
	if (entry->key == nullptr) return false;
	*val = entry->val;
	return true;
}

bool table_remove(Table* table, ObjString* key) {
	if (table->size == 0) return false;
	Entry* entry = find_entry(table->entries, table->cap, key);
	if (entry->key == nullptr) return false;
	entry->key = nullptr;
	entry->val = BOOL_VAL(true);
	return true;
}

void table_add_all(Table* from, Table* to) {
	for (int i = 0; i < from->cap; i++) {
		Entry* entry = &from->entries[i];
		if (entry->key != nullptr) table_set(to, entry->key, entry->val);
	}
}

ObjString* table_find_string(Table* table, const char* chars, int len, uint32_t hash) {
	if (table->size == 0) return nullptr;
	uint32_t i = hash % table->cap;
	for (;;) {
		Entry* entry = &table->entries[i];
		if (entry->key == nullptr) {
			if (IS_NIL(entry->val)) return nullptr;
		} else if (
			entry->key->len == len && 
			entry->key->hash == hash && 
			memcmp(entry->key->chars, chars, len) == 0
		) {
			return entry->key;
		}
		i = (i + 1) % table->cap;
	}
}

static Entry* find_entry(Entry* entries, int cap, ObjString* key) {
	uint32_t i = key->hash % cap;
	Entry* tombstone = nullptr;
	for (;;) {
		Entry* entry = &entries[i];
		if (entry->key == key) return entry;
		if (entry->key == nullptr) {
			if (IS_NIL(entry->val)) return tombstone != nullptr ? tombstone : entry;
			else if (tombstone == nullptr) tombstone = entry;
		}
		i = (i + 1) % cap;
	}
}

static void adjust_cap(Table* table, int new_cap) {
	Entry* entries = ALLOCATE(Entry, new_cap);
	for (int i = 0; i < new_cap; i++) {
		entries[i].key = NULL;
		entries[i].val = NIL_VAL;
	}
	table->size = 0;
	for (int i = 0; i < table->cap; i++) {
		Entry* entry = &table->entries[i];
		if (entry->key != nullptr) {
			Entry* dest = find_entry(entries, new_cap, entry->key);
			dest->key = entry->key;
			dest->val = entry->val;
			table->size++;
		}
	}
	FREE_ARR(Entry, table->entries, table->cap);
	table->entries = entries;
	table->cap = new_cap;
}
