#include "pch.h"
#include "parser.h"
#include <memory>

using std::shared_ptr, std::make_shared, std::dynamic_pointer_cast;

shared_ptr<AST> Parser::parse() {
	next();
	out = make_shared<AST>();
	while (current_token.type != TOKEN_EOF) {
		out->statements.push_back(statement());
	}
	return out;
}

shared_ptr<Statement> Parser::statement() {
	
	if (current_token.type == TOKEN_KEYWORD) {
		if (current_token.value == "int")
			return function();
		else if (current_token.value == "return")
			return return_statement();
	}
	else {
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
	if (current_token.value == "int")
		new_function->return_type = INTEGER;

	next();
	Token tok = current_token;

	if (!match(TOKEN_ID)) {
		make_error("Expected function identifier");
	}
	new_function->name = tok.value;
	if (!match(TOKEN_L_PAR)) {
		make_error("Expected '('");
	}
	if (!match(TOKEN_R_PAR)) {
		make_error("Expected ')'");
	}
	if (!match(TOKEN_L_BRACE)) {
		make_error("Expected '{'");
	}
	new_function->statement = statement();
	if (!match(TOKEN_R_BRACE)) {
		make_error("Expected '}'");
	}
	
	return new_function;
}

shared_ptr<Return> Parser::return_statement() {
	next();
	shared_ptr<Return> return_stmt = make_shared<Return>();
	return_stmt->expression = expression();
	return return_stmt;
}

shared_ptr<Expression> Parser::expression() {
	shared_ptr<Expression> expr = make_shared<Expression>();
	Token tok = current_token;
	if (!match(TOKEN_INT)) {
		make_error("Expected integer");
		return expr;
	}
	
	expr->value = stoi(tok.value);
	if (!match(TOKEN_SEMICOLON)) {
		make_error("Expected ';'");
	}
	return expr;
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
	std::cout << expression->value;
}