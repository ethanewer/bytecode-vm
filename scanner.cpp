#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "scanner.h"

Scanner::Scanner(const char* source) {
  start = source;
  curr = source;
  line = 1;
}

Token Scanner::scan_token() {
	skip_whitespace();
	start = curr;
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

bool Scanner::at_end() {
	return *curr == '\0';
}

Token Scanner::make_token(TokenType type) {
	Token token;
	token.type = type;
	token.start = start;
	token.len = (int) (curr - start);
	token.line = line;
	return token;
}

Token Scanner::error_token(const char* msg) {
	Token token;
	token.type = TOKEN_ERROR;
	token.start = msg;
	token.len = strlen(msg);
	token.line = line;
	return token;
}

char Scanner::advance() {
	curr++;
	return curr[-1];
}

bool Scanner::match(char expected) {
	if (at_end()) return false;
	if (*curr != expected) return false;
	curr++;
	return true;
}

char Scanner::peek() {
	return *curr;
}

char Scanner::peek_next() {
	if (at_end()) return '\0';
	return curr[1];
}

void Scanner::skip_whitespace() {
  	for (;;) {
		char c = peek();
		switch (c) {
			case ' ':
			case '\r':
			case '\t':
				advance();
				break;
			case '\n':
				line++;
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

TokenType Scanner::check_keyword(int begin, int len, const char* rest, TokenType type) {
	if (curr - start == begin + len && memcmp(start + begin, rest, len) == 0) return type;
  	return TOKEN_IDENTIFIER;
}

TokenType Scanner::identifier_type() {
	switch (start[0]) {
		case 'a': return check_keyword(1, 2, "nd", TOKEN_AND);
		case 'c': return check_keyword(1, 4, "lass", TOKEN_CLASS);
		case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
		case 'f':
			if (curr - start > 1) {
				switch (start[1]) {
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
			if (curr - start > 1) {
				switch (start[1]) {
				case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
				case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
				}
			}
			break;
		case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);
	}
  	return TOKEN_IDENTIFIER;
}

Token Scanner::string() {
	while (peek() != '"' && !at_end()) {
		if (peek() == '\n') line++;
		advance();
	}
	if (at_end()) return error_token("Unterminated string.");
	advance();
	return make_token(TOKEN_STRING);
}

Token Scanner::number() {
	while (isdigit(peek())) advance();
	if (peek() == '.' && isdigit(peek_next())) {
		advance();
		while (isdigit(peek())) advance();
	}
	return make_token(TOKEN_NUMBER);
}

Token Scanner::identifier() {
	while (isalpha(peek()) || isdigit(peek())) advance();
	return make_token(identifier_type());
}