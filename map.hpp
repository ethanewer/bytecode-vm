#ifndef map_h
#define map_h

#include "common.hpp"
#include "value.hpp"

struct MapEntry {
  Value key;
  Value value;
};

class Map {
public:
  int count;
  int capacity;
  MapEntry* entries;

  Map();
  void clear();
  bool get(Value key, Value* value);
  bool set(Value key, Value value);
  bool remove(Value key);
  void mark();

private:
  static MapEntry* find_entry(MapEntry* entries, int capacity, Value key);
  void adjust_capacity(int new_capacity);
};

#endif