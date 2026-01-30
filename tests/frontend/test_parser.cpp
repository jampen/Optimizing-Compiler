//
// Created by Jamie on 28/01/2026.
//
#include "frontend/parser.hpp"

#include <gtest/gtest.h>

struct cyrex::Parser::Testing
{
    static AST::Ptr parse_if(Parser& parser)
    {
        return parser.if_stmt();
    }
};


TEST(Parser, WontReturnNullOnEmptyTokens)
{
    cyrex::Parser parser({});
    ASSERT_NE(parser.parse(), nullptr);
}

TEST(Parser, IfBlockStatement)
{
    using enum cyrex::Token::Kind;
    const auto tokens = std::array{
        cyrex::Token{LiteralInteger, "1"},
        cyrex::Token{LeftCurlyBracket, "{"},
        cyrex::Token{RightCurlyBracket, "}"},
    };
    cyrex::Parser parser(tokens);
    const auto result = cyrex::Parser::Testing::parse_if(parser);
    ASSERT_NE(result, nullptr);
    ASSERT_FALSE(parser.has_errors());
}
