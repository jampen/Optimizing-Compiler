//
// Created by Jamie on 28/01/2026.
//

#ifndef CYREXC_PARSER_HPP
#define CYREXC_PARSER_HPP
#include "ast.hpp"
#include "tokens.hpp"

#include <span>

namespace cyrex
{

struct ParsingError
{
};

struct ParsingWarning
{
};

class Parser
{
public:
    // Implemented in tests/parser.hpp
    struct Testing;

    AST::Ptr parse(std::span<Token> tokens);
    bool has_errors() const noexcept;
    bool has_warnings() const noexcept;
    std::span<ParsingError> get_errors() const noexcept;
    std::span<ParsingWarning> get_warnings() const noexcept;

private:
    bool check_is_next(Token::Kind kind);
    bool expect_or_error(Token::Kind kind, std::string_view error);

#pragma region Top
    AST::Ptr function();
    AST::Ptr parameter_list();
#pragma endregion

#pragma region Statements
    AST::Ptr block();
    AST::Ptr if_stmt();
    AST::Ptr while_stmt();
    AST::Ptr return_stmt();
    AST::Ptr variable_stmt();
#pragma endregion

#pragma region Expressions
    AST::Ptr if_expr();
    AST::Ptr while_expr();
    AST::Ptr assign_expr(AST::Ptr&& left);
    AST::Ptr binary_expr(AST::Ptr&& left, AST::BinaryExpr::Kind kind);
    AST::Ptr number_expr();
    AST::Ptr string_expr();
#pragma endregion

    std::size_t token_index{};
    std::vector<ParsingError> errors;
    std::vector<ParsingWarning> warnings;
};

} // namespace cyrex
#endif //CYREXC_PARSER_HPP
