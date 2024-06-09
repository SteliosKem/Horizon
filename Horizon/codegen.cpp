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

void CodeGenerator::generate_function_decl(const shared_ptr<Function>& function) {		// Handle Function declarations
	assembly_out += std::format(".globl {0}\n", function->name);						
	generate_label(function->name);														
	generate_instruction("push %rbp");													// } Function prologue, save stack frame
	generate_instruction("mov %rsp, %rbp");												// }
	generate_compound(function->statement);
	generate_instruction("mov $0, %rax");												// Return 0 at end, if there is a return statement this is skipped
	generate_instruction("ret");
	// Generates declaration and statements inside the function
}

void CodeGenerator::generate_return(const shared_ptr<Return>& return_stmt) {			// Emits return
	generate_expression(return_stmt->expression, "%rax");
	generate_instruction("mov %rbp, %rsp");												// } Function epilogue, revert stack frame
	generate_instruction("pop %rbp");													// }
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
	case EXPR_STM:
		generate_expression(dynamic_pointer_cast<ExpressionStatement>(statement)->expression, "%rax");
		break;
	case VARIABLE_DECL:
		generate_var_declaration(dynamic_pointer_cast<VariableDeclaration>(statement));
		break;
	case IF_STATEMENT:
		make_if_statement(dynamic_pointer_cast<IfStatement>(statement));
		break;
	default:
		break;
	}
}

void CodeGenerator::make_if_statement(const std::shared_ptr<IfStatement> if_statement) {
	int current_jump_label = ++jump_label_counter;
	generate_expression(if_statement->condition, "%rax");
	generate_instruction("cmp $0, %rax");
	
	if (if_statement->has_else)
		generate_instruction(std::format("je _else_body{0}", current_jump_label));
	else
		generate_instruction(std::format("je _continue{0}", current_jump_label));
	generate_statement(if_statement->body);
	generate_instruction(std::format("jmp _continue{0}", current_jump_label));
	if (if_statement->has_else) {
		generate_label(std::format("_else_body{0}", current_jump_label));
		generate_statement(if_statement->else_body);
	}
	generate_label(std::format("_continue{0}", current_jump_label));
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
		case TOKEN_PERCENT:
			generate_expression(binary->expression_b, to_where);					// Handle left expression
			generate_instruction("push " + to_where);								// Push the result to the stack in order to save it	
			generate_expression(binary->expression_a, to_where);					// Handle right expression
			generate_instruction("pop %rcx");										// Pop and get the top of the stack to retrieve the result from left expression
			generate_instruction("cdq");
			generate_instruction("idivq %rcx");										// Divide the two expressions
			generate_instruction("mov %rdx, " + to_where);
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
		case TOKEN_OR: {
			generate_expression(binary->expression_a, to_where);
			generate_instruction("cmp $0, " + to_where);
			int current_jump_label = ++jump_label_counter;
			generate_instruction("je _clause" + std::to_string(current_jump_label));
			generate_instruction("mov $1, " + to_where);
			generate_instruction("jmp _end" + std::to_string(current_jump_label));

			generate_label("_clause" + std::to_string(current_jump_label));
			generate_expression(binary->expression_b, to_where);
			generate_instruction("cmp $0, " + to_where);
			generate_instruction("mov $0, " + to_where);
			generate_instruction("setne %al");

			generate_label("_end" + std::to_string(current_jump_label));
			break;
		}
		case TOKEN_AND: {
			generate_expression(binary->expression_a, to_where);
			generate_instruction("cmp $0, " + to_where);
			int current_jump_label = ++jump_label_counter;
			generate_instruction("jne _clause" + std::to_string(current_jump_label));
			generate_instruction("jmp _end" + std::to_string(current_jump_label));

			generate_label("_clause" + std::to_string(current_jump_label));
			generate_expression(binary->expression_b, to_where);
			generate_instruction("cmp $0, " + to_where);
			generate_instruction("mov $0, " + to_where);
			generate_instruction("setne %al");

			generate_label("_end" + std::to_string(current_jump_label));
			break;
		}
		default:
			break;
		}
		break;
	}
	case VARIABLE_ASSIGN: {
		shared_ptr<VariableAssignment> assignment = dynamic_pointer_cast<VariableAssignment>(expression);
		if (local_variables.find(assignment->variable_name) == local_variables.end()) {
			make_error("Variable " + assignment->variable_name + " is not declared in this scope");
		}
		int stack_offset = local_variables[assignment->variable_name];

		if (!assignment->is_compound) {
			generate_expression(assignment->to_assign, "%rax");
			generate_instruction(std::format("mov %rax, {0}(%rbp)", stack_offset));
		}
		else {
			switch (assignment->compound_type) {
			case INCREMENT:
				generate_instruction(std::format("add $1, {0}(%rbp)", stack_offset));
				generate_instruction(std::format("mov {0}(%rbp), %rax", stack_offset));
				break;
			case DECREMENT:
				generate_instruction(std::format("sub $1, {0}(%rbp)", stack_offset));
				generate_instruction(std::format("mov {0}(%rbp), %rax", stack_offset));
				break;
			case ADDITION:
				generate_expression(assignment->to_assign, "%rax");
				generate_instruction(std::format("add %rax, {0}(%rbp)", stack_offset));
				generate_instruction(std::format("mov {0}(%rbp), %rax", stack_offset));
				break;
			case SUBTRACTION:
				generate_expression(assignment->to_assign, "%rax");
				generate_instruction(std::format("sub %rax, {0}(%rbp)", stack_offset));
				generate_instruction(std::format("mov {0}(%rbp), %rax", stack_offset));
				break;
			case MULTIPLICATION:
				generate_expression(assignment->to_assign, "%rax");
				generate_instruction(std::format("imul {0}(%rbp), %rax", stack_offset));
				generate_instruction(std::format("mov %rax, {0}(%rbp)", stack_offset));
				break;
			case DIVISION:
				generate_expression(assignment->to_assign, "%rcx");
				generate_instruction(std::format("mov {0}(%rbp), %rax", stack_offset));
				generate_instruction("cdq");
				generate_instruction("idivq %rcx");										// Divide the two expressions
				generate_instruction(std::format("mov %rax, {0}(%rbp)", stack_offset));
				break;
			case MOD:
				generate_expression(assignment->to_assign, "%rcx");
				generate_instruction(std::format("mov {0}(%rbp), %rax", stack_offset));
				generate_instruction("cdq");
				generate_instruction("idivq %rcx");										// Divide the two expressions
				generate_instruction(std::format("mov %rdx, {0}(%rbp)", stack_offset));
				generate_instruction(std::format("mov {0}(%rbp), %rax", stack_offset));
				break;
			default:
				break;
			}
			
		}

		
		break;
	}
	case NAME: {
		shared_ptr<Name> name = dynamic_pointer_cast<Name>(expression);
		if (local_variables.find(name->name) == local_variables.end()) {
			make_error("Variable " + name->name + " is not declared in this scope");
		}
		int stack_offset = local_variables[name->name];
		generate_instruction(std::format("mov {0}(%rbp), %rax", stack_offset));
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

void CodeGenerator::generate_var_declaration(std::shared_ptr<VariableDeclaration> decl) {
	stack_index -= 8;
	if (local_variables.find(decl->variable_name) != local_variables.end()) {
		make_error("Already declared variable " + decl->variable_name + " in this scope");
	}
	
	if (!decl->is_init)
		generate_instruction("push $0");
	else {
		generate_expression(decl->optional_to_assign, "%rax");
		generate_instruction("push %rax");
	}
	local_variables[decl->variable_name] = stack_index;
	
}

void CodeGenerator::make_error(const std::string& message) {
	Token default_tok = Token();
	error_handler->report_error(message, default_tok);
}

inline void CodeGenerator::generate_instruction(const std::string& instruction) {	// Outputs instruction
	assembly_out.append("\t" + instruction + "\n");
}

void CodeGenerator::generate_label(const std::string& name) {
 	assembly_out.append(name + ":\n");
}

void CodeGenerator::generate_compound(std::shared_ptr<Compound> compound) {
	for (shared_ptr<Statement>& stmt : compound->statements) {
		generate_statement(stmt);
	}
}