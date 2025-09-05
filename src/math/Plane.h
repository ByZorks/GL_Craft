#ifndef PLANE_H
#define PLANE_H

class Plane {
public:
    Plane();
    Plane(float a, float b, float c, float d);

    [[nodiscard]] float getA() const;
    [[nodiscard]] float getB() const;
    [[nodiscard]] float getC() const;
    [[nodiscard]] float getD() const;

    void setD(float m_d);
private:
    void normalize();

private:
    float m_a, m_b, m_c, m_d;
};

#endif //PLANE_H
