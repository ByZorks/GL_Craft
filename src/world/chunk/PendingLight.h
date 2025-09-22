#ifndef PENDING_LIGHT_H
#define PENDING_LIGHT_H
#include <cstdint>

struct PendingLight {
    const int localX, localY, localZ;
    const RGBLight blockLight;
    const uint8_t sunlight;
};

#endif // PENDING_LIGHT_H

