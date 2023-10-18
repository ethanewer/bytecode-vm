#ifndef debug_h
#define debug_h

#include "chunk.h"

void disassemble_chunk(Chunk* chunk, const char* name);
int assamble_chunk(Chunk* chunk, int offset);
int disassemble_instruction(Chunk* chunk, int offset);
static int simple_instruction(const char* name, int offset);
static int constant_instruction(const char* name, Chunk* chunk, int offset);

#endif