#ifndef CUSTOMHASH_H
#define CUSTOMHASH_H

#include <tuple>
#include <functional>

// Tu dois être dans namespace std pour spécialiser std::hash
template <>
struct std::hash<std::tuple<int, int, int>> {
    size_t operator()(const std::tuple<int, int, int>& t) const noexcept {
        const auto hash1 = std::hash<int>{}(std::get<0>(t));
        const auto hash2 = std::hash<int>{}(std::get<1>(t));
        const auto hash3 = std::hash<int>{}(std::get<2>(t));

        size_t seed = 0;
        seed ^= hash1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};


#endif //CUSTOMHASH_H
