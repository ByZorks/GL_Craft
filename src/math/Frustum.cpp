#include "Frustum.h"

#include <algorithm>

#include "glm/vec3.hpp"

Frustum::Frustum() = default;

Frustum::Frustum(const Plane &left, const Plane &right, const Plane &bottom, const Plane &top, const Plane &near,
                 const Plane &far) : m_planes{left, right, bottom, top, near, far} {
}

bool Frustum::isAABBInFrustum(const AABB &box) const {
    if (!std::ranges::all_of(m_planes, [&](const auto &plane) {
        const glm::vec3 nVertex = box.getNVertex(plane);
        return plane.getA() * nVertex.x + plane.getB() * nVertex.y + plane.getC() * nVertex.z + plane.getD() >= 0;
    }))
        return false;

    return true;
}

bool Frustum::isPointInFrustum(const glm::vec3 &point) const {
    return std::ranges::all_of(m_planes, [&](const auto &plane) {
        return plane.getA() * point.x + plane.getB() * point.y + plane.getC() * point.z + plane.getD() >= 0;
    });
}
