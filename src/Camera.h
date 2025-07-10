#ifndef CAMERA_H
#define CAMERA_H
#include "vec3.hpp"
#include "GLFW/glfw3.h"

class Camera {
private:
    float m_lastX, m_lastY;
    float m_yaw, m_pitch;
    bool m_firstMouse;
    glm::vec3 m_cameraPos, m_cameraFront, m_cameraUp;

public:
    Camera(unsigned int windowWidth, unsigned int windowHeight);
    void processInput(GLFWwindow *window, float deltaTime);
    void handleMouse(double xpos, double ypos);
    static void mouseCallback(GLFWwindow *window, double xpos, double ypos);

    [[nodiscard]] glm::vec3 m_camera_pos() const;
    [[nodiscard]] glm::vec3 m_camera_front() const;
    [[nodiscard]] glm::vec3 m_camera_up() const;
};

#endif //CAMERA_H
