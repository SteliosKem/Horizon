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
		if (current_token.value == "fn")
			return function();
		else if (current_token.value == "let")
			return variable_declaration();
		else if (current_token.value == "return")
			return return_statement();
	}
	else {															// Else it is an expression statement
		return expression_statement();
	}

	if (panic_mode)
		synchronize();
	return nullptr;
}

shared_ptr<ExpressionStatement> Parser::expression_statement() {
	shared_ptr<ExpressionStatement> stmt = make_shared<ExpressionStatement>();
	stmt->expression = expression();

	if (!match(TOKEN_SEMICOLON)) {
		make_error("Expected ';'");
	}

	return stmt;
}

void Parser::make_error(std::string message) {
	if (panic_mode) return;
	error_handler->report_error(message, current_token);
	panic_mode = true;
}

shared_ptr<Function> Parser::function() {
	shared_ptr<Function> new_function = std::make_shared<Function>();

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
	if (match(TOKEN_ARROW)) {
		tok = current_token;
		if (!match(TOKEN_KEYWORD)) {
			make_error("Expected type after '->'");
		}
		else if (tok.value == "isize")
			new_function->return_type = TYPE_INTEGER;
		else if (tok.value == "void")
			new_function->return_type = TYPE_VOID;
		else {
			make_error("Expected type after '->'");
		}
	}
	else {
		new_function->return_type = TYPE_VOID;
	}
	if (!match(TOKEN_L_BRACE)) {
		make_error("Expected '{'");
	}
	new_function->statement = compound_statement();							// Make function body
	
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
	if (index + 1 < tokens.size() && tokens[index + 1].type == TOKEN_EQUAL) {
		Token tok = current_token;
		shared_ptr<VariableAssignment> assignment = make_shared<VariableAssignment>();
		if (!match(TOKEN_ID)) {
			make_error("Invalid assignment target");
		}
		assignment->variable_name = tok.value;
		next();
		assignment->to_assign = expression();

		return assignment;
	}
	shared_ptr<Expression> left = parse_and();
	Token tok = current_token;
	while (match(TOKEN_OR)) {
		TokenType op = tok.type;
		shared_ptr<Expression> right = parse_and();
		left = make_shared<BinaryExpression>(left, op, right);
	}
	
	return left;
}

std::shared_ptr<Expression> Parser::parse_and() {
	shared_ptr<Expression> left = parse_equality();
	Token tok = current_token;
	while (match(TOKEN_AND)) {
		TokenType op = tok.type;
		shared_ptr<Expression> right = parse_equality();
		left = make_shared<BinaryExpression>(left, op, right);
	}

	return left;
}

std::shared_ptr<Expression> Parser::parse_equality() {
	shared_ptr<Expression> left = parse_relational();
	Token tok = current_token;
	while (match(TOKEN_EQUAL_EQUAL) || match(TOKEN_BANG_EQUAL)) {
		TokenType op = tok.type;
		shared_ptr<Expression> right = parse_relational();
		left = make_shared<BinaryExpression>(left, op, right);
	}

	return left;
}

std::shared_ptr<Expression> Parser::parse_relational() {
	shared_ptr<Expression> left = parse_arithmetic();
	Token tok = current_token;
	while (match(TOKEN_LESS) || match(TOKEN_GREATER) || match(TOKEN_LESS_EQUAL) || match(TOKEN_GREATER_EQUAL)) {
		TokenType op = tok.type;
		shared_ptr<Expression> right = parse_arithmetic();
		left = make_shared<BinaryExpression>(left, op, right);
	}

	return left;
}

std::shared_ptr<Expression> Parser::parse_arithmetic() {
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
		shared_ptr<Statement> stmt = function->statement;
		print_node(stmt);
		std::cout << ")\n";
		break;
	}
	case COMPOUND_STM: {
		shared_ptr<Compound> compound = dynamic_pointer_cast<Compound>(node);
		for (shared_ptr<Statement>& stmt : compound->statements) {
			print_node(stmt);
		}
		break;
	}
	case EXPR_STM: {
		shared_ptr<ExpressionStatement> expr = dynamic_pointer_cast<ExpressionStatement>(node);
		print_expression(expr->expression);
		break;
	}
	case RETURN_STM: {
		shared_ptr<Return> ret = dynamic_pointer_cast<Return>(node);
		std::cout << "return ";
		print_expression(ret->expression);
		std::cout << "\n";
		break;
	}
	case VARIABLE_DECL: {
		shared_ptr<VariableDeclaration> decl = dynamic_pointer_cast<VariableDeclaration>(node);
		std::cout << "Variable" << decl->variable_name << " of type: " << decl->type << " ";
		if (decl->is_init) {
			print_expression(decl->optional_to_assign);
		}
		
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
	case NAME:
		std::cout << dynamic_pointer_cast<Name>(expression)->name;
		break;
	case UNARY_EXPR:
		std::cout << dynamic_pointer_cast<UnaryExpression>(expression)->operator_type << " ";
		print_expression(dynamic_pointer_cast<UnaryExpression>(expression)->expression);
		break;
	case VARIABLE_ASSIGN:
		std::cout << "Assign ";
		print_expression(dynamic_pointer_cast<VariableAssignment>(expression)->to_assign);
		std::cout << " to variable " << dynamic_pointer_cast<VariableAssignment>(expression)->variable_name;
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
	else if (match(TOKEN_ID)) {
		expr = make_shared<Name>(tok.value);
	}

	return expr;
}

std::shared_ptr<Expression> Parser::parse_term() {
	shared_ptr<Expression> left = parse_factor();
	Token tok = current_token;
	while (match(TOKEN_STAR) || match(TOKEN_SLASH) || match(TOKEN_PERCENT)) {
		TokenType op = tok.type;
		shared_ptr<Expression> right = parse_factor();
		left = make_shared<BinaryExpression>(left, op, right);
	}

	return left;
}

std::shared_ptr<Compound> Parser::compound_statement() {
	shared_ptr<Compound> block = make_shared<Compound>();
	while (current_token.type != TOKEN_R_BRACE && current_token.type != TOKEN_EOF) {
		block->statements.push_back(statement());
		
	}
	if (!match(TOKEN_R_BRACE)) {
		make_error("Expected '}'");
	}
	return block;
}

std::shared_ptr<Statement> Parser::variable_declaration() {
	shared_ptr<VariableDeclaration> variable_decl = make_shared<VariableDeclaration>();
	bool has_type = false;
	next();
	if (!match(TOKEN_ID)) {
		make_error("Expected variable name");
	}
	
	if (match(TOKEN_ARROW)) {
		Token tok = current_token;
		if (!match(TOKEN_KEYWORD)) {
			make_error("Expected variable type");
			
		}
		else {
			if (tok.value == "isize")
				variable_decl->holds_type = TYPE_INTEGER;
			else {
				make_error("Expected variable type");
			}
			has_type = true;
		}
	}
	if (!match(TOKEN_EQUAL)) {
		if (!has_type) {
			make_error("Type must be annotated for an uninitialized variable");
		}
	}
	else {
		variable_decl->is_init = true;
		variable_decl->optional_to_assign = expression();
	}

	if (!match(TOKEN_SEMICOLON)) {
		make_error("Expected ';'");
	}

	return variable_decl;
}

