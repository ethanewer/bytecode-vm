#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "scanner.h"

struct Scanner {
  const char* start;
  const char* curr;
  int line;
};

Scanner scanner;

void init_scanner(const char* source) {
  scanner.start = source;
  scanner.curr = source;
  scanner.line = 1;
}

static bool at_end() {
	return *scanner.curr == '\0';
}

static Token make_token(TokenType type) {
	Token token;
	token.type = type;
	token.start = scanner.start;
	token.len = (int) (scanner.curr - scanner.start);
	token.line = scanner.line;
	return token;
}

static Token error_token(const char* msg) {
	Token token;
	token.type = TOKEN_ERROR;
	token.start = msg;
	token.len = strlen(msg);
	token.line = scanner.line;
	return token;
}

static char advance() {
	scanner.curr++;
	return scanner.curr[-1];
}

static bool match(char expected) {
	if (at_end()) return false;
	if (*scanner.curr != expected) return false;
	scanner.curr++;
	return true;
}

static char peek() {
	return *scanner.curr;
}

static char peek_next() {
	if (at_end()) return '\0';
	return scanner.curr[1];
}

static void skip_whitespace() {
  	for (;;) {
		char c = peek();
		switch (c) {
			case ' ':
			case '\r':
			case '\t':
				advance();
				break;
			case '\n':
				scanner.line++;
				advance();
				break;
			case '#':
				while (peek() != '\n' && !at_end()) advance();
				break;
			default:
				return;
		}
	}
}

static TokenType check_keyword(int start, int length, const char* rest, TokenType type) {
	if (scanner.curr - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0) {
		return type;
	}
  	return TOKEN_IDENTIFIER;
}

static TokenType identifier_type() {
	switch (scanner.start[0]) {
		case 'a': return check_keyword(1, 2, "nd", TOKEN_AND);
		case 'c': return check_keyword(1, 4, "lass", TOKEN_CLASS);
		case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
		case 'f':
			if (scanner.curr - scanner.start > 1) {
				switch (scanner.start[1]) {
					case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
					case 'o': return check_keyword(2, 1, "r", TOKEN_FOR);
					case 'n': return TOKEN_FN;
				}
			}
			break;
		case 'i': return check_keyword(1, 1, "f", TOKEN_IF);
		case 'l': return check_keyword(1, 2, "et", TOKEN_LET);
		case 'n': return check_keyword(1, 2, "il", TOKEN_NIL);
		case 'o': return check_keyword(1, 1, "r", TOKEN_OR);
		case 'p': return check_keyword(1, 4, "rint", TOKEN_PRINT);
		case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
		case 's': return check_keyword(1, 4, "uper", TOKEN_SUPER);
		case 't':
			if (scanner.curr - scanner.start > 1) {
				switch (scanner.start[1]) {
				case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
				case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
				}
			}
			break;
		case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);
	}
  	return TOKEN_IDENTIFIER;
}

static Token string() {
	while (peek() != '"' && !at_end()) {
		if (peek() == '\n') scanner.line++;
		advance();
	}
	if (at_end()) return error_token("Unterminated string.");
	advance();
	return make_token(TOKEN_STRING);
}

static Token number() {
	while (isdigit(peek())) advance();
	if (peek() == '.' && isdigit(peek_next())) {
		advance();
		while (isdigit(peek())) advance();
	}
	return make_token(TOKEN_NUMBER);
}

static Token identifier() {
	while (isalpha(peek()) || isdigit(peek())) advance();
	return make_token(identifier_type());
}

Token scan_token() {
	skip_whitespace();
	scanner.start = scanner.curr;
	if (at_end()) return make_token(TOKEN_EOF);

	char c = advance();

	if (isdigit(c)) return number();
	if (isalpha(c)) return identifier();

	switch (c) {
		case '(': return make_token(TOKEN_LEFT_PAREN);
		case ')': return make_token(TOKEN_RIGHT_PAREN);
		case '{': return make_token(TOKEN_LEFT_BRACE);
		case '}': return make_token(TOKEN_RIGHT_BRACE);
		case ';': return make_token(TOKEN_SEMICOLON);
		case ',': return make_token(TOKEN_COMMA);
		case '.': return make_token(TOKEN_DOT);
		case '-': return make_token(match('=') ? TOKEN_MINUS_EQUAL : TOKEN_MINUS);
		case '+': return make_token(match('=') ? TOKEN_PLUS_EQUAL : TOKEN_PLUS);
		case '/': return make_token(match('=') ? TOKEN_SLASH_EQUAL : (match('/') ? (match('=') ? TOKEN_SLASH_SLASH_EQUAL : TOKEN_SLASH_SLASH) : TOKEN_SLASH));
		case '*': return make_token(match('=') ? TOKEN_STAR_EQUAL : (match('*') ? (match('=') ? TOKEN_STAR_STAR_EQUAL : TOKEN_STAR_STAR) : TOKEN_STAR));
		case '!': return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
		case '=': return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
		case '<': return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
		case '>': return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
		case '"': return string();
	}

	return error_token("Unexpected character");
}