#include "pch.h"
#include "codegen.h"
#include <string>
#include <format>

using std::shared_ptr, std::dynamic_pointer_cast;

void CodeGenerator::generate_asm() {
	for (shared_ptr<Statement>& stmt : ast->statements) {								// For every statement in the AST, generate assembly instructions
		generate_statement(stmt);
	}
}

void CodeGenerator::generate_function_decl(const shared_ptr<Function>& function) {		// Handle Function declarations:
	assembly_out += std::format(".globl _{0}\n_{0}:\n", function->name);				// global (_name)
	current_indentation += '\t';														// (_name):
	generate_statement(function->statement);											//		(function statements)
	// Generates declaration and statements inside the function
}

void CodeGenerator::generate_return(const shared_ptr<Return>& return_stmt) {			// Emits return
	generate_expression(return_stmt->expression, "%eax");
	generate_instruction("ret");
}

void CodeGenerator::generate_statement(const shared_ptr<Statement>& statement) {		// Handles all statements
	switch (statement->type)
	{
	case FUNCTION_STM:
		generate_function_decl(dynamic_pointer_cast<Function>(statement));
		break;
	case RETURN_STM:
		generate_return(dynamic_pointer_cast<Return>(statement));
		break;
	default:
		break;
	}
}

void CodeGenerator::generate_expression(const std::shared_ptr<Expression>& expression, const std::string& to_where) {		// Handles expressions
	switch (expression->type)																								// to_where is the register to set a value
	{
	case CONSTANT_EXPR:
		generate_instruction(std::format("movl ${0}, {1}", dynamic_pointer_cast<Constant>(expression)->value, to_where));	// Constants are just passed to a register
		break;
	case UNARY_EXPR:
	{
		shared_ptr<UnaryExpression> unary = dynamic_pointer_cast<UnaryExpression>(expression);
		switch (unary->operator_type)
		{
		case TOKEN_MINUS:
			generate_expression(unary->expression, to_where);
			generate_instruction(std::format("neg {0}", to_where));					// In unary negation the neg instruction is used
			break;
		case TOKEN_BANG:															// In NOT operation 0 becomes true and anything else false
			generate_expression(unary->expression, to_where);
			generate_instruction(std::format("cmpl $0, {0}", to_where));
			generate_instruction(std::format("xor {0}, {0}", to_where));
			generate_instruction("sete %al");
			break;
		case TOKEN_TILDE:
			generate_expression(unary->expression, to_where);
			generate_instruction(std::format("xor {0}, {0}", to_where));			// Use XOR to get bitwise complement
			break;
		default:
			break;
		}
		break;
	}
	default:
		break;
	}
}

inline void CodeGenerator::generate_instruction(const std::string& instruction) {	// Outputs instruction
	assembly_out += current_indentation + instruction + "\n";
}