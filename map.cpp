#include <stdlib.h>
#include <string.h>
#include "memory.hpp"
#include "map.hpp"
#include "value.hpp"

#define MAP_MAX_LOAD 0.75

Map::Map() : count(0), capacity(0), entries(nullptr) {}

void Map::clear() {
  FREE_ARRAY(MapEntry, entries, capacity);
  count = 0;
  capacity = 0;
  entries = nullptr;
}

bool Map::get(Value key, Value* value) {
  if (count == 0) return false;
  MapEntry* entry = find_entry(entries, capacity, key);
  if (entry->key == NIL_VAL) return false;
  *value = entry->value;
  return true;
}

bool Map::set(Value key, Value value) {
  if (count + 1 > capacity * MAP_MAX_LOAD) {
    adjust_capacity(GROW_CAPACITY(capacity));
  }
  MapEntry* entry = find_entry(entries, capacity, key);
  bool is_new_key = entry->key == NIL_VAL;
  if (is_new_key && IS_NIL(entry->value)) count++;
  entry->key = key;
  entry->value = value;
  return is_new_key;
}

bool Map::remove(Value key) {
  if (count == 0) return false;
  MapEntry* entry = find_entry(entries, capacity, key);
  if (entry->key == NIL_VAL) return false;
  entry->key = NIL_VAL;
  entry->value = BOOL_VAL(true);
  return true;
}

void Map::mark() {
  for (int i = 0; i < capacity; i++) {
    MapEntry* entry = &entries[i];
    mark_object((Obj*)entry->key);
    mark_value(entry->value);
  }
}

MapEntry* Map::find_entry(MapEntry* entries, int capacity, Value key) {
  uint32_t index = key & (capacity - 1);
  MapEntry* tombstone = nullptr;
  for (;;) {
    MapEntry* entry = &entries[index];
    if (entry->key == NIL_VAL) {
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

void Map::adjust_capacity(int new_capacity) {
  MapEntry* new_entries = ALLOCATE(MapEntry, new_capacity);
  for (int i = 0; i < new_capacity; i++) {
    new_entries[i].key = NIL_VAL;
    new_entries[i].value = NIL_VAL;
  }
  count = 0;
  for (int i = 0; i < capacity; i++) {
    MapEntry* entry = &entries[i];
    if (entry->key == NIL_VAL) continue;
    MapEntry* dest = find_entry(new_entries, new_capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    count++;
  }
  FREE_ARRAY(MapEntry, entries, capacity);
  entries = new_entries;
  capacity = new_capacity;
}

