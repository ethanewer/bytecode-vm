#include <stdlib.h>
#include "chunk.hpp"
#include "memory.hpp"
#include "vm.hpp"

Chunk::Chunk() : count(0), capacity(0), code(nullptr), lines(nullptr) {}

void Chunk::clear() {
  FREE_ARRAY(uint8_t, code, capacity);
  FREE_ARRAY(int, lines, capacity);
  constants.clear();
  count = 0;
  capacity = 0;
  code = nullptr;
  lines = nullptr;
}

void Chunk::write(uint8_t byte, int line) {
  if (capacity < count + 1) {
    int old_capacity = capacity;
    capacity = GROW_CAPACITY(old_capacity);
    code = GROW_ARRAY(uint8_t, code, old_capacity, capacity);
    lines = GROW_ARRAY(int, lines, old_capacity, capacity);
  }

  code[count] = byte;
  lines[count] = line;
  count++;
}

int Chunk::add_constant(Value value) {
  push(value);
  constants.write(value);
  pop();
  return constants.count - 1;
}
