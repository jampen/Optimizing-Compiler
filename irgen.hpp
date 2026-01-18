#pragma once
#include "ir.hpp"
#include <optional>


struct IRGen {
	struct Scope {
		std::unordered_map<std::string, ValueId> symbols;
	};

	ValueId gen(const AST::Ptr& ptr);

	// dispatchers
	ValueId top(const AST::Top& top);
	ValueId stmt(const AST::Stmt& stmt);
	ValueId expr(const AST::Expr& expr);

	// top
	ValueId root(const AST::Root& root);
	ValueId function(const AST::Function& function);
	ValueId parameter_list(const AST::ParameterList& parameter_list);

	// statements
	ValueId if_stmt(const AST::IfStmt& if_stmt);
	ValueId while_stmt(const AST::WhileStmt& while_stmt);
	ValueId block_stmt(const AST::BlockStmt& block);
	ValueId return_stmt(const AST::ReturnStmt& ret);
	ValueId var_stmt(const AST::VariableStmt& var);

	// expressions
	ValueId if_expr(const AST::IfExpr& if_expr);
	ValueId while_expr(const AST::WhileExpr& while_expr);
	ValueId literal_expr(const AST::LiteralExpr& literal);
	ValueId identifier_expr(const AST::IdentifierExpr& identifier);
	ValueId binary_expr(const AST::BinaryExpr& binary);
	ValueId assign_expr(const AST::AssignExpr& assign);

	Constant parse_literal(const AST::LiteralExpr literal);

	// errors
	void push_error(const std::string& error_message);
	
	// insts
	void push_inst(const Opcode o, const ValueId result, const std::vector<ValueId>& operands = {});
	void push_label(const LabelId label_id);

	// produces a new value and returns its id
	ValueId new_value(const AST::Type& type);
	
	void enter_scope();
	std::optional<ValueId> find_symbol(const std::string& name);
	void exit_scope();

	LabelId new_label();

	bool is_constant(const ValueId value_id);

	std::vector<std::string> errors;
	std::vector<Value> values;
	std::unordered_map<ValueId, Constant> constants;
	std::vector<Scope> scopes;
	LabelId next_label_id{};

	Module mod;
	Function* current_fn{};
};