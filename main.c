#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, const char* argv[]) {
	Chunk chunk;
	init_chunk(&chunk);

	int constant = add_constant(&chunk, 69);
	add_chunk(&chunk, OP_CONSTANT, 2);
	add_chunk(&chunk, constant, 2);

	add_chunk(&chunk, OP_RETURN, 2);

	disassemble_chunk(&chunk, "test chunk");

	free_chunk(&chunk);
	return 0;
}