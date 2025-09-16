#ifndef GL_CRAFT_RGBLIGHT_H
#define GL_CRAFT_RGBLIGHT_H
#include <algorithm>
#include <cstdint>

struct RGBLight {
    uint8_t r, g, b;

    [[nodiscard]] uint8_t getEffectiveLevel() const {
        // Perceived brightness by wikipedia
        return static_cast<uint8_t>(
            0.2126f * static_cast<float>(r) + 0.7152f * static_cast<float>(g) + 0.0722f * static_cast<float>(b));
    }

    [[nodiscard]] bool shouldPropagate(const uint8_t minLevel = 1u) const {
        return r > minLevel + 1u || g > minLevel + 1u || b > minLevel + 1u;
    }

    [[nodiscard]] RGBLight attenuated() const {
        return {
            static_cast<uint8_t>(std::max(1, static_cast<int>(r) - 1)),
            static_cast<uint8_t>(std::max(1, static_cast<int>(g) - 1)),
            static_cast<uint8_t>(std::max(1, static_cast<int>(b) - 1))
        };
    }
};

#endif //GL_CRAFT_RGBLIGHT_H