#ifndef compiler_h
#define compiler_h

#include "common.h"
#include "scanner.h"
#include "obj.h"

using ParseFn = void (*)(bool can_assign);

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

struct ParseRule {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
};

struct Parser {
	Token curr;
	Token prev;
	bool had_error;
	bool panic_mode;
};

enum FnType {
	TYPE_FN,
	TYPE_LAMBDA,
	TYPE_SCRIPT
};

struct Local {
	Token name;
	int depth;
	bool is_captured;
};

struct Upvalue {
	bool is_local;
	uint8_t idx;
};

struct Compiler {
	Compiler* enclosing;
	ObjFn* fn;
	FnType type;
	Upvalue upvalues[UINT8_MAX + 1];
	Local locals[UINT8_MAX + 1];
	int locals_len;
	int scope_depth;
};

ObjFn* compile(const char* source);
void mark_compiler_roots();
static void init_compiler(Compiler* compiler, FnType type);
static Chunk* curr_chunk();
static void error_at(Token* token, const char* msg);
static void error_at_curr(const char* msg);
static void error(const char* msg);
static void advance();
static void consume(TokenType type, const char* msg);
static bool check(TokenType type);
static bool match(TokenType type);
static void emit_byte(uint8_t byte);
static void emit_bytes(uint8_t byte1, uint8_t byte2);
static int emit_jump(uint8_t instruction);
static void emit_loop(int loop_start);
static void emit_return();
static uint8_t make_constant(Val val);
static void emit_constant(Val val);
static void patch_jump(int offset);
static void grouping(bool can_assign);
static void unary(bool can_assign);
static void binary(bool can_assign);
static void call(bool can_assign);
static void number(bool can_assign);
static void literal(bool can_assign);
static void string(bool can_assign);
static void named_variable(Token name, bool can_assign);
static void named_variable_local(uint8_t arg, bool can_assign);
static void named_variable_global(uint8_t arg, bool can_assign);
static void named_variable_upvalue(uint8_t arg, bool can_assign);
static void variable(bool can_assign);
static void lambda(bool can_assign);
static void parse_precedence(Precedence precedence);
static ParseRule* get_rule(TokenType type);
static void expression();
static void block();
static void fn(FnType type);
static void begin_scope();
static void end_scope();
static void synchronize();
static void declaration();
static void fn_declaration();
static void let_declaration();
static uint8_t parse_variable(const char* err_msg);
static void mark_initialized();
static uint8_t identifier_constant(Token* name);
static void declare_variable();
static void add_local(Token name);
static bool identifiers_equal(Token* a, Token* b);
static int resolve_local(Compiler* compiler, Token* name);
static int add_upvalue(Compiler* compiler, uint8_t idx, bool is_local);
static int resolve_upvalue(Compiler* compiler, Token* name);
static void define_variable(uint8_t global);
static uint8_t argument_list();
static void logical_and(bool can_assign);
static void logical_or(bool can_assign);
static void statement();
static void print_statement();
static void if_statement();
static void while_statement();
static void for_statement();
static void return_statement();
static void expression_statement();
static ObjFn* end_compiler();

#endif