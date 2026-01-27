#pragma once
#include <map>
#include <string>
#include <vector>

struct Token
{
    enum class Type
    {
        // -- elements --
        Identifier,
        Number,
        String,
        // -- keywords --
        Function,
        Return,
        While,
        If,
        Then,
        Do,
        Else,
        Var,
        Const,
        Inline,
        // types
        Void,
        Char,
        Short,
        Int,
        Long,
        // -- punctuation --
        LeftParen,
        RightParen,
        LeftSqBracket,
        RightSqBracket,

        LeftBrace,
        RightBrace,
        Colon,
        Comma,
        Assign,
        // logic
        And,
        Or,
        Xor,
        Not,
        // arithmetic
        Plus,
        Minus,
        Star,
        Slash,
        // comparison
        Lesser,
        Greater,
        LesserOrEqual,
        GreaterOrEqual,
        Equal,
        NotEqual,
    };
    std::string text;
    Type type;
};

std::vector<Token> tokenize(const std::string_view str,
                            const std::map<std::string, Token::Type>& keywords,
                            const std::map<std::string, Token::Type>& punctuation);