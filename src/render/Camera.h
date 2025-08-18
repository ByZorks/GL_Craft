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
    glm::mat4 m_mvp{};
    glm::vec3 m_lastCameraChunkPos = { std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };
    glm::vec3 m_lastCameraBlockPos = { std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };

public:
    Camera(unsigned int windowWidth, unsigned int windowHeight);

    void updateLastState();
    void processInput(GLFWwindow *window, double deltaTime);
    void handleMouse(double xpos, double ypos);
    void resetMousePosition(GLFWwindow *window);

    [[nodiscard]] float distanceToCamera(const Mesh& mesh) const;
    [[nodiscard]] bool hasCameraChangedChunk() const;
    [[nodiscard]] bool hasCameraChangedBlock() const;
    [[nodiscard]] bool hasCameraChangedDirection() const;
    [[nodiscard]] bool hasCameraUpdated() const;
    [[nodiscard]] bool isUnderWater(int columnHeight) const;

    [[nodiscard]] glm::mat4 getProjectionMatrix() const ;
    [[nodiscard]] glm::mat4 getViewMatrix() const;
    void calculateMVP();
    [[nodiscard]] glm::mat4 getMVP() const;
    [[nodiscard]] const void * getMVPData() const;
    static Frustum getFrustum(glm::mat4 modelViewProjecMatrix);
    [[nodiscard]] bool isInputEnabled() const;
    void setInput(bool m_input_enabled);
    [[nodiscard]] const glm::vec3 & getPos() const;
    [[nodiscard]] const glm::vec3 & getFront() const;

    void setAspectRatio(float m_aspect_ratio);
};

#endif //CAMERA_H
