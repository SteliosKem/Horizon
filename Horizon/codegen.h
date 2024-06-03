#pragma once
#include <memory>
#include "parser.h"

class CodeGenerator {
public:
	CodeGenerator(const std::shared_ptr<AST>& ast) : ast(ast) {}

	const std::shared_ptr<AST>& ast;
	std::string assembly_out = "";
	void generate_asm();														// Outputs target assembly code
private:
	void generate_function_decl(const std::shared_ptr<Function>& function);		// Function declarations
	void generate_return(const std::shared_ptr<Return>& return_stmt);			// Return statements
	void generate_statement(const std::shared_ptr<Statement>& statement);		// Statements
	void generate_expression(const std::shared_ptr<Expression>& expression, const std::string& to_where);	// Expressions
	void generate_instruction(const std::string& instruction);					// Instruction

	std::string current_indentation = "";
};