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
  ObjString* findString(const char* chars, int length, uint32_t hash);
  void removeWhite();
  void mark();

private:
  static Entry* findEntry(Entry* entries, int capacity, ObjString* key);
  void adjustCapacity(int newCapacity);
};

void tableAddAll(Table* from, Table* to);

#endif
