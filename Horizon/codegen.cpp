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
	assembly_out += std::format(".globl {0}\n", function->name);						// global (name)
	generate_label(function->name);														// (name):
	generate_statement(function->statement);											//		(function statements)
	// Generates declaration and statements inside the function
}

void CodeGenerator::generate_return(const shared_ptr<Return>& return_stmt) {			// Emits return
	generate_expression(return_stmt->expression, "%rax");
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
		generate_instruction(std::format("mov ${0}, {1}", dynamic_pointer_cast<Constant>(expression)->value, to_where));	// Constants are just passed to a register
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
			generate_instruction(std::format("cmp $0, {0}", to_where));
			generate_instruction(std::format("mov $0, {0}", to_where));
			generate_instruction("sete %al");
			break;
		case TOKEN_TILDE:
			generate_expression(unary->expression, to_where);
			generate_instruction(std::format("not {0}", to_where));			// Use not to get bitwise complement
			break;
		default:
			break;
		}
		break;
	}
	case BINARY_EXPR:
	{
		shared_ptr<BinaryExpression> binary = dynamic_pointer_cast<BinaryExpression>(expression);
		switch (binary->operator_type)
		{
		case TOKEN_PLUS:
			generate_expression(binary->expression_a, to_where);					// Handle left expression
			generate_instruction("push " + to_where);								// Push the result to the stack in order to save it	
			generate_expression(binary->expression_b, to_where);					// Handle right expression
			generate_instruction("pop %rcx");										// Pop and get the top of the stack to retrieve the result from left expression
			generate_instruction("add %rcx, " + to_where);							// Add the two expressions
			break;
		case TOKEN_STAR:															
			generate_expression(binary->expression_a, to_where);					// Handle left expression
			generate_instruction("push " + to_where);								// Push the result to the stack in order to save it	
			generate_expression(binary->expression_b, to_where);					// Handle right expression
			generate_instruction("pop %rcx");										// Pop and get the top of the stack to retrieve the result from left expression
			generate_instruction("imul %rcx, " + to_where);							// Multiply the two expressions
			break;
		case TOKEN_MINUS:
			generate_expression(binary->expression_b, to_where);					// Handle left expression
			generate_instruction("push " + to_where);								// Push the result to the stack in order to save it	
			generate_expression(binary->expression_a, to_where);					// Handle right expression
			generate_instruction("pop %rcx");										// Pop and get the top of the stack to retrieve the result from left expression
			generate_instruction("sub %rcx, " + to_where);							// Subtract expression_b from expression_a and set the result to %rax
			break;
		case TOKEN_SLASH:
			generate_expression(binary->expression_b, to_where);					// Handle left expression
			generate_instruction("push " + to_where);								// Push the result to the stack in order to save it	
			generate_expression(binary->expression_a, to_where);					// Handle right expression
			generate_instruction("pop %rcx");										// Pop and get the top of the stack to retrieve the result from left expression
			generate_instruction("cdq");
			generate_instruction("idivq %rcx");										// Divide the two expressions
			break;
		case TOKEN_EQUAL_EQUAL:
			generate_comparison(binary, to_where);
			generate_instruction("sete %al");
			break;
		case TOKEN_BANG_EQUAL:
			generate_comparison(binary, to_where);
			generate_instruction("setne %al");
			break;
		case TOKEN_GREATER_EQUAL:
			generate_comparison(binary, to_where);
			generate_instruction("setge %al");
			break;
		case TOKEN_LESS_EQUAL:
			generate_comparison(binary, to_where);
			generate_instruction("setle %al");
			break;
		case TOKEN_GREATER:
			generate_comparison(binary, to_where);
			generate_instruction("setg %al");
			break;
		case TOKEN_LESS:
			generate_comparison(binary, to_where);
			generate_instruction("setl %al");
			break;
		case TOKEN_OR:
			generate_expression(binary->expression_a, to_where);
			generate_instruction("cmp $0, " + to_where);
			jump_label_counter++;
			generate_instruction("je _clause" + std::to_string(jump_label_counter));
			generate_instruction("mov $1, " + to_where);
			generate_instruction("jmp _end" + std::to_string(jump_label_counter));

			generate_label("_clause" + std::to_string(jump_label_counter));
			generate_expression(binary->expression_b, to_where);
			generate_instruction("cmp $0, " + to_where);
			generate_instruction("mov $0, " + to_where);
			generate_instruction("setne %al");

			generate_label("_end" + std::to_string(jump_label_counter));
			break;
		case TOKEN_AND:
			generate_expression(binary->expression_a, to_where);
			generate_instruction("cmp $0, " + to_where);
			jump_label_counter++;
			generate_instruction("jne _clause" + std::to_string(jump_label_counter));
			generate_instruction("jmp _end" + std::to_string(jump_label_counter));

			generate_label("_clause" + std::to_string(jump_label_counter));
			generate_expression(binary->expression_b, to_where);
			generate_instruction("cmp $0, " + to_where);
			generate_instruction("mov $0, " + to_where);
			generate_instruction("setne %al");

			generate_label("_end" + std::to_string(jump_label_counter));
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

void CodeGenerator::generate_comparison(shared_ptr<BinaryExpression> binary, const std::string& to_where) {
	generate_expression(binary->expression_a, to_where);					// Handle left expression
	generate_instruction("push " + to_where);								// Push the result to the stack in order to save it	
	generate_expression(binary->expression_b, to_where);					// Handle right expression
	generate_instruction("pop %rcx");										// Pop and get the top of the stack to retrieve the result from left expression
	generate_instruction("cmp %rax, %rcx");
	generate_instruction("mov $0, %rax");
}

inline void CodeGenerator::generate_instruction(const std::string& instruction) {	// Outputs instruction
	assembly_out.append("\t" + instruction + "\n");
}

void CodeGenerator::generate_label(const std::string& name) {
 	assembly_out.append(name + ":\n");
}