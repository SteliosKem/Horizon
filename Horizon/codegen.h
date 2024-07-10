#pragma once
#include <memory>
#include "parser.h"
#include <vector>
#include <unordered_map>
#include <utility>
#include <array>

class CodeGenerator {
public:
	CodeGenerator(const std::shared_ptr<AST>& ast, ErrorHandler* error_handler) : ast(ast), error_handler(error_handler) {}

	const std::shared_ptr<AST>& ast;
	std::string headers = "";
	std::string text = "";
	std::string assembly_out = "";
	void generate_asm();														// Outputs target assembly code
private:
	std::vector<std::unordered_map<std::string, int>> local_variables;			// Holds variable name and offset from base stack pointer
	std::vector<std::string> global_variables;
	int stack_index = 0;														// Stores index of stack for local variables
	void generate_label(const std::string& label);
	void generate_function_decl(const std::shared_ptr<Function>& function);		// Function declarations
	void generate_return(const std::shared_ptr<Return>& return_stmt);			// Return statements
	void generate_statement(const std::shared_ptr<Statement>& statement);		// Statements
	void generate_expression(const std::shared_ptr<Expression>& expression, const std::string& to_where);	// Expressions
	void generate_instruction(const std::string& instruction);					// Instruction
	void generate_header(const std::string& instruction);						// Instruction
	void generate_comparison(std::shared_ptr<BinaryExpression> binary, const std::string& to_where);
	void generate_compound(std::shared_ptr<Compound> compound);
	void generate_var_declaration(std::shared_ptr<VariableDeclaration> decl);
	void make_error(const std::string& message);
	void make_if_statement(const std::shared_ptr<IfStatement> if_statement);
	void generate_while_statement(const std::shared_ptr<WhileStatement> while_statement);
	void generate_for_statement(const std::shared_ptr<ForStatement> for_statement);
	void loop_flow_statement(const std::shared_ptr<BreakStatement> break_statement);
	void loop_flow_statement(const std::shared_ptr<ContinueStatement> continue_statement);
	void call(const std::shared_ptr<Call> call_expression);

	int do_operation(std::shared_ptr<Expression> expression);
	bool op_error = false;
	std::string simplify(std::shared_ptr<Expression> expression);

	std::string current_indentation = "";
	ErrorHandler* error_handler;

	void new_scope() {
		if (local_variables.size() > 0) {
			local_variables.push_back(local_variables[local_variables.size() - 1]);
		}
		else
			local_variables.push_back(std::unordered_map<std::string, int>());
	}
	void pop_scope() {
		local_variables.pop_back();
	}

	// COUNTERS
	int jump_label_counter = -1;
	std::vector<std::pair<NodeType, int>> loop_positions;
};