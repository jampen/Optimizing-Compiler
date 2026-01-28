#pragma once
#include <frontend/tokens.hpp>
#include <stdexcept>
#include <vector>

namespace cyrex
{

struct LexicalError : std::runtime_error
{
    std::string message;

    explicit constexpr LexicalError(const std::string_view message) :
        std::runtime_error({message.begin(), message.end()})
    {
    }
};

// Throws: LexicalError
std::vector<Token> tokenize(const std::string& source_code);
} // namespace cyrex