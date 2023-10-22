#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"


#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

Parser parser;
Chunk* compiling_chunk;

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
	[TOKEN_IDENTIFIER]    = {variable, nullptr, PREC_NONE},
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
	[TOKEN_STAR_STAR]     = {nullptr,  binary,  PREC_POW},
	[TOKEN_SLASH_SLASH]   = {nullptr,  binary,  PREC_FACTOR},

	[TOKEN_MINUS_EQUAL]         = {unary,    binary,  PREC_NONE},
	[TOKEN_PLUS_EQUAL]          = {nullptr,  binary,  PREC_NONE},
	[TOKEN_SLASH_EQUAL]         = {nullptr,  binary,  PREC_NONE},
	[TOKEN_STAR_EQUAL]          = {nullptr,  binary,  PREC_NONE},
	[TOKEN_STAR_STAR_EQUAL]     = {nullptr,  binary,  PREC_NONE},
	[TOKEN_SLASH_SLASH_EQUAL]   = {nullptr,  binary,  PREC_NONE},
};

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

static bool check(TokenType type) {
  	return parser.curr.type == type;
}

static bool match(TokenType type) {
	if (!check(type)) return false;
	advance();
	return true;
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

static void grouping(bool can_assign) {
  	expression();
  	consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary(bool can_assign) {
	TokenType op_type = parser.prev.type;
	parse_precedence(PREC_UNARY);
	switch (op_type) {
		case TOKEN_BANG: 
			emit_byte(OP_NOT); 
			break;
		case TOKEN_MINUS: 
			emit_byte(OP_NEGATE); 
			break;
		default: return;
	}
}

static void binary(bool can_assign) {
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
		case TOKEN_SLASH_SLASH:
			emit_byte(OP_INT_DIVIDE); 
			break;
		case TOKEN_STAR_STAR:
			emit_byte(OP_POW);
			break;
		// case TOKEN_PLUS_EQUAL: 
		// 	emit_byte(OP_ADD_SELF); 
		// 	break;
		// case TOKEN_MINUS_EQUAL: 
		// 	emit_byte(OP_SUBTRACT_SELF); 
		// 	break;
		// case TOKEN_STAR_EQUAL: 
		// 	emit_byte(OP_MULTIPLY_SELF); 
		// 	break;
		// case TOKEN_SLASH_EQUAL: 
		// 	emit_byte(OP_DIVIDE_SELF); 
		// 	break;
		// case TOKEN_SLASH_SLASH_EQUAL:
		// 	emit_byte(OP_INT_DIVIDE_SELF); 
		// 	break;
		// case TOKEN_STAR_STAR_EQUAL:
		// 	emit_byte(OP_POW_SELF);
		// 	break;
		default: 
			return;
	}
}

static void number(bool can_assign) {
	double val = strtod(parser.prev.start, nullptr);
	emit_constant(NUMBER_VAL(val));
}

static void literal(bool can_assign) {
	switch (parser.prev.type) {
		case TOKEN_FALSE: emit_byte(OP_FALSE); break;
		case TOKEN_NIL: emit_byte(OP_NIL); break;
		case TOKEN_TRUE: emit_byte(OP_TRUE); break;
		default: return;
	}
}

static void string(bool can_assign) {
  	emit_constant(OBJ_VAL(copy_string(parser.prev.start + 1, parser.prev.len - 2)));
}

static void named_variable(Token name, bool can_assign) {
	uint8_t arg = identifier_constant(&name);
	if (can_assign && match(TOKEN_EQUAL)) {
		expression();
		emit_bytes(OP_SET_GLOBAL, arg);
	} else if (can_assign && match(TOKEN_PLUS_EQUAL)) {
		expression();
		emit_bytes(OP_ADD_SELF, arg);
	} else if (can_assign && match(TOKEN_MINUS_EQUAL)) {
		expression();
		emit_bytes(OP_SUBTRACT_SELF, arg);
	} else if (can_assign && match(TOKEN_STAR_EQUAL)) {
		expression();
		emit_bytes(OP_MULTIPLY_SELF, arg);
	} else if (can_assign && match(TOKEN_SLASH_EQUAL)) {
		expression();
		emit_bytes(OP_DIVIDE_SELF, arg);
	} else if (can_assign && match(TOKEN_SLASH_SLASH_EQUAL)) {
		expression();
		emit_bytes(OP_INT_DIVIDE_SELF, arg);
	} else if (can_assign && match(TOKEN_STAR_STAR_EQUAL)) {
		expression();
		emit_bytes(OP_POW_SELF, arg);
	} else {
		emit_bytes(OP_GET_GLOBAL, arg);
	}
}

static void variable(bool can_asign) {
	named_variable(parser.prev, can_asign);
}

static void parse_precedence(Precedence precedence) {
	advance();
	ParseFn prefix_rule = get_rule(parser.prev.type)->prefix;
	if (prefix_rule == nullptr) {
		error("Expect expression.");
		return;
	}

	bool can_assign = precedence <= PREC_ASSIGNMENT;
	prefix_rule(can_assign);

	while (precedence <= get_rule(parser.curr.type)->precedence) {
		advance();
		ParseFn infix_rule = get_rule(parser.prev.type)->infix;
		infix_rule(can_assign);
	}

	if (can_assign && match(TOKEN_EQUAL)) error("Invalid assignment target.");
}

static ParseRule* get_rule(TokenType type) {
	return &rules[type];
}

static void expression() {
	parse_precedence(PREC_ASSIGNMENT);
}

static void synchronize() {
  	parser.panic_mode = false;
	while (parser.curr.type != TOKEN_EOF) {
		if (parser.prev.type == TOKEN_SEMICOLON) return;
		switch (parser.curr.type) {
			case TOKEN_CLASS:
			case TOKEN_FN:
			case TOKEN_LET:
			case TOKEN_FOR:
			case TOKEN_IF:
			case TOKEN_WHILE:
			case TOKEN_PRINT:
			case TOKEN_RETURN:
				return;
			default:
				;
		}
		advance();
	}
}

static void declaration() {
	if (match(TOKEN_LET)) let_declaration();
	else statement();
	
	if (parser.panic_mode) synchronize();
}

static void let_declaration() {
	uint8_t global = parse_variable("Expect variable name.");
	if (match(TOKEN_EQUAL)) expression();
	else emit_byte(OP_NIL);
	consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
	define_variable(global);
}

static uint8_t parse_variable(const char* err_msg) {
	consume(TOKEN_IDENTIFIER, err_msg);
	return identifier_constant(&parser.prev);
}

static uint8_t identifier_constant(Token* name) {
	return make_constant(OBJ_VAL(copy_string(name->start, name->len)));
}

static void define_variable(uint8_t global) {
	emit_bytes(OP_DEFINE_GLOBAL, global);
}

static void statement() {
	if (match(TOKEN_PRINT)) print_statement();
	else expression_statement();
}

static void print_statement() {
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after value.");
	emit_byte(OP_PRINT);
}

static void expression_statement() {
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
	emit_byte(OP_POP);
}


bool compile(const char* source, Chunk* chunk) {
  	init_scanner(source);
	compiling_chunk = chunk;
	parser.had_error = parser.panic_mode = false;

	int line = -1;
	advance();
	
	while (!match(TOKEN_EOF)) declaration();

	emit_return();
	#ifdef DEBUG_PRINT_CODE
		if (!parser.had_error) disassemble_chunk(curr_chunk(), "code");
	#endif
	return !parser.had_error;
}