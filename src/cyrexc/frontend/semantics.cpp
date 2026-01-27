#include "semantics.hpp"

#include <stdexcept>

void SemanticAnalyzer::analyze(AST::Ptr& ast)
{
    if (ast == nullptr)
        return;

    if (visited.contains(ast.get()))
        return;
    visited.emplace(ast.get());

    std::visit(
        [&](auto& x)
        {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::same_as<T, AST::Top>)
            {
                analyze_top(ast, x);
            }
            else if constexpr (std::same_as<T, AST::Expr>)
            {
                analyze_expr(ast, x);
            }
            else if constexpr (std::same_as<T, AST::Stmt>)
            {
                analyze_stmt(ast, x);
            }
        },
        ast->data);
}

void SemanticAnalyzer::analyze_top(AST::Ptr& ast, AST::Top& top)
{
    std::visit(
        [&](auto& x)
        {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::same_as<T, AST::Function>)
            {
                analyze(x.parameter_list);
                analyze(x.block);
            }
            else if constexpr (std::same_as<T, AST::Root>)
            {
                for (auto& f : x.functions)
                {
                    analyze(f);
                }
            }
            else if constexpr (std::same_as<T, AST::ParameterList>)
            {
                for (auto& i : x.items)
                {
                    analyze(i);
                }
            }
        },
        top);
}

void SemanticAnalyzer::analyze_expr(AST::Ptr& ast, AST::Expr& expr)
{
    std::visit(
        [&](auto& x)
        {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::same_as<T, AST::BinaryExpr>)
            {
                analyze(x.left);
                analyze(x.right);
            }
            else if constexpr (std::same_as<T, AST::IdentifierExpr>)
            {
                //sym.is_assignable = var->is_assignable;
            }
            else if constexpr (std::same_as<T, AST::LiteralExpr>)
            {
                //sym.is_assignable = false;
            }
            else if constexpr (std::same_as<T, AST::AssignExpr>)
            {
                analyze(x.left);
                analyze(x.expr);
            }
            else if constexpr (std::same_as<T, AST::WhileExpr>)
            {
                analyze(x.condition);
                analyze(x.block);
                analyze(x.returns);
            }
            else if constexpr (std::same_as<T, AST::IfExpr>)
            {
                analyze(x.condition);
                analyze(x.then_expr);
                analyze(x.else_expr);
            }
            else if constexpr (std::same_as<T, AST::TupleExpr>)
            {
                for (auto& item : x.exprs)
                {
                    analyze(item);
                }
            }
            else if constexpr (std::same_as<T, AST::TupleAssignExpr>)
            {
                analyze(x.tup_left);
                analyze(x.tup_right);
            }
        },
        expr);
}

void SemanticAnalyzer::analyze_stmt(AST::Ptr& ast, AST::Stmt& stmt)
{
    std::visit(
        [&](auto& x)
        {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::same_as<T, AST::BlockStmt>)
            {
                begin_scope();
                for (auto& stmt : x.statements)
                {
                    analyze(stmt);
                }
                end_scope();
            }
            else if constexpr (std::same_as<T, AST::ReturnStmt>)
            {
                analyze(x.expr);
            }
            else if constexpr (std::same_as<T, AST::VariableStmt>)
            {
                analyze(x.initializer);
            }
            else if constexpr (std::same_as<T, AST::WhileStmt>)
            {
                analyze(x.condition);
                analyze(x.block);
            }
            else if constexpr (std::same_as<T, AST::IfStmt>)
            {
                analyze(x.condition);
                analyze(x.then_stmt);
                analyze(x.else_stmt);
            }
        },
        stmt);
}

void SemanticAnalyzer::begin_scope()
{
    scopes.emplace_back();
}

void SemanticAnalyzer::end_scope()
{
    scopes.pop_back();
}
