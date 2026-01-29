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

    constexpr explicit Parser(const std::span<const Token> tokens) : m_tokens(tokens)
    {
    }

    [[nodiscard]] AST::Ptr parse();
    [[nodiscard]] constexpr bool has_errors() const noexcept
    {
        return !m_errors.empty();
    }

    [[nodiscard]] constexpr bool has_warnings() const noexcept
    {
        return m_warnings.empty();
    }

    [[nodiscard]] constexpr std::span<const ParsingError> get_errors() const noexcept
    {
        return {m_errors.begin(), m_errors.end()};
    }

    [[nodiscard]] constexpr std::span<const ParsingWarning> get_warnings() const noexcept
    {
        return {m_warnings.begin(), m_warnings.end()};
    }

private:
    [[nodiscard]] constexpr bool is_at_end() const noexcept
    {
        return m_token_index >= m_tokens.size();
    }

    [[nodiscard]] constexpr bool check_is_next(const Token::Kind kind) const noexcept
    {
        // Prevent lookahead into invalid memory from m_tokens.size() being 0
        if (is_at_end())
        {
            return false;
        }

        return get_token_kind_at(m_token_index + 1) == kind;
    }

    [[nodiscard]] constexpr Token::Kind get_token_kind_at(const std::size_t index) const
    {
        if (index >= m_tokens.size())
        {
            throw std::range_error("Index out of bounds");
        }

        return m_tokens[index].kind; // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
    }

    [[nodiscard]] bool expect_or_error(Token::Kind kind, std::string_view error) noexcept;

    constexpr void advance()
    {
        if (is_at_end())
        {
            throw std::range_error("Cannot advance past end of token span");
        }

        m_token_index++;
    }

#pragma region Top
    [[nodiscard]] AST::Ptr function();
    [[nodiscard]] AST::Ptr parameter_list();
#pragma endregion

#pragma region Statements
    [[nodiscard]] AST::Ptr block();
    [[nodiscard]] AST::Ptr if_stmt();
    [[nodiscard]] AST::Ptr while_stmt();
    [[nodiscard]] AST::Ptr return_stmt();
    [[nodiscard]] AST::Ptr variable_stmt();
#pragma endregion

#pragma region Expressions
    [[nodiscard]] AST::Ptr assign_expr(AST::Ptr&& left);
    [[nodiscard]] AST::Ptr binary_expr(AST::Ptr&& left, AST::BinaryExpr::Kind kind);
    [[nodiscard]] AST::Ptr number_expr();
    [[nodiscard]] AST::Ptr string_expr();
#pragma endregion

    std::span<const Token> m_tokens;
    std::size_t m_token_index{};
    std::vector<ParsingError> m_errors;
    std::vector<ParsingWarning> m_warnings;
};

} // namespace cyrex
#endif //CYREXC_PARSER_HPP
