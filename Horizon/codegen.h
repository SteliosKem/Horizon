#pragma once
#include <memory>
#include "parser.h"
#include <vector>
#include <unordered_map>

class CodeGenerator {
public:
	CodeGenerator(const std::shared_ptr<AST>& ast, ErrorHandler* error_handler) : ast(ast), error_handler(error_handler) {}

	const std::shared_ptr<AST>& ast;
	std::string assembly_out = "";
	void generate_asm();														// Outputs target assembly code
private:
	std::vector<std::unordered_map<std::string, int>> local_variables;			// Holds variable name and offset from base stack pointer
	int stack_index = 0;														// Stores index of stack for local variables
	void generate_label(const std::string& label);
	void generate_function_decl(const std::shared_ptr<Function>& function);		// Function declarations
	void generate_return(const std::shared_ptr<Return>& return_stmt);			// Return statements
	void generate_statement(const std::shared_ptr<Statement>& statement);		// Statements
	void generate_expression(const std::shared_ptr<Expression>& expression, const std::string& to_where);	// Expressions
	void generate_instruction(const std::string& instruction);					// Instruction
	void generate_comparison(std::shared_ptr<BinaryExpression> binary, const std::string& to_where);
	void generate_compound(std::shared_ptr<Compound> compound);
	void generate_var_declaration(std::shared_ptr<VariableDeclaration> decl);
	void make_error(const std::string& message);
	void make_if_statement(const std::shared_ptr<IfStatement> if_statement);
	void generate_while_statement(const std::shared_ptr<WhileStatement> while_statement);
	void loop_flow_statement(const std::shared_ptr<BreakStatement> break_statement);
	void loop_flow_statement(const std::shared_ptr<ContinueStatement> continue_statement);
	std::string current_indentation = "";
	ErrorHandler* error_handler;

	// COUNTERS
	int jump_label_counter = -1;
	std::vector<int> loop_positions;
};