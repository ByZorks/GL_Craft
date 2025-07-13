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

AABB::~AABB() = default;

glm::vec3 AABB::getCorner(const int index) const {
    return m_corners[index];
}
