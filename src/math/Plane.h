#ifndef PLANE_H
#define PLANE_H

class Plane {
public:
    Plane();
    Plane(float a, float b, float c, float d);

    [[nodiscard]] float m_a1() const;
    [[nodiscard]] float m_b1() const;
    [[nodiscard]] float m_c1() const;
    [[nodiscard]] float m_d1() const;

    void setD(float m_d);
private:
    void normalize();

private:
    float m_a, m_b, m_c, m_d;
};

#endif //PLANE_H
