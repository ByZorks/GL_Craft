#include "AABB.h"

AABB::AABB(const float xMin, const float yMin, const float zMin, const float xMax, const float yMax, const float zMax)
    : m_min(xMin, yMin, zMin), m_max(xMax, yMax, zMax) {
}

AABB::AABB(const glm::vec3& center, const glm::vec3& halfExtents) : m_min(center - halfExtents), m_max(center + halfExtents) {
}

glm::vec3 AABB::getNVertex(const Plane &plane) const {
    return {
        plane.getA() < 0 ? m_max.x : m_min.x,
        plane.getB() < 0 ? m_max.y : m_min.y,
        plane.getC() < 0 ? m_max.z : m_min.z
    };
}

const glm::vec3 & AABB::getMin() const {
    return m_min;
}

const glm::vec3 & AABB::getMax() const {
    return m_max;
}
