//
// Created by Jamie on 27/01/2026.
//

#ifndef CYREXC_RESERVED_WORDS_HPP
#define CYREXC_RESERVED_WORDS_HPP

#include <frontend/tokens.hpp>
#include <util/cmap.hpp>

namespace cyrex
{

constexpr auto make_reserved_words()
{
    using namespace std::string_view_literals;
    using enum Token::Kind;

    constexpr auto values = std::array{std::pair{"function"sv, Function},
                                       std::pair{"const"sv, Const},
                                       std::pair{"var"sv, Var},
                                       std::pair{"if"sv, If},
                                       std::pair{"else"sv, Else},
                                       std::pair{"while"sv, While},
                                       std::pair{"do"sv, Do},
                                       std::pair{"then"sv, Then},
                                       std::pair{"return"sv, Return},

                                       // Bitwise logical operators
                                       std::pair{"s8"sv, S8},
                                       std::pair{"s16"sv, S16},
                                       std::pair{"s32"sv, S32},
                                       std::pair{"s64"sv, S64},
                                       std::pair{"u8"sv, U8},
                                       std::pair{"u16"sv, U16},
                                       std::pair{"u32"sv, U32},
                                       std::pair{"u64"sv, U64}};
    return util::CompileTimeMap(values).sorted();
}

constexpr auto make_reserved_punctuation()
{
    using namespace std::string_view_literals;
    using enum Token::Kind;

    constexpr auto values = std::array{
        // Math operators
        std::pair{"+"sv, Plus},
        std::pair{"-"sv, Minus},
        std::pair{"*"sv, Star},
        std::pair{"/"sv, Slash},

        // Punctuation
        std::pair{"("sv, LeftParenthesis},
        std::pair{")"sv, RightParenthesis},
        std::pair{"["sv, LeftSquareBracket},
        std::pair{"]"sv, RightSquareBracket},
        std::pair{"{"sv, LeftCurlyBracket},
        std::pair{"}"sv, RightCurlyBracket},
        std::pair{","sv, Comma},
        std::pair{":"sv, Colon},
        std::pair{"="sv, Assign},

        // Bitwise logical operators
        std::pair{"!"sv, Not},
        std::pair{"^"sv, ExclusiveOr},
        std::pair{"|"sv, Or},
        std::pair{"&"sv, And},

        // Comparative operators
        std::pair{"<"sv, LessThan},
        std::pair{"<="sv, LessThanEqualTo},
        std::pair{">"sv, GreaterThan},
        std::pair{">="sv, GreaterThanEqualTo},
        std::pair{"=="sv, EqualTo},
        std::pair{"!="sv, NotEqualTo},
    };

    return util::CompileTimeMap(values).sorted();
}

} // namespace cyrex

#endif //CYREXC_RESERVED_WORDS_HPP
