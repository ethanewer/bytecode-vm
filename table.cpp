#include <stdlib.h>
#include <string.h>
#include "obj.h"
#include "table.h"
#include "val.h"

#define TABLE_MAX_LOAD 0.75

bool Table::set(ObjString* key, Val val) {
	if (size >= cap * TABLE_MAX_LOAD) adjust_cap(cap < 8 ? 8 : 2 * cap);
	Entry* entry = find_entry(entries, cap, key);
	bool is_new_key = entry->key == nullptr;
	if (is_new_key && IS_NIL(entry->val)) size++;
	entry->key = key;
	entry->val = val;
	return is_new_key;
}

bool Table::get(ObjString* key, Val* val) {
	if (size == 0) return false;
	Entry* entry = find_entry(entries, cap, key);
	if (entry->key == nullptr) return false;
	*val = entry->val;
	return true;
}

bool Table::remove(ObjString* key) {
	if (size == 0) return false;
	Entry* entry = find_entry(entries, cap, key);
	if (entry->key == nullptr) return false;
	entry->key = nullptr;
	entry->val = BOOL_VAL(true);
	return true;
}

void table_add_all(Table* from, Table* to) {
	for (int i = 0; i < from->cap; i++) {
		Entry* entry = &from->entries[i];
		if (entry->key != nullptr) to->set(entry->key, entry->val);
	}
}

ObjString* Table::find_string(const char* chars, int len, uint32_t hash) {
	if (size == 0) return nullptr;
	uint32_t i = hash % cap;
	for (;;) {
		Entry* entry = &entries[i];
		if (entry->key == nullptr) {
			if (IS_NIL(entry->val)) return nullptr;
		} else if (
			entry->key->len == len && 
			entry->key->hash == hash && 
			memcmp(entry->key->chars, chars, len) == 0
		) {
			return entry->key;
		}
		i = (i + 1) % cap;
	}
}

Entry* Table::find_entry(Entry* entries, int cap, ObjString* key) {
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

void Table::adjust_cap(int new_cap) {
	Entry* new_entries = (Entry*) malloc(new_cap * sizeof(Entry));
	for (int i = 0; i < new_cap; i++) {
		new_entries[i].key = NULL;
		new_entries[i].val = NIL_VAL;
	}
	size = 0;
	for (int i = 0; i < cap; i++) {
		Entry* entry = &entries[i];
		if (entry->key != nullptr) {
			Entry* dest = find_entry(new_entries, new_cap, entry->key);
			dest->key = entry->key;
			dest->val = entry->val;
			size++;
		}
	}
	free(entries);
	entries = new_entries;
	cap = new_cap;
}

void Table::clear() {
	free(entries);
	size = 0;
	cap = 0;
	entries = nullptr;
}