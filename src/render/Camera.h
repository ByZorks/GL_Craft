#ifndef CAMERA_H
#define CAMERA_H
#include "../world/Chunk.h"
#include "../math/Frustum.h"
#include "GLFW/glfw3.h"
#include "glm.hpp"

class Camera {
private:
    float m_lastX, m_lastY;
    float m_yaw, m_pitch;
    bool m_firstMouse;
    glm::vec3 m_cameraPos, m_cameraFront, m_cameraUp;
    float m_FOVDegrees, m_aspectRatio, m_nearPlane, m_farPlane;

public:
    Camera(unsigned int windowWidth, unsigned int windowHeight);
    void processInput(GLFWwindow *window, float deltaTime);
    void handleMouse(double xpos, double ypos);
    static void mouseCallback(GLFWwindow *window, double xpos, double ypos);
    float distanceToCamera(const Chunk& chunk) const;

    [[nodiscard]] glm::mat4 getProjectionMatrix() const ;
    [[nodiscard]] glm::mat4 getViewMatrix() const;
    static Frustum getFrustum(glm::mat4 modelViewProjecMatrix);
};

#endif //CAMERA_H
