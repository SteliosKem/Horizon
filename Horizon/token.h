#pragma once
#include <string>

enum TokenType {
	TOKEN_L_PAR, TOKEN_R_PAR, TOKEN_L_BRACE, TOKEN_R_BRACE, TOKEN_L_BRACK, TOKEN_R_BRACK, TOKEN_COMMA,		// SINGLE CHARACTER TOKENS
	TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS, TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR, TOKEN_CAP, TOKEN_TILDE,	//			

	TOKEN_BANG, TOKEN_BANG_EQUAL, TOKEN_EQUAL, TOKEN_EQUAL_EQUAL, TOKEN_GREATER, TOKEN_OR,					//
	TOKEN_GREATER_EQUAL, TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL,					// DOUBLE CHARACTER TOKENS
	TOKEN_STAR_EQUAL, TOKEN_SLASH_EQUAL, TOKEN_PLUS_PLUS, TOKEN_MINUS_MINUS, TOKEN_AND,						//

	TOKEN_ID, TOKEN_STR, TOKEN_BOOL, TOKEN_KEYWORD, TOKEN_U8, TOKEN_U16, TOKEN_U32,							//
	TOKEN_U64, TOKEN_I8, TOKEN_I16, TOKEN_I32, TOKEN_I64, TOKEN_F32, TOKEN_F64,								// TYPE TOKENS
	TOKEN_INT, TOKEN_FLOAT,																					//

	TOKEN_ERROR, TOKEN_EOF																					// SPECIAL TOKENS
};



class Token {
public:
	Token(TokenType _type, std::string _value, int _line, int _start, int _end) :
		type(_type), value(_value), line(_line), start_idx(_start), end_idx(_end) {}

	Token() {}

	TokenType type = TOKEN_EOF;
	std::string value = "";
	int line = 0;
	int start_idx = 0;
	int end_idx = 0;

};