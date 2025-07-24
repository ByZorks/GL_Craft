#ifndef FRUSTUM_H
#define FRUSTUM_H
#include "AABB.h"
#include "Plane.h"

class Frustum {
private:
    Plane m_planes[6];

public:
    Frustum(const Plane& left, const Plane& right, const Plane &bottom, const Plane &top, const Plane &near, const Plane& far);
    ~Frustum();

    bool isAABBInFrustum(const AABB& box) const;
};

#endif //FRUSTUM_H
