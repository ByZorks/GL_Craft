#include "AABB.h"

AABB::AABB(const float xMin, const float yMin, const float zMin, const float xMax, const float yMax, const float zMax)
    : m_corners{
        {glm::vec3(xMin, yMin, zMin)},
        {glm::vec3(xMax, yMin, zMin)},
        {glm::vec3(xMax, yMax, zMin)},
        {glm::vec3(xMin, yMax, zMin)},
        {glm::vec3(xMin, yMin, zMax)},
        {glm::vec3(xMax, yMin, zMax)},
        {glm::vec3(xMax, yMax, zMax)},
        {glm::vec3(xMin, yMax, zMax)}
    } {
}

glm::vec3 AABB::getNVertex(const Plane &plane) const {
    return {
        plane.m_a1() < 0 ? m_corners[1].x : m_corners[0].x,
        plane.m_b1() < 0 ? m_corners[2].y : m_corners[0].y,
        plane.m_c1() < 0 ? m_corners[4].z : m_corners[0].z
    };
}
