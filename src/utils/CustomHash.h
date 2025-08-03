#ifndef CUSTOMHASH_H
#define CUSTOMHASH_H

#include <tuple>
#include <functional>

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

template <>
struct std::hash<std::pair<int, int>> {
    size_t operator()(const std::pair<int, int>& t) const noexcept {
        const auto hash1 = std::hash<int>{}(std::get<0>(t));
        const auto hash2 = std::hash<int>{}(std::get<1>(t));

        size_t seed = 0;
        seed ^= hash1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

template <>
struct std::hash<glm::vec3> {
    std::size_t operator()(const glm::vec3& v) const noexcept {
        const std::size_t h1 = std::hash<float>{}(v.x);
        const std::size_t h2 = std::hash<float>{}(v.y);
        const std::size_t h3 = std::hash<float>{}(v.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};


#endif //CUSTOMHASH_H
