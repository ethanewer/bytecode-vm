#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "compiler.h"
#include "scanner.h"

typedef struct {
	Token curr;
	Token prev;
	bool had_error;
	bool panic_mode;
} Parser;

Parser parser;
Chunk* compiling_chunk;

static Chunk* curr_chunk() {
	return compiling_chunk;
}

static void error_at(Token* token, const char* msg) {
	if (parser.panic_mode) return;
	parser.panic_mode = true;
	
	fprintf(stderr, "[line %d] Error", token->line);

	if (token->type == TOKEN_EOF) fprintf(stderr, " at end");
	else if (token->type == TOKEN_ERROR) return;
	else fprintf(stderr, " at '%.*s'", token->len, token->start);

	fprintf(stderr, ": %s\n", msg);
	parser.had_error = true;
}

static void error_at_curr(const char* msg) {
  	error_at(&parser.curr, msg);
}

static void error(const char* msg) {
  	error_at(&parser.prev, msg);
}

static void advance() {
	parser.prev = parser.curr;
	for (;;) {
		parser.curr = scan_token();
		if (parser.curr.type != TOKEN_ERROR) break;
		error_at_curr(parser.curr.start);
	}
}

static void consume(TokenType type, const char* msg) {
	if (parser.curr.type == type) advance();
	else error_at_curr(msg);
}

static void emit_byte(uint8_t byte) {
	add_chunk(curr_chunk(), byte, parser.prev.line);
}

static void emit_bytes(uint8_t byte1, uint8_t byte2) {
	emit_byte(byte1);
	emit_byte(byte2);
}

static void emit_return() {
	emit_byte(OP_RETURN);
}

static uint8_t make_constant(Val val) {
	int constant = add_constant(curr_chunk(), val);
	if (constant > UINT8_MAX) {
		error("Too many constants in one chunk.");
		return 0;
	}
	return (uint8_t) constant;
}

static void emit_constant(Val val) {
	emit_bytes(OP_CONSTANT, make_constant(val));
}

static void number() {
	double val = strtod(parser.prev.start, NULL);
	emit_constant(val);
}

static void expression() {

}

static void grouping() {
  	expression();
  	consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary() {
	TokenType op_type = parser.prev.type;

	expression();

	switch (op_type) {
		case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
		default: return;
	}
}

bool compile(const char* source, Chunk* chunk) {
  	init_scanner(source);
	compiling_chunk = chunk;
	parser.had_error = parser.panic_mode = false;

	int line = -1;
	advance();
	expression();
	consume(TOKEN_EOF, "Expect end of expression.");
	emit_return();
	return !parser.had_error;
}