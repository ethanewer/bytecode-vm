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
    int oldCapacity = capacity;
    capacity = GROW_CAPACITY(oldCapacity);
    code = GROW_ARRAY(uint8_t, code, oldCapacity, capacity);
    lines = GROW_ARRAY(int, lines, oldCapacity, capacity);
  }

  code[count] = byte;
  lines[count] = line;
  count++;
}

int Chunk::addConstant(Value value) {
  push(value);
  constants.write(value);
  pop();
  return constants.count - 1;
}
