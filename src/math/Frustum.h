#ifndef FRUSTUM_H
#define FRUSTUM_H
#include <array>

#include "AABB.h"
#include "Plane.h"

class Frustum {
public:
    Frustum();
    Frustum(const Plane& left, const Plane& right, const Plane &bottom, const Plane &top, const Plane &near, const Plane& far);

    [[nodiscard]] bool isAABBInFrustum(const AABB& box) const;
    [[nodiscard]] bool isPointInFrustum(const glm::vec3& point) const;

private:
    std::array<Plane, 6> m_planes;
};

#endif //FRUSTUM_H
