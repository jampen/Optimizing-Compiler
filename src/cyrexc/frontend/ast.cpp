//
// Created by Jamie on 29/01/2026.
//
#include "ast.hpp"

#include <frontend/ast.hpp>
#include <magic_enum/magic_enum.hpp>

using json = nlohmann::ordered_json;

template <typename... T>
struct overloaded : T...
{
    using T::operator()...;
};

// STMT
namespace
{
using namespace cyrex;

[[nodiscard]] json block_to_json(const AST::BlockStmt& block)
{
    json b;
    b["type"] = "Block";

    if (!block.statements.empty())
    {
        auto& statements = b["statements"];
        for (const auto& s : block.statements)
        {
            statements += AST::to_json(*s);
        }
    }

    return b;
}

[[nodiscard]] json return_to_json(const AST::ReturnStmt& return_stmt)
{
    json r;
    r["type"] = "Return";
    if (return_stmt.expr)
    {
        r["expr"] = AST::to_json(*return_stmt.expr);
    }
    return r;
}


[[nodiscard]] json variable_to_json(const AST::VariableStmt& variable_stmt)
{
    json w;
    w["type"] = "Variable";
    w["name"] = variable_stmt.name;
    w["is-const"] = variable_stmt.is_const;
    if (variable_stmt.initializer)
    {
        w["initializer"] = AST::to_json(*variable_stmt.initializer);
    }

    return w;
}

[[nodiscard]] json while_stmt_to_json(const AST::WhileStmt& while_stmt)
{
    json w;
    w["type"] = "While-Statement";

    if (while_stmt.condition)
    {
        w["condition"] = AST::to_json(*while_stmt.condition);
    }

    if (while_stmt.block)
    {
        w["block"] = AST::to_json(*while_stmt.block);
    }

    return w;
}

[[nodiscard]] json if_stmt_to_json(const AST::IfStmt& if_stmt)
{
    json i;
    i["type"] = "If-Statement";

    if (if_stmt.condition)
    {
        i["condition"] = AST::to_json(*if_stmt.condition);
    }
    if (if_stmt.then_stmt)
    {
        i["then"] = AST::to_json(*if_stmt.then_stmt);
    }
    if (if_stmt.else_stmt)
    {
        i["else"] = AST::to_json(*if_stmt.else_stmt);
    }
    return i;
}

} // namespace

// EXPR
namespace
{
using namespace cyrex;

[[nodiscard]] json binary_expr_to_json(const AST::BinaryExpr& bin_expr)
{
    json i;
    i["type"] = "Binary-Expr";
    i["kind"] = magic_enum::enum_name(bin_expr.kind);

    if (bin_expr.left)
    {
        i["left"] = AST::to_json(*bin_expr.left);
    }

    if (bin_expr.right)
    {
        i["right"] = AST::to_json(*bin_expr.right);
    }

    return i;
}

[[nodiscard]] json identifier_expr_to_json(const AST::IdentifierExpr& ident_expr)
{
    json i;
    i["type"] = "Identifier-Expr";
    i["name"] = ident_expr.name;
    return i;
}

[[nodiscard]] json literal_expr_to_json(const AST::LiteralExpr& literal_expr)
{
    json l;
    l["type"] = "Literal-Expr";
    l["value"] = literal_expr.value;
    return l;
}

[[nodiscard]] json assign_expr_to_json(const AST::AssignExpr& assign_expr)
{
    json w;
    w["type"] = "Assign-Expr";

    if (assign_expr.left)
    {
        w["left"] = AST::to_json(*assign_expr.left);
    }

    if (assign_expr.expr)
    {
        w["expr"] = AST::to_json(*assign_expr.expr);
    }

    return w;
}

[[nodiscard]] json while_expr_to_json(const AST::WhileExpr& while_expr)
{
    json w;
    w["type"] = "While-Expression";

    if (while_expr.condition)
    {
        w["condition"] = AST::to_json(*while_expr.condition);
    }

    if (while_expr.block)
    {
        w["block"] = AST::to_json(*while_expr.block);
    }

    if (while_expr.returns)
    {
        w["returns"] = AST::to_json(*while_expr.returns);
    }
    return w;
}

} // namespace

[[nodiscard]] json if_expr_to_json(const AST::IfExpr& if_expr)
{
    json i;
    i["type"] = "If-Expression";

    if (if_expr.condition)
    {
        i["condition"] = AST::to_json(*if_expr.condition);
    }
    if (if_expr.then_expr)
    {
        i["then"] = AST::to_json(*if_expr.then_expr);
    }
    if (if_expr.else_expr)
    {
        i["else"] = AST::to_json(*if_expr.else_expr);
    }
    return i;
} // namespace

// TOP
namespace
{
using namespace cyrex;


[[nodiscard]] json function_to_json(const AST::Function& function)
{
    json fn;
    fn["type"] = "Function";
    fn["name"] = function.name;
    if (function.block)
    {
        fn["block"] = AST::to_json(*function.block);
    }
    return fn;
}

[[nodiscard]] json parameter_list_to_json(const AST::ParameterList& parameter_list)
{
    json p;
    p["type"] = "Parameter-List";

    if (!parameter_list.items.empty())
    {
        auto& items = p["items"];
        for (const auto& i : parameter_list.items)
        {
            items += AST::to_json(*i);
        }
    }

    return p;
}

[[nodiscard]] json module_to_json(const AST::Module& module)
{
    json m;
    m["type"] = "Module";
    auto& fs = m["functions"];

    for (const auto& f : module.functions)
    {
        fs += AST::to_json(*f);
    }

    return m;
}


[[nodiscard]] json top_to_json(const AST::Top& top)
{
    return std::visit(overloaded{[](const AST::Function& function) { return function_to_json(function); },
                                 [](const AST::Module& module) { return module_to_json(module); },
                                 [](const AST::ParameterList& p) { return parameter_list_to_json(p); }},
                      top);
}

[[nodiscard]] json stmt_to_json(const AST::Stmt& stmt)
{
    return std::visit(
        overloaded{
            [](const AST::BlockStmt& blk) { return block_to_json(blk); },
            [](const AST::ReturnStmt& ret) { return return_to_json(ret); },
            [](const AST::VariableStmt& var) { return variable_to_json(var); },
            [](const AST::WhileStmt& while_stmt) { return while_stmt_to_json(while_stmt); },
            [](const AST::IfStmt& if_stmt) { return if_stmt_to_json(if_stmt); },
        },
        stmt);
}

[[nodiscard]] json expr_to_json(const AST::Expr& expr)
{
    return std::visit(
        overloaded{
            [](const AST::BinaryExpr& bin) { return binary_expr_to_json(bin); },
            [](const AST::IdentifierExpr& ident) { return identifier_expr_to_json(ident); },
            [](const AST::LiteralExpr& lit) { return literal_expr_to_json(lit); },
            [](const AST::AssignExpr& assign) { return assign_expr_to_json(assign); },
            [](const AST::WhileExpr& while_expr) { return while_expr_to_json(while_expr); },
            [](const AST::IfExpr& if_expr) { return if_expr_to_json(if_expr); },
        },
        expr);
}

} // namespace

[[nodiscard]] json AST::to_json(const AST& ast)
{
    return std::visit(overloaded{[](const Top& top) { return top_to_json(top); },
                                 [](const Expr& expr) { return expr_to_json(expr); },
                                 [](const Stmt& stmt) { return stmt_to_json(stmt); }},
                      ast.data);
}
