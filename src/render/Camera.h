#ifndef CAMERA_H
#define CAMERA_H
#include "glm.hpp"
#include "../math/Frustum.h"
#include "../world/Chunk.h"
#include "GLFW/glfw3.h"

class Camera {
private:
    bool m_inputEnabled = true;
    float m_lastX, m_lastY;
    float m_yaw, m_pitch, m_lastYaw, m_lastPitch;
    bool m_firstMouse;
    glm::vec3 m_cameraPos, m_cameraFront, m_cameraUp;
    float m_FOVDegrees, m_aspectRatio, m_nearPlane, m_farPlane;
    glm::vec3 m_lastCameraChunkPos = { std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };
    glm::vec3 m_lastCameraBlockPos = { std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };

public:
    Camera(unsigned int windowWidth, unsigned int windowHeight);

    void updateLastState();
    void processInput(GLFWwindow *window, float deltaTime);
    void handleMouse(double xpos, double ypos);
    static void mouseCallback(GLFWwindow *window, double xpos, double ypos);
    void resetMousePosition(GLFWwindow *window);

    [[nodiscard]] float distanceToCamera(const Mesh& mesh) const;
    [[nodiscard]] float distanceToCamera(glm::vec3 position) const;
    [[nodiscard]] bool hasCameraChangedChunk() const;
    [[nodiscard]] bool hasCameraChangedBlock() const;
    [[nodiscard]] bool hasCameraChangedDirection() const;

    [[nodiscard]] glm::mat4 getProjectionMatrix() const ;
    [[nodiscard]] glm::mat4 getViewMatrix() const;
    static Frustum getFrustum(glm::mat4 modelViewProjecMatrix);
    [[nodiscard]] bool m_input_enabled() const;
    void set_m_input_enabled(bool m_input_enabled);
    [[nodiscard]] glm::vec3 m_camera_pos() const;

    void set_m_aspect_ratio(float m_aspect_ratio);
};

#endif //CAMERA_H
