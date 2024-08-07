#include "pch.h"
#include "codegen.h"
#include <string>
#include <format>

using std::shared_ptr, std::dynamic_pointer_cast;

void CodeGenerator::generate_asm() {
	for (shared_ptr<Statement>& stmt : ast->statements) {								// For every statement in the AST, generate assembly instructions
		generate_statement(stmt);
	}
	assembly_out = headers + ".text\n" + text;
}

int CodeGenerator::do_operation(shared_ptr<Expression> expression) {
	switch (expression->type)																								// to_where is the register to set a value
	{
	case CONSTANT_EXPR:
		return dynamic_pointer_cast<Constant>(expression)->value;
	case UNARY_EXPR:
	{
		shared_ptr<UnaryExpression> unary = dynamic_pointer_cast<UnaryExpression>(expression);
		switch (unary->operator_type)
		{
		case TOKEN_MINUS:
			return -do_operation(unary->expression);
		case TOKEN_BANG:															// In NOT operation 0 becomes true and anything else false
			return !do_operation(unary->expression);
		case TOKEN_TILDE:
			return ~do_operation(unary->expression);
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
			return do_operation(binary->expression_a) + do_operation(binary->expression_b);
		case TOKEN_STAR:
			return do_operation(binary->expression_a) * do_operation(binary->expression_b);
		case TOKEN_MINUS:
			return do_operation(binary->expression_a) - do_operation(binary->expression_b);
		case TOKEN_SLASH:
			return do_operation(binary->expression_a) / do_operation(binary->expression_b);
		case TOKEN_PERCENT:
			return do_operation(binary->expression_a) % do_operation(binary->expression_b);
		case TOKEN_EQUAL_EQUAL:
			return do_operation(binary->expression_a) == do_operation(binary->expression_b);
		case TOKEN_BANG_EQUAL:
			return do_operation(binary->expression_a) != do_operation(binary->expression_b);
		case TOKEN_GREATER_EQUAL:
			return do_operation(binary->expression_a) >= do_operation(binary->expression_b);
		case TOKEN_LESS_EQUAL:
			return do_operation(binary->expression_a) <= do_operation(binary->expression_b);
		case TOKEN_GREATER:
			return do_operation(binary->expression_a) > do_operation(binary->expression_b);
		case TOKEN_LESS:
			return do_operation(binary->expression_a) < do_operation(binary->expression_b);
		case TOKEN_OR:
			return do_operation(binary->expression_a) || do_operation(binary->expression_b);
		case TOKEN_AND:
			return do_operation(binary->expression_a) && do_operation(binary->expression_b);
		default:
			break;
		}
		break;
	}
	case NAME:
	default:
		op_error = true;
		return 0;
		break;
	}
}


std::string CodeGenerator::simplify(shared_ptr<Expression> expression) {
	int to_ret = do_operation(expression);
	if (op_error)
		make_error("Cannot assign non constant");
	op_error = false;
	return std::to_string(to_ret);
}

void CodeGenerator::generate_function_decl(const shared_ptr<Function>& function) {		// Handle Function declarations
	if (std::find(global_variables.begin(), global_variables.end(), function->name) != global_variables.end()) {
		make_error("Already declared global variable " + function->name);
	}
	else {
		global_variables.push_back(function->name);
	}
	new_scope();
	int param_index = 16;
	// Parameters:
	for (shared_ptr<Name>& i : function->parameters) {
		if (local_variables[local_variables.size() - 1].find(i->name) != local_variables[local_variables.size() - 1].end()) {
			make_error("Already declared variable " + i->name + " in this scope");
		}

		local_variables[local_variables.size() - 1][i->name] = param_index;
		param_index += 8;
	}


	headers += std::format(".globl {0}\n", function->name);						
	generate_label(function->name);														
	generate_instruction("push %rbp");													// } Function prologue, save stack frame
	generate_instruction("mov %rsp, %rbp");												// }
	generate_compound(function->statement);
	generate_instruction("mov $0, %rax");												// Return 0 at end, if there is a return statement this is skipped
	generate_instruction("mov %rbp, %rsp");												// } Function epilogue, revert stack frame
	generate_instruction("pop %rbp");													// }
	generate_instruction("ret");
	// Generates declaration and statements inside the function
	pop_scope();
}

void CodeGenerator::generate_return(const shared_ptr<Return>& return_stmt) {			// Emits return
	if(!return_stmt->is_empty)
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
	case COMPOUND_STM:
		generate_compound(dynamic_pointer_cast<Compound>(statement));
		break;
	case DO_WHILE_STM:
	case WHILE_STM:
		generate_while_statement(dynamic_pointer_cast<WhileStatement>(statement));
		break;
	case BREAK_STM:
		loop_flow_statement(dynamic_pointer_cast<BreakStatement>(statement));
		break;
	case CONTINUE_STM:
		loop_flow_statement(dynamic_pointer_cast<ContinueStatement>(statement));
		break;
	case FOR_STM:
		generate_for_statement(dynamic_pointer_cast<ForStatement>(statement));
		break;
	case EMPTY_STM:
	default:
		break;
	}
}

void CodeGenerator::loop_flow_statement(const std::shared_ptr<BreakStatement> break_statement) {
	std::pair<NodeType, int> loop = loop_positions[loop_positions.size() - 1];
	if (loop_positions.size() > 0) {
		generate_instruction(std::format("jmp _while_end{0}", std::get<int>(loop)));
	}
	else
		make_error("Break statement outside of loop body");
}

void CodeGenerator::loop_flow_statement(const std::shared_ptr<ContinueStatement> continue_statement) {
	std::pair<NodeType, int> loop = loop_positions[loop_positions.size() - 1];
	if (loop_positions.size() > 0) {
		if(std::get<NodeType>(loop) == WHILE_STM)
			generate_instruction(std::format("jmp _while_start{0}", std::get<int>(loop)));
		else
			generate_instruction(std::format("jmp _for_closing_expr{0}", std::get<int>(loop)));
	}
	else
		make_error("Continue statement outside of loop body");
}

void CodeGenerator::call(const std::shared_ptr<Call> call_expression) {
	for (int i = call_expression->arguments.size() - 1; i >= 0; i--) {
		generate_expression(call_expression->arguments[i], "%rax");
		generate_instruction("push %rax");
	}
	generate_instruction("call " + call_expression->name);
	int stack_cleanup = 8 * call_expression->arguments.size();
	generate_instruction(std::format("add ${0}, %rsp", stack_cleanup));
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
		bool is_global = false;
		shared_ptr<VariableAssignment> assignment = dynamic_pointer_cast<VariableAssignment>(expression);
		if (local_variables[local_variables.size()-1].find(assignment->variable_name) == local_variables[local_variables.size() - 1].end()) {
			if (std::find(global_variables.begin(), global_variables.end(), assignment->variable_name) == global_variables.end()) {
				make_error("Variable " + assignment->variable_name + " is not declared in this scope");
			}
			else
				is_global = true;
		}
		int stack_offset;
		if(!is_global)
			stack_offset = local_variables[local_variables.size() - 1][assignment->variable_name];

		std::string access = is_global ? assignment->variable_name + "(%rip)" : std::format("{0}(%rbp)", stack_offset);

		if (!assignment->is_compound) {
			generate_expression(assignment->to_assign, "%rax");
			generate_instruction("mov %rax, " + access);
		}
		else {
			switch (assignment->compound_type) {
			case INCREMENT:
				generate_instruction("add $1, " + access);
				generate_instruction("mov " + access + ", %rax");
				break;
			case DECREMENT:
				generate_instruction("sub $1, " + access);
				generate_instruction("mov " + access + ", % rax");
				break;
			case ADDITION:
				generate_expression(assignment->to_assign, "%rax");
				generate_instruction("add %rax, " + access);
				generate_instruction("mov " + access + ", %rax");
				break;
			case SUBTRACTION:
				generate_expression(assignment->to_assign, "%rax");
				generate_instruction("sub %rax, " + access);
				generate_instruction("mov " + access + ", %rax");
				break;
			case MULTIPLICATION:
				generate_expression(assignment->to_assign, "%rax");
				generate_instruction("imul " + access + ", %rax");
				generate_instruction("mov " + access + ", %rax");
				break;
			case DIVISION:
				generate_expression(assignment->to_assign, "%rcx");
				generate_instruction("mov " + access + ", %rax");
				generate_instruction("cdq");
				generate_instruction("idivq %rcx");										// Divide the two expressions
				generate_instruction("mov %rax, " + access);
				break;
			case MOD:
				generate_expression(assignment->to_assign, "%rcx");
				generate_instruction("mov " + access + ", %rax");
				generate_instruction("cdq");
				generate_instruction("idivq %rcx");										// Divide the two expressions
				generate_instruction("mov %rdx, " + access);
				generate_instruction("mov " + access + ", %rax");
				break;
			default:
				break;
			}
			
		}

		
		break;
	}
	case NAME: {
		bool is_global = false;
		shared_ptr<Name> name = dynamic_pointer_cast<Name>(expression);
		if (local_variables[local_variables.size() - 1].find(name->name) == local_variables[local_variables.size() - 1].end()) {
			if (std::find(global_variables.begin(), global_variables.end(), name->name) == global_variables.end()) {
				make_error("Variable " + name->name + " is not declared in this scope");
			}
			else
				is_global = true;
		}
		if (is_global) {
			generate_instruction("mov " + name->name + "(%rip), %rax");
		}
		else {
			int stack_offset = local_variables[local_variables.size() - 1][name->name];
			generate_instruction(std::format("mov {0}(%rbp), %rax", stack_offset));
		}
		
		break;
	}
	case CALL_EXPR:
		call(dynamic_pointer_cast<Call>(expression));
		break;
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

void CodeGenerator::generate_while_statement(const std::shared_ptr<WhileStatement> while_statement) {
	
	int current_jump = ++jump_label_counter;
	loop_positions.push_back(std::make_pair(WHILE_STM, current_jump));
	if (while_statement->type == DO_WHILE_STM) {
		generate_label(std::format("_while_start{0}", current_jump));
		generate_statement(while_statement->body);
		generate_expression(while_statement->condition, "%rax");
		generate_instruction("cmp $0, %rax");
		generate_instruction(std::format("jne _while_start{0}", current_jump));
		generate_label(std::format("_while_end{0}", current_jump));
	}
	else {
		generate_label(std::format("_while_start{0}", current_jump));
		generate_expression(while_statement->condition, "%rax");
		generate_instruction("cmp $0, %rax");
		generate_instruction(std::format("je _while_end{0}", current_jump));
		generate_statement(while_statement->body);
		generate_instruction(std::format("jmp _while_start{0}", current_jump));
		generate_label(std::format("_while_end{0}", current_jump));
	}
	loop_positions.pop_back();
}

void CodeGenerator::generate_for_statement(const std::shared_ptr<ForStatement> for_statement) {
	int current_jump = ++jump_label_counter;
	loop_positions.push_back(std::make_pair(FOR_STM, current_jump));
	new_scope();
	generate_statement(for_statement->initializer);
	generate_label(std::format("_while_start{0}", current_jump));
	generate_expression(for_statement->condition, "%rax");
	generate_instruction("cmp $0, %rax");
	generate_instruction(std::format("je _while_end{0}", current_jump));
	generate_statement(for_statement->body);
	generate_label(std::format("_for_closing_expr{0}", current_jump));
	generate_expression(for_statement->post, "%rax");
	generate_instruction(std::format("jmp _while_start{0}", current_jump));
	generate_label(std::format("_while_end{0}", current_jump));
	pop_scope();
	loop_positions.pop_back();
}

void CodeGenerator::generate_var_declaration(std::shared_ptr<VariableDeclaration> decl) {
	if (local_variables.size() != 0) {
		stack_index -= 8;
		if (local_variables[local_variables.size() - 1].find(decl->variable_name) != local_variables[local_variables.size() - 1].end()) {
			make_error("Already declared variable " + decl->variable_name + " in this scope");
		}

		if (!decl->is_init)
			generate_instruction("push $0");
		else {
			generate_expression(decl->optional_to_assign, "%rax");
			generate_instruction("push %rax");
		}
		local_variables[local_variables.size() - 1][decl->variable_name] = stack_index;
	}
	else {
		if (std::find(global_variables.begin(), global_variables.end(), decl->variable_name) != global_variables.end()) {
			make_error("Already declared global variable " + decl->variable_name);
		}
		global_variables.push_back(decl->variable_name);
		generate_header(".globl " + decl->variable_name);
		if (decl->is_init) {
			generate_header(".data");
			generate_header(".align 4");
			generate_header(decl->variable_name + ":");
			generate_header("\t.long " + simplify(decl->optional_to_assign));
		}
		else {
			generate_header(".bss");
			generate_header(".align 4");
			generate_header(decl->variable_name + ":");
			generate_header("\t.zero 4");
		}
	}
}

void CodeGenerator::make_error(const std::string& message) {
	Token default_tok = Token();
	error_handler->report_error(message, default_tok);
}

inline void CodeGenerator::generate_instruction(const std::string& instruction) {	// Outputs instruction
	text.append("\t" + instruction + "\n");
}

inline void CodeGenerator::generate_header(const std::string& instruction) {		// Outputs instruction
	headers.append(instruction + "\n");
}

void CodeGenerator::generate_label(const std::string& name) {
 	text.append(name + ":\n");
}

void CodeGenerator::generate_compound(std::shared_ptr<Compound> compound) {
	new_scope();
	for (shared_ptr<Statement>& stmt : compound->statements) {
		generate_statement(stmt);
	}
	//int vars = local_variables[local_variables.size() - 1].size();
	//generate_instruction(std::format("add {0}, %rsp", 8 * vars));
	pop_scope();
}