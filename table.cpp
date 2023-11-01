#include <stdlib.h>
#include <string.h>
#include "memory.hpp"
#include "object.hpp"
#include "table.hpp"
#include "value.hpp"

#define TABLE_MAX_LOAD 0.75

Table::Table() : count(0), capacity(0), entries(nullptr) {}

void Table::clear() {
  FREE_ARRAY(Entry, entries, capacity);
  count = 0;
  capacity = 0;
  entries = nullptr;
}

bool Table::get(ObjString* key, Value* value) {
  if (count == 0) return false;
  Entry* entry = find_entry(entries, capacity, key);
  if (entry->key == nullptr) return false;
  *value = entry->value;
  return true;
}

bool Table::set(ObjString* key, Value value) {
  if (count + 1 > capacity * TABLE_MAX_LOAD) {
    adjust_capacity(GROW_CAPACITY(capacity));
  }
  Entry* entry = find_entry(entries, capacity, key);
  bool is_new_key = entry->key == nullptr;
  if (is_new_key && IS_NIL(entry->value)) count++;
  entry->key = key;
  entry->value = value;
  return is_new_key;
}

bool Table::remove(ObjString* key) {
  if (count == 0) return false;
  Entry* entry = find_entry(entries, capacity, key);
  if (entry->key == nullptr) return false;
  entry->key = nullptr;
  entry->value = BOOL_VAL(true);
  return true;
}

ObjString* Table::find_string(const char* chars, int length, uint32_t hash) {
  if (count == 0) return nullptr;
  uint32_t index = hash & (capacity - 1);
  for (;;) {
    Entry* entry = &entries[index];
    if (entry->key == nullptr) {
      if (IS_NIL(entry->value)) return nullptr;
    } else if (entry->key->length == length &&
        entry->key->hash == hash &&
        memcmp(entry->key->chars, chars, length) == 0
    ) {
      return entry->key;
    }
    index = (index + 1) & (capacity - 1);
  }
}

void Table::remove_white() {
  for (int i = 0; i < capacity; i++) {
    Entry* entry = &entries[i];
    if (entry->key != nullptr && !entry->key->is_marked) {
      remove(entry->key);
    }
  }
}

void Table::mark() {
  for (int i = 0; i < capacity; i++) {
    Entry* entry = &entries[i];
    mark_object((Obj*)entry->key);
    mark_value(entry->value);
  }
}

Entry* Table::find_entry(Entry* entries, int capacity, ObjString* key) {
  uint32_t index = key->hash & (capacity - 1);
  Entry* tombstone = nullptr;
  for (;;) {
    Entry* entry = &entries[index];
    if (entry->key == nullptr) {
      if (IS_NIL(entry->value)) {
        return tombstone != nullptr ? tombstone : entry;
      } else {
        if (tombstone == nullptr) tombstone = entry;
      }
    } else if (entry->key == key) {
      return entry;
    }
    index = (index + 1) & (capacity - 1);
  }
}

void Table::adjust_capacity(int new_capacity) {
  Entry* new_entries = ALLOCATE(Entry, new_capacity);
  for (int i = 0; i < new_capacity; i++) {
    new_entries[i].key = nullptr;
    new_entries[i].value = NIL_VAL;
  }
  count = 0;
  for (int i = 0; i < capacity; i++) {
    Entry* entry = &entries[i];
    if (entry->key == nullptr) continue;
    Entry* dest = find_entry(new_entries, new_capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    count++;
  }
  FREE_ARRAY(Entry, entries, capacity);
  entries = new_entries;
  capacity = new_capacity;
}

void table_add_all(Table* from, Table* to) {
  for (int i = 0; i < from->capacity; i++) {
    Entry* entry = &from->entries[i];
    if (entry->key != nullptr) to->set(entry->key, entry->value);
  }
}
