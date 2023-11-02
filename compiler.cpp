#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.hpp"
#include "compiler.hpp"
#include "memory.hpp"
#include "scanner.hpp"

#ifdef DEBUG_PRINT_CODE
#include "debug.hpp"
#endif

struct Parser {
  Token curr;
  Token prev;
  bool had_error;
  bool panic_mode;
};

enum Precedence {
  PREC_NONE,
  PREC_ASSIGNMENT,  
  PREC_OR,          
  PREC_AND,         
  PREC_EQUALITY,    
  PREC_COMPARISON,  
  PREC_TERM,        
  PREC_FACTOR,    
  PREC_POW,  
  PREC_UNARY,       
  PREC_CALL,        
  PREC_PRIMARY
};

using ParseFn = void(*)(bool can_assign);

struct ParseRule {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
};

struct Local {
  Token name;
  int depth;
  bool is_captured;
};

struct Upvalue {
  uint8_t index;
  bool is_local;
};

enum FunctionType {
  TYPE_FUNCTION,
  TYPE_LAMBDA,
  TYPE_INITIALIZER,
  TYPE_METHOD,
  TYPE_SCRIPT
};

struct Compiler {
  Compiler* enclosing;
  ObjFunction* function;
  FunctionType type;
  Local locals[UINT8_COUNT];
  int local_count;
  Upvalue upvalues[UINT8_COUNT];
  int scope_depth;
};

struct ClassCompiler {
  Token name;
  ClassCompiler* enclosing;
  bool has_superclass;
};

Parser parser;
Compiler* curr = nullptr;
ClassCompiler* curr_class = nullptr;

static Chunk* curr_chunk() {
  return &curr->function->chunk;
}

static void error_at(Token* token, const char* message) {
  if (parser.panic_mode) return;
  parser.panic_mode = true;
  fprintf(stderr, "[line %d] Error", token->line);
  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // nothing
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }
  fprintf(stderr, ": %s\n", message);
  parser.had_error = true;
}

static void error(const char* message) {
  error_at(&parser.prev, message);
}

static void error_at_current(const char* message) {
  error_at(&parser.curr, message);
}

static void advance() {
  parser.prev = parser.curr;
  for (;;) {
    parser.curr = scan_token();
    if (parser.curr.type != TOKEN_ERROR) break;
    error_at_current(parser.curr.start);
  }
}

static void consume(TokenType type, const char* message) {
  if (parser.curr.type == type) {
    advance();
    return;
  }
  error_at_current(message);
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
  curr_chunk()->write(byte, parser.prev.line);
}

static void emit_bytes(uint8_t byte1, uint8_t byte2) {
  emit_byte(byte1);
  emit_byte(byte2);
}

static void emit_loop(int loop_start) {
  emit_byte(OP_LOOP);

  int offset = curr_chunk()->count - loop_start + 2;
  if (offset > UINT16_MAX) error("Loop body too large.");

  emit_byte((offset >> 8) & 0xff);
  emit_byte(offset & 0xff);
}

static int emit_jump(uint8_t instruction) {
  emit_byte(instruction);
  emit_byte(0xff);
  emit_byte(0xff);
  return curr_chunk()->count - 2;
}

static void emit_return() {
  if (curr->type == TYPE_INITIALIZER) {
    emit_bytes(OP_GET_LOCAL, 0);
  } else {
    emit_byte(OP_NIL);
  }
  emit_byte(OP_RETURN);
}

static uint8_t make_constant(Value value) {
  int constant = curr_chunk()->add_constant(value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }
  return static_cast<uint8_t>(constant);
}

static void emit_constant(Value value) {
  emit_bytes(OP_CONSTANT, make_constant(value));
}

static void patch_jump(int offset) {
  int jump = curr_chunk()->count - offset - 2;
  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }
  curr_chunk()->code[offset] = (jump >> 8) & 0xff;
  curr_chunk()->code[offset + 1] = jump & 0xff;
}

static void init_compiler(Compiler* compiler, FunctionType type) {
  compiler->enclosing = curr;
  compiler->function = nullptr;
  compiler->type = type;
  compiler->local_count = 0;
  compiler->scope_depth = 0;
  compiler->function = new ObjFunction();
  curr = compiler;
  if (type == TYPE_LAMBDA) {
    curr->function->name = copy_string("lambda", 6);
  } else if (type != TYPE_SCRIPT) {
    curr->function->name = copy_string(parser.prev.start, parser.prev.length);
  }
  Local* local = &curr->locals[curr->local_count++];
  local->depth = 0;
  local->is_captured = false;
  if (type != TYPE_FUNCTION) {
    local->name.start = "this";
    local->name.length = 4;
  } else {
    local->name.start = "";
    local->name.length = 0;
  }
}

static ObjFunction* end_compiler() {
  emit_return();
  ObjFunction* function = curr->function;

#ifdef DEBUG_PRINT_CODE
  if (!parser.had_error) {
    disassemble_chunk(curr_chunk(), function->name != nullptr ? function->name->chars : "<script>");
  }
#endif

  curr = curr->enclosing;
  return function;
}

static void begin_scope() {
  curr->scope_depth++;
}

static void end_scope() {
  curr->scope_depth--;
  while (curr->local_count > 0 && curr->locals[curr->local_count - 1].depth > curr->scope_depth) {
    if (curr->locals[curr->local_count - 1].is_captured) {
      emit_byte(OP_CLOSE_UPVALUE);
    } else {
      emit_byte(OP_POP);
    }
    curr->local_count--;
  }
}

static void expression();
static void statement();
static void declaration();
static ParseRule* get_rule(TokenType type);
static void parse_precedence(Precedence precedence);

static uint8_t identifier_constant(Token* name) {
  return make_constant(OBJ_VAL(copy_string(name->start, name->length)));
}

static bool identifiers_equal(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

static int resolve_local(Compiler* compiler, Token* name) {
  for (int i = compiler->local_count - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiers_equal(name, &local->name)) {
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }
  return -1;
}

static int add_upvalue(Compiler* compiler, uint8_t index, bool is_local) {
  int upvalue_count = compiler->function->upvalue_count;
  for (int i = 0; i < upvalue_count; i++) {
    Upvalue* upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->is_local == is_local) {
      return i;
    }
  }
  if (upvalue_count == UINT8_COUNT) {
    error("Too many closure variables in function.");
    return 0;
  }
  compiler->upvalues[upvalue_count].is_local = is_local;
  compiler->upvalues[upvalue_count].index = index;
  return compiler->function->upvalue_count++;
}

static int resolve_upvalue(Compiler* compiler, Token* name) {
  if (compiler->enclosing == nullptr) return -1;
  int local = resolve_local(compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->locals[local].is_captured = true;
    return add_upvalue(compiler, static_cast<uint8_t>(local), true);
  }
  int upvalue = resolve_upvalue(compiler->enclosing, name);
  if (upvalue != -1) {
    return add_upvalue(compiler, static_cast<uint8_t>(upvalue), false);
  }

  return -1;
}

static void add_local(Token name) {
  if (curr->local_count == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }
  Local* local = &curr->locals[curr->local_count++];
  local->name = name;
  local->depth = -1;
  local->is_captured = false;
}

static void declare_variable() {
  if (curr->scope_depth == 0) return;
  Token* name = &parser.prev;
  for (int i = curr->local_count - 1; i >= 0; i--) {
    Local* local = &curr->locals[i];
    if (local->depth != -1 && local->depth < curr->scope_depth) {
      break; 
    }
    if (identifiers_equal(name, &local->name)) {
      error("Already a variable with this name in this scope.");
    }
  }
  add_local(*name);
}

static uint8_t parse_variable(const char* error_message) {
  consume(TOKEN_IDENTIFIER, error_message);
  declare_variable();
  if (curr->scope_depth > 0) return 0;
  return identifier_constant(&parser.prev);
}

static void mark_initialized() {
  if (curr->scope_depth == 0) return;
  curr->locals[curr->local_count - 1].depth = curr->scope_depth;
}

static void define_variable(uint8_t global) {
  if (curr->scope_depth > 0) mark_initialized();
  else emit_bytes(OP_DEFINE_GLOBAL, global);
}

static uint8_t argument_list() {
  uint8_t arg_count = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();
      if (arg_count == 255) {
        error("Can't have more than 255 arguments.");
      }
      arg_count++;
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
  return arg_count;
}

static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void and_(bool can_assign) {
  int end_jump = emit_jump(OP_JUMP_IF_FALSE);

  emit_byte(OP_POP);
  parse_precedence(PREC_AND);

  patch_jump(end_jump);
}

static void binary(bool can_assign) {
  TokenType operator_type = parser.prev.type;
  ParseRule* rule = get_rule(operator_type);
  parse_precedence(static_cast<Precedence>(rule->precedence + 1));

  switch (operator_type) {
    case TOKEN_BANG_EQUAL:    emit_bytes(OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL:   emit_byte(OP_EQUAL); break;
    case TOKEN_GREATER:       emit_byte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emit_bytes(OP_LESS, OP_NOT); break;
    case TOKEN_LESS:          emit_byte(OP_LESS); break;
    case TOKEN_LESS_EQUAL:    emit_bytes(OP_GREATER, OP_NOT); break;

    case TOKEN_PLUS:          emit_byte(OP_ADD); break;
    case TOKEN_MINUS:         emit_byte(OP_SUBTRACT); break;
    case TOKEN_STAR:          emit_byte(OP_MULTIPLY); break;
    case TOKEN_SLASH:         emit_byte(OP_DIVIDE); break;
    case TOKEN_STAR_STAR:     emit_byte(OP_POW); break;
    case TOKEN_SLASH_SLASH:   emit_byte(OP_INT_DIVIDE); break;
    default: return; 
  }
}

static void call(bool can_assign) {
  uint8_t arg_count = argument_list();
  emit_bytes(OP_CALL, arg_count);
}

static void dot(bool can_assign) {
  consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
  uint8_t name = identifier_constant(&parser.prev);
  
  if (can_assign && match(TOKEN_EQUAL)) {
    expression();
    emit_bytes(OP_SET_PROPERTY, name);
  } else if (match(TOKEN_LEFT_PAREN)) {
    uint8_t arg_count = argument_list();
    emit_bytes(OP_INVOKE, name);
    emit_byte(arg_count);
  } else {
    emit_bytes(OP_GET_PROPERTY, name);
  }
}

static void literal(bool can_assign) {
  switch (parser.prev.type) {
    case TOKEN_FALSE: emit_byte(OP_FALSE); break;
    case TOKEN_NIL: emit_byte(OP_NIL); break;
    case TOKEN_TRUE: emit_byte(OP_TRUE); break;
    default: return; 
  }
}

static void grouping(bool can_assign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool can_assign) {
  double value = strtod(parser.prev.start, nullptr);
  emit_constant(NUMBER_VAL(value));

}

static void or_(bool can_assign) {
  int else_jump = emit_jump(OP_JUMP_IF_FALSE);
  int end_jump = emit_jump(OP_JUMP);
  patch_jump(else_jump);
  emit_byte(OP_POP);
  parse_precedence(PREC_OR);
  patch_jump(end_jump);
}

static void string(bool can_assign) {
  emit_constant(OBJ_VAL(copy_string(parser.prev.start + 1, parser.prev.length - 2)));
}

static void named_variable(Token name, bool can_assign) {
  uint8_t get_op, set_op;
  int arg = resolve_local(curr, &name);
  if (arg != -1) {
    get_op = OP_GET_LOCAL;
    set_op = OP_SET_LOCAL;
  } else if ((arg = resolve_upvalue(curr, &name)) != -1) {
    get_op = OP_GET_UPVALUE;
    set_op = OP_SET_UPVALUE;
  } else {
    arg = identifier_constant(&name);
    get_op = OP_GET_GLOBAL;
    set_op = OP_SET_GLOBAL;
  }
  
  if (!can_assign) {
    emit_bytes(get_op, static_cast<uint8_t>(arg));
    return;
  }
  
  switch (parser.curr.type) {
    case TOKEN_EQUAL:
      advance();
      expression();
      emit_bytes(set_op, static_cast<uint8_t>(arg));
      break;
    case TOKEN_PLUS_EQUAL:
      advance();
      emit_bytes(get_op, static_cast<uint8_t>(arg));
      parse_precedence(PREC_TERM);
      emit_byte(OP_ADD);
      emit_bytes(set_op, static_cast<uint8_t>(arg));
      break;
    case TOKEN_MINUS_EQUAL:
      advance();
      emit_bytes(get_op, static_cast<uint8_t>(arg));
      parse_precedence(PREC_TERM);
      emit_byte(OP_SUBTRACT);
      emit_bytes(set_op, static_cast<uint8_t>(arg));
      break;
    case TOKEN_STAR_EQUAL:
      advance();
      emit_bytes(get_op, static_cast<uint8_t>(arg));
      parse_precedence(PREC_FACTOR);
      emit_byte(OP_MULTIPLY);
      emit_bytes(set_op, static_cast<uint8_t>(arg));
      break;
    case TOKEN_SLASH_EQUAL:
      advance();
      emit_bytes(get_op, static_cast<uint8_t>(arg));
      parse_precedence(PREC_FACTOR);
      emit_byte(OP_DIVIDE);
      emit_bytes(set_op, static_cast<uint8_t>(arg));
      break;
    case TOKEN_STAR_STAR_EQUAL:
      advance();
      emit_bytes(get_op, static_cast<uint8_t>(arg));
      parse_precedence(PREC_POW);
      emit_byte(OP_POW);
      emit_bytes(set_op, static_cast<uint8_t>(arg));
      break;
    case TOKEN_SLASH_SLASH_EQUAL:
      advance();
      emit_bytes(get_op, static_cast<uint8_t>(arg));
      parse_precedence(PREC_FACTOR);
      emit_byte(OP_INT_DIVIDE);
      emit_bytes(set_op, static_cast<uint8_t>(arg));
      break;
    case TOKEN_PLUS_PLUS:
      advance();
      emit_bytes(get_op, static_cast<uint8_t>(arg));
      emit_constant(NUMBER_VAL(1));
      emit_byte(OP_ADD);
      emit_bytes(set_op, static_cast<uint8_t>(arg));
      break;
    case TOKEN_MINUS_MINUS:
      advance();
      emit_bytes(get_op, static_cast<uint8_t>(arg));
      emit_constant(NUMBER_VAL(1));
      emit_byte(OP_SUBTRACT);
      emit_bytes(set_op, static_cast<uint8_t>(arg));
      break;
    default:
      emit_bytes(get_op, static_cast<uint8_t>(arg));
  }
}

static void variable(bool can_assign) {
  named_variable(parser.prev, can_assign);
}

static Token synthetic_token(const char* text) {
  Token token;
  token.start = text;
  token.length = strlen(text);
  return token;
}

static void super_(bool can_assign) {
  if (curr_class == nullptr) error("Can't use 'super' outside of a class.");
  else if (!curr_class->has_superclass) error("Can't use 'super' in a class with no superclass.");
  
  consume(TOKEN_DOT, "Expect '.' after 'super'.");
  consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
  uint8_t name = identifier_constant(&parser.prev);
  
  named_variable(synthetic_token("this"), false);
  if (match(TOKEN_LEFT_PAREN)) {
    uint8_t arg_count = argument_list();
    named_variable(synthetic_token("super"), false);
    emit_bytes(OP_SUPER_INVOKE, name);
    emit_byte(arg_count);
  } else {
    named_variable(synthetic_token("super"), false);
    emit_bytes(OP_GET_SUPER, name);
  }
}

static void this_(bool can_assign) {
  if (curr_class == nullptr) {
    error("Can't use 'this' outside of a class.");
    return;
  }
  variable(false);
} 

static void unary(bool can_assign) {
  TokenType operator_type = parser.prev.type;
  parse_precedence(PREC_UNARY);
  switch (operator_type) {
    case TOKEN_BANG: 
      emit_byte(OP_NOT); 
      break;
    case TOKEN_MINUS: 
      emit_byte(OP_NEGATE); 
      break;
    default: 
      return; 
  }
}

static void lambda(bool can_assign) {
  Compiler compiler;
  init_compiler(&compiler, TYPE_LAMBDA);
  begin_scope(); 
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'fn' when declaring a lambda.");
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      curr->function->arity++;
      if (curr->function->arity > 255) {
        error_at_current("Can't have more than 255 parameters.");
      }
      uint8_t constant = parse_variable("Expect parameter name.");
      define_variable(constant);
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  block();
  ObjFunction* function = end_compiler();
  emit_bytes(OP_CLOSURE, make_constant(OBJ_VAL(function)));
  for (int i = 0; i < function->upvalue_count; i++) {
    emit_byte(compiler.upvalues[i].is_local ? 1 : 0);
    emit_byte(compiler.upvalues[i].index);
  }
}

ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping,    call,     PREC_CALL},
  [TOKEN_RIGHT_PAREN]   = {nullptr,     nullptr,  PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {nullptr,     nullptr,  PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {nullptr,     nullptr,  PREC_NONE},
  [TOKEN_COMMA]         = {nullptr,     nullptr,  PREC_NONE},
  [TOKEN_DOT]           = {nullptr,     dot,      PREC_CALL},
  [TOKEN_MINUS]         = {unary,       binary,   PREC_TERM},
  [TOKEN_PLUS]          = {nullptr,     binary,   PREC_TERM},
  [TOKEN_SEMICOLON]     = {nullptr,     nullptr,  PREC_NONE},
  [TOKEN_SLASH]         = {nullptr,     binary,   PREC_FACTOR},
  [TOKEN_STAR]          = {nullptr,     binary,   PREC_FACTOR},
  [TOKEN_STAR_STAR]     = {nullptr,     binary,   PREC_POW},
  [TOKEN_SLASH_SLASH]   = {nullptr,     binary,   PREC_FACTOR},
  [TOKEN_BANG]          = {unary,       nullptr,  PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {nullptr,     binary,   PREC_EQUALITY},
  [TOKEN_EQUAL]         = {nullptr,     nullptr,  PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {nullptr,     binary,   PREC_EQUALITY},
  [TOKEN_GREATER]       = {nullptr,     binary,   PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {nullptr,     binary,   PREC_COMPARISON},
  [TOKEN_LESS]          = {nullptr,     binary,   PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {nullptr,     binary,   PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {variable,    nullptr,  PREC_NONE},
  [TOKEN_STRING]        = {string,      nullptr,  PREC_NONE},
  [TOKEN_NUMBER]        = {number,      nullptr,  PREC_NONE},
  [TOKEN_AND]           = {nullptr,     and_,     PREC_AND},
  [TOKEN_CLASS]         = {nullptr,     nullptr,  PREC_NONE},
  [TOKEN_ELSE]          = {nullptr,     nullptr,  PREC_NONE},
  [TOKEN_FALSE]         = {literal,     nullptr,  PREC_NONE},
  [TOKEN_FOR]           = {nullptr,     nullptr,  PREC_NONE},
  [TOKEN_FN]            = {lambda,      nullptr,  PREC_NONE},
  [TOKEN_IF]            = {nullptr,     nullptr,  PREC_NONE},
  [TOKEN_NIL]           = {literal,     nullptr,  PREC_NONE},
  [TOKEN_OR]            = {nullptr,     or_,      PREC_OR},
  [TOKEN_RETURN]        = {nullptr,     nullptr,  PREC_NONE},
  [TOKEN_SUPER]         = {super_,      nullptr,  PREC_NONE},
  [TOKEN_THIS]          = {this_,       nullptr,  PREC_NONE},
  [TOKEN_TRUE]          = {literal,      nullptr, PREC_NONE},
  [TOKEN_LET]           = {nullptr,     nullptr,  PREC_NONE},
  [TOKEN_WHILE]         = {nullptr,     nullptr,  PREC_NONE},
  [TOKEN_ERROR]         = {nullptr,     nullptr,  PREC_NONE},
  [TOKEN_EOF]           = {nullptr,     nullptr,  PREC_NONE},
};

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
  if (can_assign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}

static ParseRule* get_rule(TokenType type) {
  return &rules[type];
}

static void expression() {
  parse_precedence(PREC_ASSIGNMENT);
}

static void function(FunctionType type) {
  Compiler compiler;
  init_compiler(&compiler, type);
  begin_scope(); 
  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      curr->function->arity++;
      if (curr->function->arity > 255) {
        error_at_current("Can't have more than 255 parameters.");
      }
      uint8_t constant = parse_variable("Expect parameter name.");
      define_variable(constant);
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  block();
  ObjFunction* function = end_compiler();
  emit_bytes(OP_CLOSURE, make_constant(OBJ_VAL(function)));
  for (int i = 0; i < function->upvalue_count; i++) {
    emit_byte(compiler.upvalues[i].is_local ? 1 : 0);
    emit_byte(compiler.upvalues[i].index);
  }
}

static void method() {
  consume(TOKEN_IDENTIFIER, "Expect method name.");
  uint8_t constant = identifier_constant(&parser.prev);
  FunctionType type = TYPE_METHOD;
  if (
    parser.prev.length == curr_class->name.length && 
    memcmp(parser.prev.start, curr_class->name.start, parser.prev.length) == 0
  ) {
    type = TYPE_INITIALIZER;
  }
  function(type);
  emit_bytes(OP_METHOD, constant);
}

static void class_declaration() {
  consume(TOKEN_IDENTIFIER, "Expect class name.");
  Token class_name = parser.prev;
  uint8_t name_constant = identifier_constant(&parser.prev);
 
  declare_variable();
  emit_bytes(OP_CLASS, name_constant);
  define_variable(name_constant);
  
  ClassCompiler class_compiler;
  class_compiler.name = class_name;
  class_compiler.has_superclass = false;
  class_compiler.enclosing = curr_class;
  curr_class = &class_compiler;
  
  if (match(TOKEN_COLON)) {
    consume(TOKEN_IDENTIFIER, "Expect superclass name.");
    variable(false);
    if (identifiers_equal(&class_name, &parser.prev)) {
      error("A class can't inherit from itself.");
    }
    begin_scope();
    add_local(synthetic_token("super"));
    define_variable(0);
    named_variable(class_name, false);
    emit_byte(OP_INHERIT);
    class_compiler.has_superclass = true;
  }
  
  named_variable(class_name, false);
  consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    method();
  }
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
  emit_byte(OP_POP);
  
  if (class_compiler.has_superclass) end_scope();
  curr_class = curr_class->enclosing;
}

static void fn_declaration() {
  uint8_t global = parse_variable("Expect function name.");
  mark_initialized();
  function(TYPE_FUNCTION);
  define_variable(global);
}

static void var_declaration() {
  uint8_t global = parse_variable("Expect variable name.");
  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emit_byte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
  define_variable(global);
}

static void expression_statement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emit_byte(OP_POP);
}

static void for_statement() {
  begin_scope();
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  if (match(TOKEN_SEMICOLON)) {
    // nothing
  } else if (match(TOKEN_LET)) {
    var_declaration();
  } else {
    expression_statement();
  }
  int loop_start = curr_chunk()->count;
  int exit_jump = -1;
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
    exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP); 
  }
  if (!match(TOKEN_RIGHT_PAREN)) {
    int body_jump = emit_jump(OP_JUMP);
    int increment_start = curr_chunk()->count;
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

static void return_statement() {
  if (curr->type == TYPE_SCRIPT) {
    error("Can't return from top-level code.");
  }
  if (match(TOKEN_SEMICOLON)) {
    emit_return();
  } else {
    if (curr->type == TYPE_INITIALIZER) {
      error("Can't return a value from an initializer.");
    }
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
    emit_byte(OP_RETURN);
  }
}

static void while_statement() {
  int loop_start = curr_chunk()->count;
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
      case TOKEN_RETURN:
        return;
      default:
        ; 
    }
    advance();
  }
}

static void declaration() {
  if (match(TOKEN_CLASS)) {
    class_declaration();
  } else if (match(TOKEN_FN)) {
    fn_declaration();
  } else if (match(TOKEN_LET)) {
    var_declaration();
  } else {
    statement();
  }
  if (parser.panic_mode) synchronize();
}

static void statement() {
  if (match(TOKEN_FOR)) {
    for_statement();
  } else if (match(TOKEN_IF)) {
    if_statement();
  } else if (match(TOKEN_RETURN)) {
    return_statement();
  } else if (match(TOKEN_WHILE)) {
    while_statement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    begin_scope();
    block();
    end_scope();
  } else {
    expression_statement();
  }
}

ObjFunction* compile(const char* source) {
  init_scanner(source);
  Compiler compiler;
  init_compiler(&compiler, TYPE_SCRIPT);
  parser.had_error = false;
  parser.panic_mode = false;
  advance();
  while (!match(TOKEN_EOF)) {
    declaration();
  }
  ObjFunction* function = end_compiler();
  return parser.had_error ? nullptr : function;
}

void mark_compiler_roots() {
  Compiler* compiler = curr;
  while (compiler != nullptr) {
    mark_object(static_cast<Obj*>(compiler->function));
    compiler = compiler->enclosing;
  }
}

