#ifndef scanner_h
#define scanner_h

#include "common.h"

enum TokenType {
	// Single-character tokens
	TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
	TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
	TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
	TOKEN_STAR_STAR, TOKEN_SLASH_SLASH,
	// One or two character tokens
	TOKEN_BANG, TOKEN_BANG_EQUAL,
	TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
	TOKEN_GREATER, TOKEN_GREATER_EQUAL,
	TOKEN_LESS, TOKEN_LESS_EQUAL,
	// Literals
	TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
	// Keywords
	TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
	TOKEN_FOR, TOKEN_FN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
	TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
	TOKEN_TRUE, TOKEN_LET, TOKEN_WHILE,
	// += -= *= /= **= //=
	TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL,
	TOKEN_STAR_EQUAL, TOKEN_SLASH_EQUAL,
	TOKEN_STAR_STAR_EQUAL, TOKEN_SLASH_SLASH_EQUAL,

	TOKEN_ERROR, TOKEN_EOF
};

struct Token {
	TokenType type;
	const char* start;
	int len;
	int line;
};


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "scanner.h"

class Scanner {
public:
	Scanner() : start(nullptr), curr(nullptr), line(0) {}
	void set_source(const char* source);
	Token scan_token();

private:
	const char* start;
	const char* curr;
	int line;

	bool at_end();
	Token make_token(TokenType type);
	Token error_token(const char* msg);
	char advance();
	bool match(char expected);
	char peek();
	char peek_next();
	void skip_whitespace();
	TokenType check_keyword(int start, int length, const char* rest, TokenType type);
	TokenType identifier_type();
	Token string();
	Token number();
	Token identifier();
};

#endif