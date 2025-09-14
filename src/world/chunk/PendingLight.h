#ifndef PENDING_LIGHT_H
#define PENDING_LIGHT_H
#include <cstdint>

struct PendingLight {
    int localX, localY, localZ;
    RGBLight blockLight;
    uint8_t sunlight;
};

#endif // PENDING_LIGHT_H

