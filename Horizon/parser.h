#pragma once
#include <vector>
#include <string>
#include "lexer.h"
#include "error.h"

enum ValueType {
	TYPE_INTEGER,
	TYPE_FLOAT,
	TYPE_STRING,
	TYPE_VOID,
	TYPE_BOOL
};

enum NodeType {
	NODE,
	EXPRESSION,
	STATEMENT,
	FUNCTION_STM,
	RETURN_STM,
	UNARY_EXPR,
	CONSTANT_EXPR,
	BINARY_EXPR,
	COMPOUND_STM,
	EXPR_STM
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

class Compound : public Statement {
public:
	Compound() {
		type = COMPOUND_STM;
	}
	std::vector<std::shared_ptr<Statement>> statements;
};



class Expression : public Node {
public:
	Expression() {
		type = EXPRESSION;
	}
};

class ExpressionStatement : public Statement {
public:
	ExpressionStatement() {
		type = EXPR_STM;
	}
	std::shared_ptr<Expression> expression;
};

class Constant : public Expression {
public:
	Constant(int value) : value(value) {
		type = CONSTANT_EXPR;
	}
	int value = 0;
};

class UnaryExpression : public Expression {
public:
	UnaryExpression(TokenType operator_type, std::shared_ptr<Expression> expression) : operator_type(operator_type), expression(expression) {
		type = UNARY_EXPR;
	}
	TokenType operator_type;
	std::shared_ptr<Expression> expression;
};

class BinaryExpression : public Expression {
public:
	BinaryExpression(std::shared_ptr<Expression> expression_a, TokenType operator_type, std::shared_ptr<Expression> expression_b) :
		expression_a(expression_a), operator_type(operator_type), expression_b(expression_b) {
		type = BINARY_EXPR;
	}
	TokenType operator_type;
	std::shared_ptr<Expression> expression_a;
	std::shared_ptr<Expression> expression_b;
};

class Function : public Statement {
public:
	Function() {
		type = FUNCTION_STM;
	}
	std::string name;
	ValueType return_type = TYPE_VOID;
	std::shared_ptr<Compound> statement;
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
	std::shared_ptr<Compound> compound_statement();
	std::shared_ptr<ExpressionStatement> expression_statement();

	// EXPRESSIONS
	std::shared_ptr<Expression> expression();								// Expression handling (Lowest precedence, OR operator)
	std::shared_ptr<Expression> parse_and();								// Lowest		|
	std::shared_ptr<Expression> parse_equality();							//				|
	std::shared_ptr<Expression> parse_relational();							// To			|
	std::shared_ptr<Expression> parse_arithmetic();							// Highest		|
	std::shared_ptr<Expression> parse_term();								//			  \	| /
	std::shared_ptr<Expression> parse_factor();								// Precedence  \_/
	

	bool match(TokenType type);								// Checks if current token type matches the desired one

	ErrorHandler* error_handler;
	void make_error(std::string message);
	bool panic_mode = false;								// If there is an error, the parser enters into panic mode, allowing it to
	void synchronize();										// ignore tokens until it reaches a point of stability, in order to allow
};															// itself to recognize other possible errors

