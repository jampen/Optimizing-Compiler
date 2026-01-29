#include <gtest/gtest.h>

#include <cyrexc/frontend/tokenizer.hpp>


TEST(Tokenizer, FindsKeyword)
{
    const auto tokens = cyrex::tokenize("function");
    ASSERT_EQ(tokens.size(), 1);
    ASSERT_EQ(tokens.at(0).kind, cyrex::Token::Kind::Function);
    ASSERT_EQ(tokens.at(0).text, "function");
}

TEST(Tokenizer, FindsNumber)
{
    const auto tokens = cyrex::tokenize("1");
    ASSERT_EQ(tokens.size(), 1);
    ASSERT_EQ(tokens.at(0).kind, cyrex::Token::Kind::LiteralInteger);
    ASSERT_EQ(tokens.at(0).text, "1");
}

TEST(Tokenizer, FindsIdentifier)
{
    const auto tokens = cyrex::tokenize("hello_world123");
    ASSERT_EQ(tokens.size(), 1);
    ASSERT_EQ(tokens.at(0).kind, cyrex::Token::Kind::Identifier);
    ASSERT_EQ(tokens.at(0).text, "hello_world123");
}

TEST(Tokenizer, FindsString)
{
    const auto tokens = cyrex::tokenize("\"Hello\"");
    ASSERT_EQ(tokens.size(), 1);
    ASSERT_EQ(tokens.at(0).kind, cyrex::Token::Kind::LiteralString);
    ASSERT_EQ(tokens.at(0).text, "Hello");
}

TEST(Tokenizer, ThrowsLexError)
{
    ASSERT_THROW(cyrex::tokenize("\"Unterminated "), cyrex::LexicalError);
}

TEST(Tokenizer, SampleCodeWorks)
{
    const auto tokens = cyrex::tokenize("function main() : u32 { return 0 }");
    ASSERT_EQ(tokens.size(), 10);
    ASSERT_EQ(tokens.at(0).kind, cyrex::Token::Kind::Function);
    ASSERT_EQ(tokens.at(1).kind, cyrex::Token::Kind::Identifier);
    ASSERT_EQ(tokens.at(2).kind, cyrex::Token::Kind::LeftParenthesis);
    ASSERT_EQ(tokens.at(3).kind, cyrex::Token::Kind::RightParenthesis);
    ASSERT_EQ(tokens.at(4).kind, cyrex::Token::Kind::Colon);
    ASSERT_EQ(tokens.at(5).kind, cyrex::Token::Kind::U32);
    ASSERT_EQ(tokens.at(6).kind, cyrex::Token::Kind::LeftCurlyBracket);
    ASSERT_EQ(tokens.at(7).kind, cyrex::Token::Kind::Return);
    ASSERT_EQ(tokens.at(8).kind, cyrex::Token::Kind::LiteralInteger);
    ASSERT_EQ(tokens.at(9).kind, cyrex::Token::Kind::RightCurlyBracket);
}