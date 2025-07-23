#ifndef CAMERA_H
#define CAMERA_H
#include "../world/Chunk.h"
#include "../math/Frustum.h"
#include "GLFW/glfw3.h"
#include "glm.hpp"

class Camera {
private:
    bool m_inputEnabled = true;
    float m_lastX, m_lastY;
    float m_yaw, m_pitch;
    bool m_firstMouse;
    glm::vec3 m_cameraPos, m_cameraFront, m_cameraUp;
    float m_FOVDegrees, m_aspectRatio, m_nearPlane, m_farPlane;
    glm::vec3 m_lastCameraChunkPos = { std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };

public:
    Camera(unsigned int windowWidth, unsigned int windowHeight);
    void processInput(GLFWwindow *window, float deltaTime);
    void handleMouse(double xpos, double ypos);
    static void mouseCallback(GLFWwindow *window, double xpos, double ypos);
    [[nodiscard]] float distanceToCamera(const Chunk& chunk) const;
    void resetMousePosition(GLFWwindow *window);
    bool hasCameraChangedChunk();

    [[nodiscard]] glm::mat4 getProjectionMatrix() const ;
    [[nodiscard]] glm::mat4 getViewMatrix() const;
    static Frustum getFrustum(glm::mat4 modelViewProjecMatrix);
    [[nodiscard]] bool m_input_enabled() const;
    void set_m_input_enabled(bool m_input_enabled);
    [[nodiscard]] glm::vec3 m_camera_pos() const;
};

#endif //CAMERA_H
