//
// Created by Jamie on 27/01/2026.
//

#ifndef CYREXC_CMAP_HPP
#define CYREXC_CMAP_HPP

#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

#include <cstdint>


namespace cyrex::util
{

// A map that works at compile time
template <typename Key, typename Value, std::size_t Size>
struct CompileTimeMap
{
    std::array<std::pair<Key, Value>, Size> data;

    [[nodiscard]] constexpr Value at(const Key& key) const
    {
        const auto iterator = find(key);

        if (iterator != std::end(data))
        {
            return iterator->second;
        }
        throw std::range_error("Key out of range");
    }

    [[nodiscard]] constexpr auto find(const Key& key) const
    {
        const auto iterator = std::find_if(std::begin(data),
                                           std::end(data),
                                           [&key](const auto& value) { return value.first == key; });
        return iterator;
    }

    [[nodiscard]] constexpr auto begin()
    {
        return data.begin();
    }

    [[nodiscard]] constexpr auto end()
    {
        return data.end();
    }

    [[nodiscard]] constexpr auto begin() const
    {
        return data.begin();
    }

    [[nodiscard]] constexpr auto end() const
    {
        return data.end();
    }

    [[nodiscard]] constexpr CompileTimeMap sorted() const
    {
        auto sorted_data = data;
        std::sort(std::begin(sorted_data), std::end(sorted_data));
        return {sorted_data};
    }
};

} // namespace cyrex::util

#endif //CYREXC_CMAP_HPP
