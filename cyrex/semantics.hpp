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


/*
// returns true if the AST represents a value that cannot be changed
constexpr bool is_constant(const AST& node) const noexcept {
	return std::visit([&](const auto& ast) -> bool {
		using T = std::decay_t<decltype(ast)>;

		// expressions
		if constexpr (std::is_same_v<T, AST::LiteralExpr>) {
			return true;
		}

		if constexpr (std::is_same_v<T, AST::TupleExpr>) {
			for (const auto& e : ast.exprs) {
				if (!e || !e->is_constant())
					return false;
			}
		}

		return true;

	}, node.data);
}
*/