#include "pch.h"
#include "codegen.h"
#include <string>
#include <format>

using std::shared_ptr, std::dynamic_pointer_cast;

void CodeGenerator::generate_asm() {
	for (shared_ptr<Statement>& stmt : ast->statements) {
		generate_statement(stmt);
	}
}

void CodeGenerator::generate_function_decl(const shared_ptr<Function>& function) {
	assembly_out += std::format(".globl _{0}\n_{0}:\n", function->name);
	current_indentation += '\t';
	generate_statement(function->statement);
}

void CodeGenerator::generate_return(const shared_ptr<Return>& return_stmt) {
	assembly_out += current_indentation + std::format("movl ${0}, %eax\n", return_stmt->expression->value);
	assembly_out += current_indentation + "ret\n";
}

void CodeGenerator::generate_statement(const shared_ptr<Statement>& statement) {
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