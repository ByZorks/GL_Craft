#ifndef PENDING_LIGHT_H
#define PENDING_LIGHT_H
#include <cstdint>

struct PendingLight {
    int localX = 0;
    int localY = 0;
    int localZ = 0;
    uint8_t lightLevel = 1u;
};

#endif // PENDING_LIGHT_H

