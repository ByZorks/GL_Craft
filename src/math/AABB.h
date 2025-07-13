#ifndef AABB_H
#define AABB_H
#include "vec3.hpp"

class AABB {
private:
    glm::vec3 m_corners[8];

public:
    AABB(float xMin, float yMin, float zMin, float xMax, float yMax, float zMax);
    ~AABB();

    [[nodiscard]] glm::vec3 getCorner(int index) const;

};

#endif //AABB_H
