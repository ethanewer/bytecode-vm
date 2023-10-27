#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"
#include "obj.h"
#include "compiler.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

Parser parser;
Compiler* curr = nullptr;
ParseRule rules[] = {
	[TOKEN_LEFT_PAREN]          = {grouping, call,        PREC_CALL},
	[TOKEN_RIGHT_PAREN]         = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_LEFT_BRACE]          = {nullptr,  nullptr,     PREC_NONE}, 
	[TOKEN_RIGHT_BRACE]         = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_COMMA]               = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_DOT]                 = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_MINUS]               = {unary,    binary,      PREC_TERM},
	[TOKEN_PLUS]                = {nullptr,  binary,      PREC_TERM},
	[TOKEN_SEMICOLON]           = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_SLASH]               = {nullptr,  binary,      PREC_FACTOR},
	[TOKEN_STAR]                = {nullptr,  binary,      PREC_FACTOR},
	[TOKEN_BANG]                = {unary,    nullptr,     PREC_NONE},
	[TOKEN_BANG_EQUAL]          = {nullptr,  binary,      PREC_EQUALITY},
	[TOKEN_EQUAL]               = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_EQUAL_EQUAL]         = {nullptr,  binary,      PREC_EQUALITY},
	[TOKEN_GREATER]             = {nullptr,  binary,      PREC_COMPARISON},
	[TOKEN_GREATER_EQUAL]       = {nullptr,  binary,      PREC_COMPARISON},
	[TOKEN_LESS]                = {nullptr,  binary,      PREC_COMPARISON},
	[TOKEN_LESS_EQUAL]          = {nullptr,  binary,      PREC_COMPARISON},
	[TOKEN_IDENTIFIER]          = {variable, nullptr,     PREC_NONE},
	[TOKEN_STRING]              = {string,   nullptr,     PREC_NONE},
	[TOKEN_NUMBER]              = {number,   nullptr,     PREC_NONE},
	[TOKEN_AND]                 = {nullptr,  logical_and, PREC_NONE},
	[TOKEN_CLASS]               = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_ELSE]                = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_FALSE]               = {literal,  nullptr,     PREC_NONE},
	[TOKEN_FOR]                 = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_FN]                  = {lambda,  nullptr,     PREC_NONE},
	[TOKEN_IF]                  = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_NIL]                 = {literal,  nullptr,     PREC_NONE},
	[TOKEN_OR]                  = {nullptr,  logical_or,  PREC_NONE},
	[TOKEN_PRINT]               = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_RETURN]              = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_SUPER]               = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_THIS]                = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_TRUE]                = {literal,  nullptr,     PREC_NONE},
	[TOKEN_LET]                 = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_WHILE]               = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_ERROR]               = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_EOF]                 = {nullptr,  nullptr,     PREC_NONE},
	[TOKEN_STAR_STAR]           = {nullptr,  binary,      PREC_POW},
	[TOKEN_SLASH_SLASH]         = {nullptr,  binary,      PREC_FACTOR},
	[TOKEN_MINUS_EQUAL]         = {unary,    binary,      PREC_NONE},
	[TOKEN_PLUS_EQUAL]          = {nullptr,  binary,      PREC_NONE},
	[TOKEN_SLASH_EQUAL]         = {nullptr,  binary,      PREC_NONE},
	[TOKEN_STAR_EQUAL]          = {nullptr,  binary,      PREC_NONE},
	[TOKEN_STAR_STAR_EQUAL]     = {nullptr,  binary,      PREC_NONE},
	[TOKEN_SLASH_SLASH_EQUAL]   = {nullptr,  binary,      PREC_NONE},
};

ObjFn* compile(const char* source) {
  	init_scanner(source);
	Compiler compiler;
	init_compiler(&compiler, TYPE_SCRIPT);
	parser.had_error = parser.panic_mode = false;

	advance();
	
	while (!match(TOKEN_EOF)) declaration();

	ObjFn* fn = end_compiler();
	return parser.had_error ? nullptr : fn;
}

static void init_compiler(Compiler* compiler, FnType type) {
	compiler->enclosing = curr;
	compiler->fn = nullptr;
	compiler->type = type;
	compiler->locals_len = 0;
	compiler->scope_depth = 0;
	compiler->fn = new_fn();
	curr = compiler;
	if (type != TYPE_SCRIPT) curr->fn->name = copy_string(parser.prev.start, parser.prev.len);

	switch (type) {
		case TYPE_FN:
			curr->fn->name = copy_string(parser.prev.start, parser.prev.len);
			break;
		case TYPE_LAMBDA:
			curr->fn->name = copy_string("lambda", 6);
			break;
		case TYPE_SCRIPT:
			break;
	}

	Local* local = &curr->locals[curr->locals_len++];
	local->depth = 0;
	local->is_captured = false;
	local->name.start = "";
	local->name.len = 0;
}

static Chunk* curr_chunk() {
	return &curr->fn->chunk;
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

static int emit_jump(uint8_t instruction) {
	emit_byte(instruction);
	emit_byte(0xff);
	emit_byte(0xff);
	return curr_chunk()->len - 2;
}

static void emit_loop(int loop_start) {
	emit_byte(OP_LOOP);
	int offset = curr_chunk()->len - loop_start + 2;
	if (offset > UINT16_MAX) error("Loop body too large.");
	emit_byte((offset >> 8) & 0xff);
	emit_byte(offset & 0xff);
}

static void emit_return() {
	emit_byte(OP_NIL);
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

static void patch_jump(int offset) {
	int jump = curr_chunk()->len - offset - 2;
	if (jump > UINT16_MAX) error("Too much code to jump over.");
	curr_chunk()->code[offset] = (jump >> 8) & 0xff;
	curr_chunk()->code[offset + 1] = jump & 0xff;
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
		default: 
			return;
	}
}

static void call(bool can_assign) {
	uint8_t num_args = argument_list();
	emit_bytes(OP_CALL, num_args);
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
	int arg = resolve_local(curr, &name);
	if (arg != -1) named_variable_local((uint8_t) arg, can_assign);
	else if ((arg = resolve_upvalue(curr, &name)) != -1) named_variable_upvalue((uint8_t) arg, can_assign);
	else named_variable_global(identifier_constant(&name), can_assign);
}


static void named_variable_local(uint8_t arg, bool can_assign) {
	if (can_assign && match(TOKEN_EQUAL)) {
		expression();
		emit_bytes(OP_SET_LOCAL, arg);
	} else if (can_assign && match(TOKEN_PLUS_EQUAL)) {
		expression();
		emit_bytes(OP_ADD_SELF_LOCAL, arg);
	} else if (can_assign && match(TOKEN_MINUS_EQUAL)) {
		expression();
		emit_bytes(OP_SUBTRACT_SELF_LOCAL, arg);
	} else if (can_assign && match(TOKEN_STAR_EQUAL)) {
		expression();
		emit_bytes(OP_MULTIPLY_SELF_LOCAL, arg);
	} else if (can_assign && match(TOKEN_SLASH_EQUAL)) {
		expression();
		emit_bytes(OP_DIVIDE_SELF_LOCAL, arg);
	} else if (can_assign && match(TOKEN_SLASH_SLASH_EQUAL)) {
		expression();
		emit_bytes(OP_INT_DIVIDE_SELF_LOCAL, arg);
	} else if (can_assign && match(TOKEN_STAR_STAR_EQUAL)) {
		expression();
		emit_bytes(OP_POW_SELF_LOCAL, arg);
	} else {
		emit_bytes(OP_GET_LOCAL, arg);
	}
}

static void named_variable_global(uint8_t arg, bool can_assign) {
	if (can_assign && match(TOKEN_EQUAL)) {
		expression();
		emit_bytes(OP_SET_GLOBAL, arg);
	} else if (can_assign && match(TOKEN_PLUS_EQUAL)) {
		expression();
		emit_bytes(OP_ADD_SELF_GLOBAL, arg);
	} else if (can_assign && match(TOKEN_MINUS_EQUAL)) {
		expression();
		emit_bytes(OP_SUBTRACT_SELF_GLOBAL, arg);
	} else if (can_assign && match(TOKEN_STAR_EQUAL)) {
		expression();
		emit_bytes(OP_MULTIPLY_SELF_GLOBAL, arg);
	} else if (can_assign && match(TOKEN_SLASH_EQUAL)) {
		expression();
		emit_bytes(OP_DIVIDE_SELF_GLOBAL, arg);
	} else if (can_assign && match(TOKEN_SLASH_SLASH_EQUAL)) {
		expression();
		emit_bytes(OP_INT_DIVIDE_SELF_GLOBAL, arg);
	} else if (can_assign && match(TOKEN_STAR_STAR_EQUAL)) {
		expression();
		emit_bytes(OP_POW_SELF_GLOBAL, arg);
	} else {
		emit_bytes(OP_GET_GLOBAL, arg);
	}
}

static void named_variable_upvalue(uint8_t arg, bool can_assign) {
	if (can_assign && match(TOKEN_EQUAL)) {
		expression();
		emit_bytes(OP_SET_UPVALUE, arg);
	} else if (can_assign && match(TOKEN_PLUS_EQUAL)) {
		expression();
		emit_bytes(OP_ADD_SELF_UPVALUE, arg);
	} else if (can_assign && match(TOKEN_MINUS_EQUAL)) {
		expression();
		emit_bytes(OP_SUBTRACT_SELF_UPVALUE, arg);
	} else if (can_assign && match(TOKEN_STAR_EQUAL)) {
		expression();
		emit_bytes(OP_MULTIPLY_SELF_UPVALUE, arg);
	} else if (can_assign && match(TOKEN_SLASH_EQUAL)) {
		expression();
		emit_bytes(OP_DIVIDE_SELF_UPVALUE, arg);
	} else if (can_assign && match(TOKEN_SLASH_SLASH_EQUAL)) {
		expression();
		emit_bytes(OP_INT_DIVIDE_SELF_UPVALUE, arg);
	} else if (can_assign && match(TOKEN_STAR_STAR_EQUAL)) {
		expression();
		emit_bytes(OP_POW_SELF_UPVALUE, arg);
	} else {
		emit_bytes(OP_GET_UPVALUE, arg);
	}
}

static void variable(bool can_asign) {
	named_variable(parser.prev, can_asign);
}

static void lambda(bool can_assign) {
	Compiler compiler;
	init_compiler(&compiler, TYPE_LAMBDA);
	begin_scope(); 

	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'fn' when declaring a lambda expression.");
	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			curr->fn->num_params++;
			if (curr->fn->num_params > 255) error_at_curr("Can't have more than 255 parameters.");
			uint8_t constant = parse_variable("Expect parameter name.");
			define_variable(constant);
		} while (match(TOKEN_COMMA));
	}
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
	consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
	block();

	ObjFn* fn = end_compiler();
	emit_bytes(OP_CLOSURE, make_constant(OBJ_VAL(fn)));

	for (int i = 0; i < fn->num_upvalues; i++) {
		emit_byte(compiler.upvalues[i].is_local);
		emit_byte(compiler.upvalues[i].idx);
	}
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

static void block() {
	while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) declaration();
	consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void fn(FnType type) {
	Compiler compiler;
	init_compiler(&compiler, type);
	begin_scope(); 

	consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			curr->fn->num_params++;
			if (curr->fn->num_params > 255) error_at_curr("Can't have more than 255 parameters.");
			uint8_t constant = parse_variable("Expect parameter name.");
			define_variable(constant);
		} while (match(TOKEN_COMMA));
	}
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
	consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
	block();

	ObjFn* fn = end_compiler();
	emit_bytes(OP_CLOSURE, make_constant(OBJ_VAL(fn)));

	for (int i = 0; i < fn->num_upvalues; i++) {
		emit_byte(compiler.upvalues[i].is_local);
		emit_byte(compiler.upvalues[i].idx);
	}
}

static void begin_scope() {
	curr->scope_depth++;
}

static void end_scope() {
	curr->scope_depth--;
	while (curr->locals_len > 0 && curr->locals[curr->locals_len - 1].depth > curr->scope_depth) {
		emit_byte(OP_POP);
		if (curr->locals[curr->locals_len - 1].is_captured) emit_byte(OP_CLOSE_UPVALUE);
		else emit_byte(OP_POP);
		curr->locals_len--;
	}
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
	if (match(TOKEN_FN)) fn_declaration();
	else if (match(TOKEN_LET)) let_declaration();
	else statement();
	
	if (parser.panic_mode) synchronize();
}

static void fn_declaration() {
	uint8_t global = parse_variable("Expect variable name.");
	mark_initialized();
	fn(TYPE_FN);
	define_variable(global);
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
	declare_variable();
	if (curr->scope_depth > 0) return 0;
	return identifier_constant(&parser.prev);
}

static void mark_initialized() {
	if (curr->scope_depth == 0) return;
  	curr->locals[curr->locals_len - 1].depth = curr->scope_depth;
}

static uint8_t identifier_constant(Token* name) {
	return make_constant(OBJ_VAL(copy_string(name->start, name->len)));
}

static void declare_variable() {
	if (curr->scope_depth == 0) return;
	Token* name = &parser.prev;
	for (int i = curr->locals_len - 1; i >= 0; i--) {
		Local* local = &curr->locals[i];
		if (local->depth != -1 && local->depth < curr->scope_depth) break;
		if (identifiers_equal(name, &local->name)) {
			error("Already a variable with this name in this scope.");
		}
	}
	add_local(*name);
}

static void add_local(Token name) {
	if (curr->locals_len > UINT8_MAX) {
		error("Too many local variables.");
		return;
	}
	Local* local = &curr->locals[curr->locals_len++];
	local->name = name;
	local->depth = -1;
	local->is_captured = false;
}

static bool identifiers_equal(Token* a, Token* b) {
	if (a->len != b->len) return false;
	return memcmp(a->start, b->start, a->len) == 0;
}

static int resolve_local(Compiler* compiler, Token* name) {
	for (int i = compiler->locals_len - 1; i >= 0; i--) {
		Local* local = &compiler->locals[i];
		if (identifiers_equal(name, &local->name)) {
			if (local->depth == -1) error("Can't read local variable in its own initializer.");
			return i;
		}
	}
	return -1;
}

static int add_upvalue(Compiler* compiler, uint8_t idx, bool is_local) {
	int num_upvalues = compiler->fn->num_upvalues;
	for (int i = 0; i < num_upvalues; i++) {
		Upvalue* upvalue = &compiler->upvalues[i];
		if (upvalue->idx == idx && upvalue->is_local == is_local) return i;
	}
	if (num_upvalues > UINT8_MAX) {
		error("Too many closure variables in function.");
		return 0;
	}
	compiler->upvalues[num_upvalues].is_local = is_local;
	compiler->upvalues[num_upvalues].idx = idx;
	return compiler->fn->num_upvalues++;
}

static int resolve_upvalue(Compiler* compiler, Token* name) {
	if (compiler->enclosing == NULL) return -1;

	int local = resolve_local(compiler->enclosing, name);
	if (local != -1) {
		compiler->enclosing->locals[local].is_captured = true;
		return add_upvalue(compiler, (uint8_t) local, true);
	}

	int upvalue = resolve_upvalue(compiler->enclosing, name);
	if (upvalue != -1) return add_upvalue(compiler, (uint8_t) upvalue, false);

	return -1;
}

static void define_variable(uint8_t global) {
	if (curr->scope_depth > 0) {
		curr->locals[curr->locals_len - 1].depth = curr->scope_depth;
		return;
	}
	emit_bytes(OP_DEFINE_GLOBAL, global);
}

static uint8_t argument_list() {
	uint8_t num_args = 0;
	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			expression();
			if (num_args == 255) error("Can't have more than 255 arguments.");
			num_args++;
		} while (match(TOKEN_COMMA));
	}
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
	return num_args;
}

static void logical_and(bool can_assign) {
	int end_jump = emit_jump(OP_JUMP_IF_FALSE);
	emit_byte(OP_POP);
	parse_precedence(PREC_AND);
	patch_jump(end_jump);
}

static void logical_or(bool can_assign) {
	int else_jump = emit_jump(OP_JUMP_IF_FALSE);
	int end_jump = emit_jump(OP_JUMP);
	patch_jump(else_jump);
	emit_byte(OP_POP);
	parse_precedence(PREC_OR);
	patch_jump(end_jump);
}

static void statement() {
	if (match(TOKEN_PRINT)) {
		print_statement();
	} else if (match(TOKEN_IF)) {
		if_statement();
	} else if (match(TOKEN_WHILE)) {
		while_statement();
	} else if (match(TOKEN_FOR)) {
		for_statement();
	} else if (match(TOKEN_LEFT_BRACE)) {
		begin_scope();
		block();
		end_scope();
	} else if (match(TOKEN_RETURN)) {
		return_statement();
	} else {
		expression_statement();
	}
}

static void print_statement() {
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after value.");
	emit_byte(OP_PRINT);
}

static void if_statement() {
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
	int then_jump = emit_jump(OP_JUMP_IF_FALSE);
	emit_byte(OP_POP);
	statement();
	int else_jump = emit_jump(OP_JUMP);
	patch_jump(then_jump);
	emit_byte(OP_POP);
	if (match(TOKEN_ELSE)) statement();
	patch_jump(else_jump);
}

static void while_statement() {
	int loop_start = curr_chunk()->len;
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
	int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
	emit_byte(OP_POP);
	statement();
	emit_loop(loop_start);
	patch_jump(exit_jump);
	emit_byte(OP_POP);
}

static void for_statement() {
	begin_scope();
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

	if (match(TOKEN_SEMICOLON)) {
		// No initializer.
	} else if (match(TOKEN_LET)) {
		let_declaration();
	} else {
		expression_statement();
	}

	int loop_start = curr_chunk()->len;
	int exit_jump = -1;
	if (!match(TOKEN_SEMICOLON)) {
		expression();
		consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
		exit_jump = emit_jump(OP_JUMP_IF_FALSE);
		emit_byte(OP_POP);
	}

	if (!match(TOKEN_RIGHT_PAREN)) {
		int body_jump = emit_jump(OP_JUMP);
		int increment_start = curr_chunk()->len;
		expression();
		emit_byte(OP_POP);
		consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");
		emit_loop(loop_start);
		loop_start = increment_start;
		patch_jump(body_jump);
	}

	statement();
	emit_loop(loop_start);

	if (exit_jump != -1) {
		patch_jump(exit_jump);
		emit_byte(OP_POP);
	}
	end_scope();
}

static void return_statement() {
	if (curr->type == TYPE_SCRIPT) error("Can't return from top-level code.");
	if (match(TOKEN_SEMICOLON)) {
		emit_return();
	} else {
		expression();
		consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
		emit_byte(OP_RETURN);
	}
}



static void expression_statement() {
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
	emit_byte(OP_POP);
}

static ObjFn* end_compiler() {
	emit_return();
	ObjFn* fn = curr->fn;
	
	#ifdef DEBUG_PRINT_CODE
		if (!parser.had_error) {
			disassemble_chunk(curr_chunk(), fn->name != nullptr ? fn->name->chars : "main-script");
		}
	#endif
	
	curr = curr->enclosing;
	return fn;
}