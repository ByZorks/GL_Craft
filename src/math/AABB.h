#ifndef AABB_H
#define AABB_H
#include "Plane.h"
#include "glm/vec3.hpp"

class AABB {
public:
    AABB(float xMin, float yMin, float zMin, float xMax, float yMax, float zMax);
    AABB(const glm::vec3 &center, const glm::vec3 &halfExtents);

    [[nodiscard]] glm::vec3 getNVertex(const Plane &plane) const;
    [[nodiscard]] const glm::vec3 & getMin() const;
    [[nodiscard]] const glm::vec3 & getMax() const;

private:
    glm::vec3 m_min, m_max;
};

#endif //AABB_H
