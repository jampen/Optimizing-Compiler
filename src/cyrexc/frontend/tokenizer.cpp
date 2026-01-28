#include "tokenizer.hpp"

#include <frontend/reserved_words.hpp>
#include <util/cmap.hpp>

namespace
{
constexpr auto reserved_words = cyrex::make_reserved_words();
constexpr auto reserved_punctuation = cyrex::make_reserved_punctuation();

} // namespace

std::vector<cyrex::Token> cyrex::tokenize(const std::string& source_code)
{
    std::vector<Token> tokens;

    auto start = source_code.begin();

    while (start != source_code.end())
    {
        const auto is_digit = [](const unsigned char ch)
        {
            return isdigit(ch);
        };
        const auto is_identifier_beginning = [](const unsigned char ch)
        {
            return isalpha(ch) || ch == '_';
        };
        const auto is_identifier_ending = [is_identifier_beginning, is_digit](const unsigned char ch)
        {
            return is_identifier_beginning(ch) || isdigit(ch);
        };

        const auto is_space = [](const unsigned char ch)
        {
            return isspace(ch);
        };
        const auto is_punct = [](const unsigned char ch)
        {
            return ispunct(ch);
        };
        const auto is_speech_mark = [](const unsigned char ch)
        {
            return ch == '\"';
        };
        const auto find_or_default = [](const auto& map, const auto& key, const auto& or_else)
        {
            const auto it = map.find(key);
            if (it != map.end())
            {
                return it->second;
            }
            return or_else;
        };

        // skip whitespace
        start = std::find_if_not(start, source_code.end(), is_space);
        if (start == source_code.end())
            break;

        // scan numbers
        if (is_digit(*start))
        {
            const auto end = std::find_if_not(start, source_code.end(), is_digit);
            tokens.emplace_back(Token::Kind::LiteralInteger, std::string{start, end});
            start = end;
            continue;
        }

        // scan a word
        if (is_identifier_beginning(*start))
        {
            const auto end = std::find_if_not(start, source_code.end(), is_identifier_ending);
            const auto word = std::string{start, end};
            const auto kind = find_or_default(reserved_words, word, Token::Kind::Identifier);
            auto& tok = tokens.emplace_back(kind, std::string{start, end});
            // may be a keyword
            start = end;
            continue;
        }

        if (is_speech_mark(*start))
        {
            start = std::next(start);
            const auto end = std::find_if(start, source_code.end(), is_speech_mark);
            if (end == source_code.end())
            {
                throw LexicalError("Unterminated string");
            }
            tokens.emplace_back(Token::Kind::LiteralString, std::string{start, end});
            start = std::next(end);
            continue;
        }

        if (is_punct(*start))
        {
            std::string bestMatch;
            for (auto len = 1; len <= source_code.end() - start; ++len)
            {
                const std::string candidate(start, start + len);
                if (reserved_punctuation.find(candidate) != reserved_punctuation.data.end())
                    bestMatch = candidate;
            }
            if (bestMatch.empty())
                goto unexpected; // NOLINT(*-avoid-goto)
            tokens.emplace_back(reserved_punctuation.at(bestMatch), bestMatch);
            start += bestMatch.size();
            continue;
        }

    unexpected:
        start = std::next(start);
    }

    return tokens;
}