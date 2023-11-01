#ifndef table_h
#define table_h

#include "common.hpp"
#include "value.hpp"

struct Entry {
  ObjString* key;
  Value value;
};

class Table {
public:
  int count;
  int capacity;
  Entry* entries;

  Table();
  void clear();
  bool get(ObjString* key, Value* value);
  bool set(ObjString* key, Value value);
  bool remove(ObjString* key);
  ObjString* find_string(const char* chars, int length, uint32_t hash);
  void remove_white();
  void mark();

private:
  static Entry* find_entry(Entry* entries, int capacity, ObjString* key);
  void adjust_capacity(int new_capacity);
};

void table_add_all(Table* from, Table* to);

#endif
