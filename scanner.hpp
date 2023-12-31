#ifndef scanner_h
#define scanner_h

typedef enum {
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
  TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
  TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
  TOKEN_PLUS_PLUS, TOKEN_MINUS_MINUS,
  TOKEN_STAR_STAR, TOKEN_SLASH_SLASH,
  TOKEN_BANG, TOKEN_BANG_EQUAL,
  TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER, TOKEN_GREATER_EQUAL,
  TOKEN_LESS, TOKEN_LESS_EQUAL,
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
  TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
  TOKEN_FOR, TOKEN_FN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
  TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS, TOKEN_COLON,
  TOKEN_TRUE, TOKEN_LET, TOKEN_WHILE,
  TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL,
	TOKEN_STAR_EQUAL, TOKEN_SLASH_EQUAL,
	TOKEN_STAR_STAR_EQUAL, TOKEN_SLASH_SLASH_EQUAL,
  TOKEN_ERROR, TOKEN_EOF
} TokenType;

struct Token {
  TokenType type;
  const char* start;
  int length;
  int line;
};

void init_scanner(const char* source);
Token scan_token();

#endif
