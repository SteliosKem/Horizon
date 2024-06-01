#pragma once
#include <vector>
#include <string>
#include "lexer.h"
#include "error.h"

enum ValueType {
	INTEGER,
	FLOAT,
	STRING,
	VOID,
	BOOL
};

enum NodeType {
	NODE,
	EXPRESSION,
	STATEMENT,
	FUNCTION_STM,
	RETURN_STM,
};

class Node {
public:
	virtual ~Node() = default;
	NodeType type;
};

class Statement : public Node {
public:
	Statement() {
		type = STATEMENT;
	}
};

class Expression : public Node {
public:
	Expression() {
		type = EXPRESSION;
	}
	int value;
};

class Constant : public Expression {

};

class Function : public Statement {
public:
	Function() {
		type = FUNCTION_STM;
	}
	std::string name;
	ValueType return_type;
	std::shared_ptr<Statement> statement;
};

class Return : public Statement {
public:
	Return() {
		type = RETURN_STM;
	}
	std::shared_ptr<Expression> expression;
};

class AST : public Node {
public:
	std::vector<std::shared_ptr<Statement>> statements;
};

class Parser {
public:
	Parser(std::vector<Token> tokens, ErrorHandler* error_handler) : tokens(tokens), error_handler(error_handler) {}
	std::shared_ptr<AST> parse();								// Main parser function, returns the Abstract Syntax Tree
	std::vector<Token> tokens;								// Holds the tokens returned by the lexer
	std::shared_ptr<AST> out;
	void print_ast();
	
private:
	Token current_token;									// Holds current token from the token vector
	int index = -1;
	void next();											// Updates current_token

	void print_node(std::shared_ptr<Statement>& node);
	void print_expression(std::shared_ptr<Expression>& expression);

	std::shared_ptr<Statement> statement();									// Statement handling
	std::shared_ptr<Function> function();									// Function handling
	std::shared_ptr<Return> return_statement();								// Return statement handling
	std::shared_ptr<Expression> expression();								// Expression handling

	bool match(TokenType type);								// Checks if current token type matches the desired one

	ErrorHandler* error_handler;
	void make_error(std::string message);
	bool panic_mode = false;								// If there is an error, the parser enters into panic mode, allowing it to
	void synchronize();										// ignore tokens until it reaches a point of stability, in order to allow
};															// itself to recognize other possible errors

