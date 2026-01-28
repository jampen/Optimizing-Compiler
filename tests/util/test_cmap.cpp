//
// Created by Jamie on 27/01/2026.
//

#include <gtest/gtest.h>
#include <util/cmap.hpp>


TEST(CompileTimeMap, ThrowsRangeError)
{
    constexpr auto values = std::array<std::pair<int, char>, 4>{
        {{1, 'a'}, {2, 'b'}, {3, 'c'}},
    };
    constexpr auto map = cyrex::util::CompileTimeMap{values};
    ASSERT_THROW(map.at(-1), std::range_error);
}

TEST(CompileTimeMap, FindsItem)
{
    constexpr auto values = std::array<std::pair<int, char>, 4>{
        {{1, 'a'}, {2, 'b'}, {3, 'c'}},
    };
    constexpr auto map = cyrex::util::CompileTimeMap{values};
    ASSERT_EQ(map.at(2), 'b');
}

TEST(CompileTimeMap, FindReturnsEndOfData)
{
    constexpr auto values = std::array<std::pair<int, char>, 4>{
        {{1, 'a'}, {2, 'b'}, {3, 'c'}},
    };
    constexpr auto map = cyrex::util::CompileTimeMap{values};
    ASSERT_TRUE(map.find(-1) == map.data.end());
}
