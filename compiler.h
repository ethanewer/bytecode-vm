#ifndef compiler_h
#define compiler_h

#include "vm.h"
#include "common.h"
#include "scanner.h"
#include "obj.h"

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
	PREC_POW, // **
	PREC_UNARY, // ! -
	PREC_CALL, // . ()
	PREC_PRIMARY
};

typedef void (*ParseFn)(bool can_assign);

struct ParseRule {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
};

struct Local {
  Token name;
  int depth;
};

struct Compiler {
	Local locals[UINT8_MAX + 1];
	int locals_len;
	int scope_depth;
};

static void init_compiler(Compiler* compiler);
static void error_at(Token* token, const char* msg);
static void error_at_curr(const char* msg);
static void error(const char* msg);
static void advance();
static void consume(TokenType type, const char* msg);
static bool check(TokenType type);
static bool match(TokenType type);
static void emit_byte(uint8_t byte);
static void emit_bytes(uint8_t byte1, uint8_t byte2);
static void emit_return();
static uint8_t make_constant(Val val);
static void emit_constant(Val val);
static void grouping(bool can_assign);
static void unary(bool can_assign);
static void binary(bool can_assign);
static void number(bool can_assign);
static void literal(bool can_assign);
static void string(bool can_assign);
static void named_variable(Token name, bool can_assign);
static void variable(bool can_assign);
static void parse_precedence(Precedence precedence);
static ParseRule* get_rule(TokenType type);
static void expression();
static void block();
static void begin_scope();
static void end_scope();
static void synchronize();
static void declaration();
static void let_declaration();
static uint8_t parse_variable(const char* err_msg);
static uint8_t identifier_constant(Token* name);
static void declare_variable();
static void add_local(Token name);
static bool identifiers_equal(Token* a, Token* b);
static int resolve_local(Compiler* compiler, Token* name);
static void define_variable(uint8_t global);
static void statement();
static void print_statement();
static void expression_statement();
bool compile(const char* source, Chunk* chunk);

#endif