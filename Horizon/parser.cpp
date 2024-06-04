#include "pch.h"
#include "parser.h"
#include <memory>

using std::shared_ptr, std::make_shared, std::dynamic_pointer_cast;

shared_ptr<AST> Parser::parse() {
	next();
	out = make_shared<AST>();
	while (current_token.type != TOKEN_EOF) {						// Parse all statements in file
		out->statements.push_back(statement());
	}
	return out;
}

shared_ptr<Statement> Parser::statement() {
	if (current_token.type == TOKEN_KEYWORD) {						// Handle statement depending on keyword
		if (current_token.value == "int")
			return function();
		else if (current_token.value == "return")
			return return_statement();
	}
	else {															// Else it is an expression statement
		make_error("Expected statement");
	}

	if (panic_mode)
		synchronize();
	return nullptr;
}

void Parser::make_error(std::string message) {
	if (panic_mode) return;
	error_handler->report_error(message, current_token);
	panic_mode = true;
}

shared_ptr<Function> Parser::function() {
	shared_ptr<Function> new_function = std::make_shared<Function>();
	if (current_token.value == "int")								// Get function type
		new_function->return_type = TYPE_INTEGER;

	next();
	Token tok = current_token;

	if (!match(TOKEN_ID)) {
		make_error("Expected function identifier");
	}
	new_function->name = tok.value;									// Get function name
	if (!match(TOKEN_L_PAR)) {
		make_error("Expected '('");
	}
	if (!match(TOKEN_R_PAR)) {
		make_error("Expected ')'");
	}
	if (!match(TOKEN_L_BRACE)) {
		make_error("Expected '{'");
	}
	new_function->statement = statement();							// Make function body
	if (!match(TOKEN_R_BRACE)) {
		make_error("Expected '}'");
	}
	
	return new_function;
}

shared_ptr<Return> Parser::return_statement() {
	next();
	shared_ptr<Return> return_stmt = make_shared<Return>();
	return_stmt->expression = expression();							// Make expression inside return
	if (!match(TOKEN_SEMICOLON)) {
		make_error("Expected ';'");
	}
	return return_stmt;
}

shared_ptr<Expression> Parser::expression() {
	shared_ptr<Expression> left = parse_term();
	Token tok = current_token;
	while (match(TOKEN_PLUS) || match(TOKEN_MINUS)) {
		TokenType op = tok.type;
		shared_ptr<Expression> right = parse_term();
		left = make_shared<BinaryExpression>(left, op, right);
	}
	
	return left;
}

void Parser::next() {
	index++;
	if (index < tokens.size())
		current_token = tokens[index];
}

bool Parser::match(TokenType type) {
	if (current_token.type != type)
		return false;
	next();											// If current token type matches, advance current_token as well
	return true;
}

void Parser::synchronize() {
	panic_mode = false;

	while (current_token.type != TOKEN_EOF) {
		
		if (current_token.type == TOKEN_SEMICOLON)
			return;
		else if (current_token.type == TOKEN_KEYWORD && current_token.value == "int")
			return;
		else if (current_token.type == TOKEN_KEYWORD && current_token.value == "return")
			return;
		else
			next();
	}
}


void Parser::print_ast() {
	for (shared_ptr<Statement>& stmt : out->statements) {
		print_node(stmt);
		std::cout << '\n';
	}
}

void Parser::print_node(shared_ptr<Statement>& node) {
	switch (node->type)
	{
	case FUNCTION_STM: {
		shared_ptr<Function> function = dynamic_pointer_cast<Function>(node);
		std::cout << "function " << function->name << ": " << function->return_type << "(\n";
		print_node(function->statement);
		std::cout << ")\n";
		break;
	}
	case RETURN_STM: {
		shared_ptr<Return> ret = dynamic_pointer_cast<Return>(node);
		std::cout << "return ";
		print_expression(ret->expression);
		std::cout << "\n";
		break;
	}
	default:
		break;
	}
}

void Parser::print_expression(shared_ptr<Expression>& expression) {
	switch (expression->type)
	{
	case CONSTANT_EXPR:
		std::cout << dynamic_pointer_cast<Constant>(expression)->value;
		break;
	case UNARY_EXPR:
		std::cout << dynamic_pointer_cast<UnaryExpression>(expression)->operator_type << " ";
		print_expression(dynamic_pointer_cast<UnaryExpression>(expression)->expression);
		break;
	case BINARY_EXPR:
		std::cout << "(";
		print_expression(dynamic_pointer_cast<BinaryExpression>(expression)->expression_a);
		std::cout << " " << dynamic_pointer_cast<BinaryExpression>(expression)->operator_type << " ";
		print_expression(dynamic_pointer_cast<BinaryExpression>(expression)->expression_b);
		std::cout << ")";
		break;
	default:
		break;
	}
}

std::shared_ptr<Expression> Parser::parse_factor() {
	shared_ptr<Expression> expr;
	Token tok = current_token;
	if (match(TOKEN_INT)) {											// If token is a value make constant
		expr = make_shared<Constant>(stoi(tok.value));
	}
	else if (match(TOKEN_TILDE) || match(TOKEN_BANG) || match(TOKEN_MINUS)) {	// If token is a unary operator make unary expression
		TokenType op = tok.type;
		shared_ptr<Expression> new_expr = expression();
		expr = make_shared<UnaryExpression>(op, new_expr);
	}
	else if (match(TOKEN_L_PAR)) {
		expr = expression();
		if (!match(TOKEN_R_PAR)) {
			make_error("Expected ')'");
		}
	}

	return expr;
}

std::shared_ptr<Expression> Parser::parse_term() {
	shared_ptr<Expression> left = parse_factor();
	Token tok = current_token;
	while (match(TOKEN_STAR) || match(TOKEN_SLASH)) {
		TokenType op = tok.type;
		shared_ptr<Expression> right = parse_factor();
		left = make_shared<BinaryExpression>(left, op, right);
	}

	return left;
}