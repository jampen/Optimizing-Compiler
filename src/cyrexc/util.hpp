#pragma once

template <typename Map, typename Key, typename Default>
auto find_or_default(const Map& m, const Key& k, Default d)
{
    if (auto it = m.find(k); it != m.end())
        return it->second;
    return d;
}
