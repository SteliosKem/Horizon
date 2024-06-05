#pragma once
#include <string>
#include <vector>
#include <array>
#include "error.h"
#include "token.h"

class Lexer {
public:
	Lexer(const std::string& _source, ErrorHandler* error_handler) : source(_source), error_handler(error_handler) {}
	Lexer() { source = ""; }

	char current_char = '\0';								// Current character
	int index = -1;											// Current index
	int line = 1;											// Current line inside a file
	std::string source;										// The source code to lex

											
	std::vector<Token> analyze();							// Lexes the source code and returns vector of tokens
	bool had_error = false;									// If an error is produced from the lexer it is reported here
	std::vector<Token> out;

	static bool is_digit(char character);					// Check if a character is a digit
	static bool is_alpha(char character);					// Check if a character is alphanumeric

	std::array<std::string, 5> keywords{ "return", "isize", "let", "fn", "void"};
private:
	void next();											// Advances the index and updates current_char
	void back();
	bool match(char expected);								// Match next character

	Token lex();											// Lex a single Token

	Token string();											// Make string Token
	Token number();											// Make number Token
	Token identifier();										// Make identifier/keyword Token

	ErrorHandler* error_handler;
};