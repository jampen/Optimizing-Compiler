#include "token.hpp"

#include "util.hpp"

#include <algorithm>

std::vector<Token> tokenize(const std::string_view str,
                            const std::map<std::string, Token::Type>& keywords,
                            const std::map<std::string, Token::Type>& punctuation)
{
    std::vector<Token> tokens;

    auto start = str.begin();

    while (start != str.end())
    {
        auto is_digit = [](unsigned char ch)
        {
            return isdigit(ch);
        };
        auto is_identifier = [](unsigned char ch)
        {
            return isalpha(ch) || ch == '_';
        };
        auto is_space = [](unsigned char ch)
        {
            return isspace(ch);
        };
        auto is_punct = [](unsigned char ch)
        {
            return ispunct(ch);
        };
        auto is_speech_mark = [](unsigned char ch)
        {
            return ch == '\"';
        };

        // skip whitespace
        start = std::find_if_not(start, str.end(), is_space);
        if (start == str.end())
            break;

        // scan numbers
        if (is_digit(*start))
        {
            auto end = std::find_if_not(start, str.end(), is_digit);
            tokens.emplace_back(std::string{start, end}, Token::Type::Number);
            start = end;
            continue;
        }

        // scan a word
        if (is_identifier(*start))
        {
            auto end = std::find_if_not(start, str.end(), is_identifier);
            auto& tok = tokens.emplace_back(std::string{start, end});
            // may be a keyword
            tok.type = find_or_default(keywords, tok.text, Token::Type::Identifier);
            start = end;
            continue;
        }

        if (is_speech_mark(*start))
        {
            start = std::next(start);
            auto end = std::find_if(start, str.end(), is_speech_mark);
            if (end == str.end())
            {
                // unterminated string
                puts("unterminated string");
                break;
            }
            tokens.emplace_back(std::string{start, end}, Token::Type::String);
            start = std::next(end);
            continue;
        }

        if (is_punct(*start))
        {
            std::string best_match;
            for (auto len = 1; len <= str.end() - start; ++len)
            {
                std::string candidate(start, start + len);
                if (punctuation.contains(candidate))
                    best_match = candidate;
            }
            if (best_match.empty())
                goto unexpected;
            tokens.emplace_back(best_match, punctuation.at(best_match));
            start += best_match.size();
            continue;
        }

    unexpected:
        puts("unexpected");
        start = std::next(start);
        // errors.push(...)
    }

    return tokens;
}
