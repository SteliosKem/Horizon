#pragma once
#include <memory>
#include "parser.h"
#include <vector>

class CodeGenerator {
public:
	CodeGenerator(const std::shared_ptr<AST>& ast) : ast(ast) {}

	const std::shared_ptr<AST>& ast;
	std::string assembly_out = "";
	void generate_asm();														// Outputs target assembly code
private:

	void generate_label(const std::string& label);
	void generate_function_decl(const std::shared_ptr<Function>& function);		// Function declarations
	void generate_return(const std::shared_ptr<Return>& return_stmt);			// Return statements
	void generate_statement(const std::shared_ptr<Statement>& statement);		// Statements
	void generate_expression(const std::shared_ptr<Expression>& expression, const std::string& to_where);	// Expressions
	void generate_instruction(const std::string& instruction);					// Instruction
	void generate_comparison(std::shared_ptr<BinaryExpression> binary, const std::string& to_where);
	std::string current_indentation = "";

	// COUNTERS
	int jump_label_counter = -1;
};