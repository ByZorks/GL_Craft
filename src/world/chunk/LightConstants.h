#ifndef GL_CRAFT_LIGHTCONSTANTS_H
#define GL_CRAFT_LIGHTCONSTANTS_H

#include <cstdint>

namespace LightConstants {

// Global limits
constexpr uint8_t SUN_MIN = 1u;
constexpr uint8_t SUN_MAX = 15u;   // 4 bits
constexpr uint8_t RGB_MIN = 0u;
constexpr uint8_t RGB_MAX = 15u;   // 4 bits

// Bit layout in a 16-bit light cell:
// bits  0..3   -> sunlight (4 bits)
// bits  4..7   -> red   (4 bits)
// bits  8..11  -> green (4 bits)
// bits 12..15  -> blue  (4 bits)
constexpr uint16_t SUN_MASK = 0x000Fu;
constexpr uint16_t R_MASK   = 0x00F0u;
constexpr uint16_t G_MASK   = 0x0F00u;
constexpr uint16_t B_MASK   = 0xF000u;
constexpr uint16_t ONE_CHANNEL_MASK = 0x0Fu;

constexpr int R_SHIFT = 4;
constexpr int G_SHIFT = 8;
constexpr int B_SHIFT = 12;

// Helpers
constexpr uint16_t packSun(const uint8_t sun) noexcept {
    return static_cast<uint16_t>(sun & ONE_CHANNEL_MASK);
}
constexpr uint16_t packRGB(const uint8_t r, const uint8_t g, const uint8_t b) noexcept {
    return static_cast<uint16_t>(
        static_cast<uint16_t>(r & ONE_CHANNEL_MASK) << R_SHIFT |
        static_cast<uint16_t>(g & ONE_CHANNEL_MASK) << G_SHIFT |
        static_cast<uint16_t>(b & ONE_CHANNEL_MASK) << B_SHIFT
    );
}
constexpr uint16_t packAll(const uint8_t sun, const uint8_t r, const uint8_t g, const uint8_t b) noexcept {
    return packSun(sun) | packRGB(r,g,b);
}

constexpr uint8_t unpackSun(const uint16_t packed) noexcept {
    return static_cast<uint8_t>(packed & SUN_MASK);
}
constexpr uint8_t unpackR(const uint16_t packed) noexcept {
    return static_cast<uint8_t>((packed & R_MASK) >> R_SHIFT);
}
constexpr uint8_t unpackG(const uint16_t packed) noexcept {
    return static_cast<uint8_t>((packed & G_MASK) >> G_SHIFT);
}
constexpr uint8_t unpackB(const uint16_t packed) noexcept {
    return static_cast<uint8_t>((packed & B_MASK) >> B_SHIFT);
}

} // namespace LightConstants

#endif //GL_CRAFT_LIGHTCONSTANTS_H