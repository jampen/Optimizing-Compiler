//
// Created by Jamie on 28/01/2026.
//

#ifndef CYREXC_AST_H
#define CYREXC_AST_H

#include <memory>
#include <nlohmann/json.hpp>
#include <variant>
#include <vector>

namespace cyrex
{

struct AST
{
    using Ptr = std::unique_ptr<AST>;

    struct Type
    {
        std::string name;
    };

#pragma region Top Level

    struct Function
    {
        std::string name;
        Type return_type;
        Ptr parameter_list;
        Ptr block;
    };

    struct ParameterList
    {
        std::vector<Ptr> items;
    };

    struct Module
    {
        std::vector<Ptr> functions;
    };
#pragma endregion

#pragma region Expressions
    struct BinaryExpr
    {
        enum class Kind
        {
            Lesser,
            LesserOrEqual,
            Greater,
            GreaterOrEqual,
            Equal,
            NotEqual,
            And,
            Or,
            Xor,
            Add,
            Sub,
            Times,
            Divide,
        };

        Kind kind;
        Ptr left;
        Ptr right;
    };

    struct IdentifierExpr
    {
        std::string name;
    };

    struct LiteralExpr
    {
        Type type;
        std::string value;
    };

    struct WhileExpr
    {
        Ptr condition;
        Ptr block;
        Ptr returns;
    };

    struct IfExpr
    {
        Ptr condition;
        Ptr then_expr;
        Ptr else_expr;
    };

    struct AssignExpr
    {
        Ptr left;
        Ptr expr;
    };
#pragma endregion

#pragma region Statements
    struct BlockStmt
    {
        std::vector<Ptr> statements;
    };

    struct ReturnStmt
    {
        Ptr expr;
    };

    struct VariableStmt
    {
        bool is_const{};
        std::string name;
        Type type;
        Ptr initializer;
    };

    struct WhileStmt
    {
        Ptr condition;
        Ptr block;
    };

    struct IfStmt
    {
        Ptr condition;
        Ptr then_stmt;
        Ptr else_stmt;
    };
#pragma endregion

    [[nodiscard]] static nlohmann::json to_json(const AST& ast);

    using Top = std::variant<Function, Module, ParameterList>;
    using Expr = std::variant<BinaryExpr, IdentifierExpr, LiteralExpr, AssignExpr, WhileExpr, IfExpr>;
    using Stmt = std::variant<BlockStmt, ReturnStmt, VariableStmt, WhileStmt, IfStmt>;

    std::variant<Top, Expr, Stmt> data;
};


} // namespace cyrex

#endif //CYREXC_AST_H
