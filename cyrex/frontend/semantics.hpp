#pragma once
#include "ast.hpp"
#include <unordered_map>
#include <unordered_set>
#include <optional>

struct SemanticAnalyzer {
	struct Scope {
		std::unordered_map<std::string, Symbol> syms;
	};

	void analyze	 (AST::Ptr& ast);
	void analyze_top (AST::Ptr& ast, AST::Top& top);
	void analyze_expr(AST::Ptr& ast, AST::Expr& expr);
	void analyze_stmt(AST::Ptr& ast, AST::Stmt& stmt);

	void begin_scope();
	void end_scope();

	std::unordered_set<AST*> visited{};
	std::vector<Scope> scopes;
};
