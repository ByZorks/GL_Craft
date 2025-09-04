#ifndef AABB_H
#define AABB_H
#include "Plane.h"
#include "glm/vec3.hpp"

class AABB {
public:
    AABB(float xMin, float yMin, float zMin, float xMax, float yMax, float zMax);

    [[nodiscard]] glm::vec3 getNVertex(const Plane &plane) const;

private:
    glm::vec3 m_corners[8];
};

#endif //AABB_H
