#include "Frustum.h"

#include <algorithm>
#include <ranges>

#include "vec3.hpp"

Frustum::Frustum(const Plane &left, const Plane &right, const Plane &bottom, const Plane &top, const Plane &near,
                 const Plane &far) : m_planes{left, right, bottom, top, near, far} {
}

Frustum::~Frustum() = default;

bool Frustum::isAABBInFrustum(const AABB &box) const {
    for (const auto& plane : m_planes) {
        int out = 0;
        for (int i = 0; i < 8; ++i) {
            const glm::vec3 corner = box.getCorner(i);
            if (const float distance = plane.m_a1() * corner.x + plane.m_b1() * corner.y + plane.m_c1() * corner.z + plane.m_d1(); distance < 0) out++;
        }
        if (out == 8) return false; // All corners are outside the plane
    }

    return true;
}

bool Frustum::isPointInFrustum(const glm::vec3 &point) const {
    return std::ranges::all_of(m_planes, [&](const auto& plane) {
        return plane.m_a1() * point.x + plane.m_b1() * point.y + plane.m_c1() * point.z + plane.m_d1() >= 0;
    });
}
