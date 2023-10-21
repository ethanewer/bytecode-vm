#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "obj.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

struct Parser {
	Token curr;
	Token prev;
	bool had_error;
	bool panic_mode;
};

enum Precedence {
	PREC_NONE,
	PREC_ASSIGNMENT, // =
	PREC_OR, // or
	PREC_AND, // and
	PREC_EQUALITY, // == !=
	PREC_COMPARISON, // < > <= >=
	PREC_TERM, // + -
	PREC_FACTOR, // * /
	PREC_UNARY, // ! -
	PREC_CALL, // . ()
	PREC_PRIMARY
};

typedef void (*ParseFn)();

struct ParseRule {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
};

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

static void expression();
static ParseRule* get_rule(TokenType type);
static void parse_precedence(Precedence precedence);

static void grouping() {
  	expression();
  	consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary() {
	TokenType op_type = parser.prev.type;
	parse_precedence(PREC_UNARY);
	switch (op_type) {
		case TOKEN_BANG: emit_byte(OP_NOT); break;
		case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
		default: return;
	}
}

static void binary() {
	TokenType op_type = parser.prev.type;
	ParseRule* rule = get_rule(op_type);
	parse_precedence((Precedence) (rule->precedence + 1));
	switch (op_type) {
		case TOKEN_BANG_EQUAL: 
			emit_bytes(OP_EQUAL, OP_NOT); 
			break;
		case TOKEN_EQUAL_EQUAL: 
			emit_byte(OP_EQUAL); 
			break;
		case TOKEN_GREATER: 
			emit_byte(OP_GREATER); 
			break;
		case TOKEN_GREATER_EQUAL: 
			emit_bytes(OP_LESS, OP_NOT); 
			break;
		case TOKEN_LESS: 
			emit_byte(OP_LESS); 
			break;
		case TOKEN_LESS_EQUAL: 
			emit_bytes(OP_GREATER, OP_NOT); 
			break;
		case TOKEN_PLUS: 
			emit_byte(OP_ADD); 
			break;
		case TOKEN_MINUS: 
			emit_byte(OP_SUBTRACT); 
			break;
		case TOKEN_STAR: 
			emit_byte(OP_MULTIPLY); 
			break;
		case TOKEN_SLASH: 
			emit_byte(OP_DIVIDE); 
			break;
		default: 
			return;
	}
}

static void number() {
	double val = strtod(parser.prev.start, nullptr);
	emit_constant(NUMBER_VAL(val));
}

static void literal() {
	switch (parser.prev.type) {
		case TOKEN_FALSE: emit_byte(OP_FALSE); break;
		case TOKEN_NIL: emit_byte(OP_NIL); break;
		case TOKEN_TRUE: emit_byte(OP_TRUE); break;
		default: return;
	}
}

static void string() {
  	emit_constant(OBJ_VAL(copy_string(parser.prev.start + 1, parser.prev.len - 2)));
}

ParseRule rules[] = {
	[TOKEN_LEFT_PAREN]    = {grouping, nullptr, PREC_NONE},
	[TOKEN_RIGHT_PAREN]   = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_LEFT_BRACE]    = {nullptr,  nullptr, PREC_NONE}, 
	[TOKEN_RIGHT_BRACE]   = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_COMMA]         = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_DOT]           = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_MINUS]         = {unary,    binary,  PREC_TERM},
	[TOKEN_PLUS]          = {nullptr,  binary,  PREC_TERM},
	[TOKEN_SEMICOLON]     = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_SLASH]         = {nullptr,  binary,  PREC_FACTOR},
	[TOKEN_STAR]          = {nullptr,  binary,  PREC_FACTOR},
	[TOKEN_BANG]          = {unary,    nullptr, PREC_NONE},
	[TOKEN_BANG_EQUAL]    = {nullptr,  binary,  PREC_EQUALITY},
	[TOKEN_EQUAL]         = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_EQUAL_EQUAL]   = {nullptr,  binary,  PREC_EQUALITY},
	[TOKEN_GREATER]       = {nullptr,  binary,  PREC_COMPARISON},
	[TOKEN_GREATER_EQUAL] = {nullptr,  binary,  PREC_COMPARISON},
	[TOKEN_LESS]          = {nullptr,  binary,  PREC_COMPARISON},
	[TOKEN_LESS_EQUAL]    = {nullptr,  binary,  PREC_COMPARISON},
	[TOKEN_IDENTIFIER]    = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_STRING]        = {string,   nullptr, PREC_NONE},
	[TOKEN_NUMBER]        = {number,   nullptr, PREC_NONE},
	[TOKEN_AND]           = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_CLASS]         = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_ELSE]          = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_FALSE]         = {literal,  nullptr, PREC_NONE},
	[TOKEN_FOR]           = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_FN]            = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_IF]            = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_NIL]           = {literal,  nullptr, PREC_NONE},
	[TOKEN_OR]            = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_PRINT]         = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_RETURN]        = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_SUPER]         = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_THIS]          = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_TRUE]          = {literal,  nullptr, PREC_NONE},
	[TOKEN_LET]           = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_WHILE]         = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_ERROR]         = {nullptr,  nullptr, PREC_NONE},
	[TOKEN_EOF]           = {nullptr,  nullptr, PREC_NONE},
};

static void parse_precedence(Precedence precedence) {
	advance();
	ParseFn prefix_rule = get_rule(parser.prev.type)->prefix;
	if (prefix_rule == nullptr) {
		error("Expect expression.");
		return;
	}
	prefix_rule();
	while (precedence <= get_rule(parser.curr.type)->precedence) {
		advance();
		ParseFn infix_rule = get_rule(parser.prev.type)->infix;
		infix_rule();
	}
}

static ParseRule* get_rule(TokenType type) {
	return &rules[type];
}

static void expression() {
	parse_precedence(PREC_ASSIGNMENT);
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
	#ifdef DEBUG_PRINT_CODE
		if (!parser.had_error) disassemble_chunk(curr_chunk(), "code");
	#endif
	return !parser.had_error;
}