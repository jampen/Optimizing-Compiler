//
// Created by Jamie on 29/01/2026.
//
#include <frontend/ast.hpp>
#include <gtest/gtest.h>


TEST(AST_Json, X)
{
    using namespace cyrex;
    const auto if_stmt = std::make_unique<AST>(
        AST::IfStmt{.condition = std::make_unique<AST>(AST::LiteralExpr{.value = "1", .type = {"int"}}),
                    .then_stmt = std::make_unique<AST>(AST::AssignExpr{
                        .left = std::make_unique<AST>(AST::IdentifierExpr("x")),
                        .expr = std::make_unique<AST>(
                            AST::BinaryExpr{.kind = AST::BinaryExpr::Kind::Add,
                                            .left = std::make_unique<AST>(AST::IdentifierExpr("x")),
                                            .right = std::make_unique<AST>(AST::LiteralExpr{.value = "1", .type = {"int"}})}),
                    })});

    const auto json = AST::to_json(*if_stmt);
    std::cout << "JSON:\n";
    std::cout << json.dump(1) << '\n';
}