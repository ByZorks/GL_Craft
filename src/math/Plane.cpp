#include "Plane.h"

#include <cmath>
#include <stdexcept>

Plane::Plane() : m_a(0.0f), m_b(0.0f), m_c(0.0f), m_d(0.0f) {
}

Plane::Plane(const float a, const float b, const float c, const float d) : m_a(a), m_b(b), m_c(c), m_d(d) {
    normalize();
}

void Plane::normalize() {
    const float length = std::sqrt(m_a * m_a + m_b * m_b + m_c * m_c);
    if (length == 0.0f) throw std::runtime_error("Cannot normalize a plane with zero length.");

    m_a /= length;
    m_b /= length;
    m_c /= length;
    m_d /= length;
}

float Plane::m_a1() const {
    return m_a;
}

float Plane::m_b1() const {
    return m_b;
}

float Plane::m_c1() const {
    return m_c;
}

float Plane::m_d1() const {
    return m_d;
}

void Plane::setD(const float m_d) {
    this->m_d = m_d;
}
