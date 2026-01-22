#pragma once
#include "ast.hpp"
#include <unordered_map>
#include <unordered_set>
#include <optional>

class SemanticAnalyzer {
private:
	struct Scope {
		std::unordered_map<std::string, AST::Symbol> syms;
	};
public:
	void analyze(AST::Ptr& ast);

private:
	void analyze_top(AST::Ptr& ast, AST::Top& top);
	void analyze_expr(AST::Ptr& ast, AST::Expr& expr);
	void analyze_stmt(AST::Ptr& ast, AST::Stmt& stmt);

private:
	void begin_scope();
	void end_scope();

private:
	std::unordered_set<AST*> visited{};
	std::vector<Scope> scopes;
};
