#ifndef CAMERA_H
#define CAMERA_H
#include <memory>
#include <unordered_map>

#include "../math/Frustum.h"
#include "../world/chunk/Chunk.h"
#include "../world/chunk/ChunkPosition.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

class Camera {
public:
    Camera(unsigned int windowWidth, unsigned int windowHeight);

    void updateLastState(double deltaTime, bool applyGravity);
    void updateLastState(double deltaTime, bool applyGravity,
                                     const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>>& loadedChunks);
    void processInput(const std::shared_ptr<GLFWwindow> &window, double deltaTime);
    void processInput(const std::shared_ptr<GLFWwindow> &window, double deltaTime, bool isGravityEnabled,
                                   const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>>& loadedChunks);
    void handleMouse(double xPos, double yPos);
    void resetMousePosition(GLFWwindow *window);
    void calculateMVP();

    [[nodiscard]] float distanceToCamera(const Mesh &mesh) const;
    [[nodiscard]] bool hasCameraChangedChunk() const;
    [[nodiscard]] bool hasCameraChangedBlock() const;
    [[nodiscard]] bool hasCameraChangedDirection() const;
    [[nodiscard]] bool hasCameraUpdated() const;
    [[nodiscard]] bool isUnderWater(int columnHeight) const;
    [[nodiscard]] glm::mat4 getProjectionMatrix() const;
    [[nodiscard]] glm::mat4 getViewMatrix() const;
    [[nodiscard]] glm::mat4 getMVP() const;
    [[nodiscard]] const void *getMVPData() const;
    static Frustum getFrustum(glm::mat4 mvp);
    [[nodiscard]] const glm::vec3 &getPos() const;
    [[nodiscard]] bool isInputEnabled() const;
    [[nodiscard]] const glm::vec3 &getFront() const;

    void setInput(bool m_input_enabled);
    void setAspectRatio(float m_aspect_ratio);

private:
    bool m_inputEnabled = true;
    float m_lastX, m_lastY;
    float m_yaw, m_pitch, m_lastYaw, m_lastPitch;
    bool m_firstMouse;
    glm::vec3 m_cameraPos, m_cameraFront, m_cameraUp;
    float m_FOVDegrees, m_aspectRatio, m_nearPlane, m_farPlane;
    glm::mat4 m_mvp{};
    glm::vec3 m_lastCameraChunkPos = {
        std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), std::numeric_limits<int>::max()
    };
    glm::vec3 m_lastCameraBlockPos = {
        std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), std::numeric_limits<int>::max()
    };
    glm::vec3 m_playerSize = glm::vec3(0.6f, 1.8f, 0.6f);

};

#endif //CAMERA_H
