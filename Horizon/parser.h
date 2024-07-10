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
	EXPR_STM,
	VARIABLE_DECL,
	VARIABLE_ASSIGN,
	NAME,
	IF_STATEMENT,
	WHILE_STM,
	DO_WHILE_STM,
	EMPTY_STM,
	CONTINUE_STM,
	BREAK_STM,
	FOR_STM,
	CALL_EXPR
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

class VariableDeclaration : public Statement {
public:
	VariableDeclaration() {
		type = VARIABLE_DECL;
	}
	std::string variable_name = "";
	ValueType holds_type = TYPE_INTEGER;
	bool is_init = false;
	std::shared_ptr<Expression> optional_to_assign;
	bool is_global = false;
	int global_value;
};

class Name : public Expression {
public:
	Name() {
		type = NAME;
	}
	Name(const std::string& _name) : name(_name) {
		type = NAME;
	}
	std::string name = "";
};

class Call : public Expression {
public:
	Call() {
		type = CALL_EXPR;
	}
	Call(const std::string& _name) : name(_name) {
		type = CALL_EXPR;
	}
	std::string name = "";
	std::vector<std::shared_ptr<Expression>> arguments;
};

enum CompoundAssignment {
	ADDITION,
	SUBTRACTION,
	DIVISION,
	MULTIPLICATION,
	MOD,

	INCREMENT,
	DECREMENT
};

class VariableAssignment : public Expression {
public:
	VariableAssignment() {
		type = VARIABLE_ASSIGN;
	}
	std::string variable_name = "";
	std::shared_ptr<Expression> to_assign;
	bool is_compound = false;
	CompoundAssignment compound_type = ADDITION;
};

class IfStatement : public Statement {
public:
	IfStatement() {
		type = IF_STATEMENT;
	}
	std::shared_ptr<Expression> condition;
	std::shared_ptr<Statement> body;
	bool has_else = false;
	std::shared_ptr<Statement> else_body;
};

class WhileStatement : public Statement {
public:
	WhileStatement(NodeType node_type = WHILE_STM) {
		type = node_type;
	}
	std::shared_ptr<Expression> condition;
	std::shared_ptr<Statement> body;
};

class ForStatement : public Statement {
public:
	ForStatement() {
		type = FOR_STM;
	}
	std::shared_ptr<Expression> condition;
	std::shared_ptr<Expression> post;
	std::shared_ptr<Statement> body;
	std::shared_ptr<Statement> initializer;
};

class ContinueStatement : public Statement {
public:
	ContinueStatement() {
		type = CONTINUE_STM;
	}
};

class BreakStatement : public Statement {
public:
	BreakStatement() {
		type = BREAK_STM;
	}
};

class EmptyStatement : public Statement {
public:
	EmptyStatement() {
		type = EMPTY_STM;
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
	std::vector<std::shared_ptr<Name>> parameters;
};

class Return : public Statement {
public:
	Return() {
		type = RETURN_STM;
	}
	bool is_empty = false;
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

	std::shared_ptr<Statement> statement();									// General statement handling
	std::shared_ptr<Function> function();									// Function declaration handling
	std::shared_ptr<Return> return_statement();								// Return statement handling
	std::shared_ptr<Compound> compound_statement();							// Block {} handling
	std::shared_ptr<ExpressionStatement> expression_statement();			// Simple expression handling
	std::shared_ptr<Statement> variable_declaration();						// Variable declaration and instansiation handling

	std::shared_ptr<Statement> if_statement();
	std::shared_ptr<Statement> while_statement(NodeType node_type = WHILE_STM);
	std::shared_ptr<Statement> for_statement();

	// EXPRESSIONS
	std::shared_ptr<Expression> expression();								// Expression handling (Lowest precedence, OR operator)
	std::shared_ptr<Expression> parse_and();								// Lowest		|
	std::shared_ptr<Expression> parse_equality();							//				|
	std::shared_ptr<Expression> parse_relational();							// To			|
	std::shared_ptr<Expression> parse_arithmetic();							// Highest		|
	std::shared_ptr<Expression> parse_term();								//			  \	| /
	std::shared_ptr<Expression> parse_factor();								// Precedence  \_/

	std::shared_ptr<Expression> assignment_helper(bool is_compound, CompoundAssignment compound);
	

	bool match(TokenType type);								// Checks if current token type matches the desired one

	ErrorHandler* error_handler;
	void make_error(std::string message);
	bool panic_mode = false;								// If there is an error, the parser enters into panic mode, allowing it to
	void synchronize();										// ignore tokens until it reaches a point of stability, in order to allow
};															// itself to recognize other possible errors

