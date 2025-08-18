#include "Frustum.h"

#include <algorithm>

#include "vec3.hpp"

Frustum::Frustum() = default;

Frustum::Frustum(const Plane &left, const Plane &right, const Plane &bottom, const Plane &top, const Plane &near,
                 const Plane &far) : m_planes{left, right, bottom, top, near, far} {
}

bool Frustum::isAABBInFrustum(const AABB &box) const {
    for (const auto& plane : m_planes) {
        if (const glm::vec3 nVertex = box.getNVertex(plane);
            plane.m_a1() * nVertex.x + plane.m_b1() * nVertex.y + plane.m_c1() * nVertex.z + plane.m_d1() < 0)
            return false;
    }

    return true;
}

bool Frustum::isPointInFrustum(const glm::vec3 &point) const {
    return std::ranges::all_of(m_planes, [&](const auto& plane) {
        return plane.m_a1() * point.x + plane.m_b1() * point.y + plane.m_c1() * point.z + plane.m_d1() >= 0;
    });
}
