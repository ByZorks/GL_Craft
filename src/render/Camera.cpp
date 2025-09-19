#include "Camera.h"

#include <cmath>

#include "../math/Plane.h"
#include "../world/TerrainGenerator.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

Camera::Camera(const unsigned int windowWidth,
               const unsigned int windowHeight) : m_lastX(static_cast<float>(windowWidth) / 2.0f),
                                                  m_lastY(static_cast<float>(windowHeight) / 2.0f),
                                                  m_yaw(-90.0f), m_pitch(0.0f), m_lastYaw(0.f), m_lastPitch(.0f),
                                                  m_firstMouse(true), m_cameraPos(.0f, 150, .0f),
                                                  m_cameraFront(0.0f, 0.0f, 0.0f),
                                                  m_cameraUp(0.0f, 1.0f, 0.0f), m_FOVDegrees(70.f),
                                                  m_aspectRatio(
                                                      static_cast<float>(windowWidth) / static_cast<float>(
                                                          windowHeight)),
                                                  m_nearPlane(.1f), m_farPlane(2048.0f) {
}

void Camera::updateLastState(const double deltaTime, const bool applyGravity) {
    constexpr auto chunkSize = static_cast<float>(Chunk::SIZE);
    const float cameraChunkX = std::floor(m_cameraPos.x / chunkSize);
    const float cameraChunkY = std::floor(m_cameraPos.y / chunkSize);
    const float cameraChunkZ = std::floor(m_cameraPos.z / chunkSize);
    m_lastCameraChunkPos = {cameraChunkX, cameraChunkY, cameraChunkZ};

    const float cameraBlockX = std::floor(m_cameraPos.x);
    const float cameraBlockY = std::floor(m_cameraPos.y);
    const float cameraBlockZ = std::floor(m_cameraPos.z);
    m_lastCameraBlockPos = {cameraBlockX, cameraBlockY, cameraBlockZ};

    if (std::abs(m_lastYaw - m_yaw) > 15.f) m_lastYaw = m_yaw;
    if (std::abs(m_lastPitch - m_pitch) > 15.f) m_lastPitch = m_pitch;

    if (applyGravity) m_cameraPos.y -= 20.f * static_cast<float>(deltaTime);
}

void Camera::processInput(const std::shared_ptr<GLFWwindow> &window, const double deltaTime) {
    const float cameraSpeed = 50.0f * static_cast<float>(deltaTime);
    if (glfwGetKey(window.get(), GLFW_KEY_W) == GLFW_PRESS)
        m_cameraPos += cameraSpeed * m_cameraFront;
    if (glfwGetKey(window.get(), GLFW_KEY_S) == GLFW_PRESS)
        m_cameraPos -= cameraSpeed * m_cameraFront;
    if (glfwGetKey(window.get(), GLFW_KEY_A) == GLFW_PRESS)
        m_cameraPos -= glm::normalize(glm::cross(m_cameraFront, m_cameraUp)) * cameraSpeed;
    if (glfwGetKey(window.get(), GLFW_KEY_D) == GLFW_PRESS)
        m_cameraPos += glm::normalize(glm::cross(m_cameraFront, m_cameraUp)) * cameraSpeed;
}

void Camera::handleMouse(const double xPos, const double yPos) {
    if (m_firstMouse) {
        m_lastX = static_cast<float>(xPos);
        m_lastY = static_cast<float>(yPos);
        m_firstMouse = false;
    }

    double xOffset = xPos - m_lastX;
    double yOffset = m_lastY - yPos;
    m_lastX = static_cast<float>(xPos);
    m_lastY = static_cast<float>(yPos);

    constexpr float sensitivity = 0.1f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    m_yaw += static_cast<float>(xOffset);
    m_pitch += static_cast<float>(yOffset);

    if (m_pitch > 89.0f)
        m_pitch = 89.0f;
    if (m_pitch < -89.0f)
        m_pitch = -89.0f;

    glm::vec3 direction;
    direction.x = glm::cos(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch));
    direction.y = glm::sin(glm::radians(m_pitch));
    direction.z = glm::sin(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch));
    m_cameraFront = glm::normalize(direction);
}

float Camera::distanceToCamera(const Mesh &mesh) const {
    const auto x = static_cast<float>(mesh.getX());
    const auto y = static_cast<float>(mesh.getY());
    const auto z = static_cast<float>(mesh.getZ());
    constexpr auto size = static_cast<float>(Chunk::SIZE) * 0.5f;
    const glm::vec3 middle(x + size, y + size, z + size);

    return glm::distance(m_cameraPos, middle);
}

void Camera::resetMousePosition(GLFWwindow *window) {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    m_lastX = static_cast<float>(width) / 2.0f;
    m_lastY = static_cast<float>(height) / 2.0f;
    m_firstMouse = true;
}

bool Camera::hasCameraChangedChunk() const {
    // Calculate which chunk the camera is in
    constexpr auto chunkSize = static_cast<float>(Chunk::SIZE);
    const float cameraChunkX = std::floor(m_cameraPos.x / chunkSize);
    const float cameraChunkY = std::floor(m_cameraPos.y / chunkSize);

    if (const float cameraChunkZ = std::floor(m_cameraPos.z / chunkSize);
        m_lastCameraChunkPos.x == cameraChunkX &&
        m_lastCameraChunkPos.y == cameraChunkY &&
        m_lastCameraChunkPos.z == cameraChunkZ) {
        return false;
    }

    return true;
}

bool Camera::hasCameraChangedBlock() const {
    const float cameraBlockX = std::floor(m_cameraPos.x);
    const float cameraBlockY = std::floor(m_cameraPos.y);

    if (const float cameraBlockZ = std::floor(m_cameraPos.z);
        m_lastCameraBlockPos.x == cameraBlockX &&
        m_lastCameraBlockPos.y == cameraBlockY &&
        m_lastCameraBlockPos.z == cameraBlockZ) {
        return false;
    }

    return true;
}

bool Camera::hasCameraChangedDirection() const {
    return std::abs(m_lastYaw - m_yaw) > 15.f || std::abs(m_lastPitch - m_pitch) > 15.f;
}

bool Camera::hasCameraUpdated() const {
    return hasCameraChangedBlock() || hasCameraChangedDirection();
}

bool Camera::isUnderWater(const int columnHeight) const {
    return m_cameraPos.y > static_cast<float>(columnHeight) && m_cameraPos.y <= static_cast<float>(
               TerrainGenerator::getSeaLevel());
}

glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(glm::radians(m_FOVDegrees), m_aspectRatio, m_nearPlane, m_farPlane);
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_cameraUp);
}

void Camera::calculateMVP() {
    m_mvp = getProjectionMatrix() * getViewMatrix();
}

glm::mat4 Camera::getMVP() const {
    return m_mvp;
}

const void *Camera::getMVPData() const {
    return &m_mvp[0][0];
}

Frustum Camera::getFrustum(glm::mat4 mvp) {
    constexpr float padding = Chunk::SIZE * 1.7f; // Prevent popping

    auto left = Plane(mvp[0][3] + mvp[0][0],
                      mvp[1][3] + mvp[1][0],
                      mvp[2][3] + mvp[2][0],
                      mvp[3][3] + mvp[3][0]
    );
    left.setD(left.getD() + padding);

    auto right = Plane(mvp[0][3] - mvp[0][0],
                       mvp[1][3] - mvp[1][0],
                       mvp[2][3] - mvp[2][0],
                       mvp[3][3] - mvp[3][0]
    );
    right.setD(right.getD() + padding);

    auto bottom = Plane(mvp[0][3] + mvp[0][1],
                        mvp[1][3] + mvp[1][1],
                        mvp[2][3] + mvp[2][1],
                        mvp[3][3] + mvp[3][1]
    );
    bottom.setD(bottom.getD() + padding);

    auto top = Plane(mvp[0][3] - mvp[0][1],
                     mvp[1][3] - mvp[1][1],
                     mvp[2][3] - mvp[2][1],
                     mvp[3][3] - mvp[3][1]
    );
    top.setD(top.getD() + padding);

    auto near = Plane(mvp[0][3] + mvp[0][2],
                      mvp[1][3] + mvp[1][2],
                      mvp[2][3] + mvp[2][2],
                      mvp[3][3] + mvp[3][2]
    );
    near.setD(near.getD() + padding);

    const auto far = Plane(mvp[0][3] - mvp[0][2],
                           mvp[1][3] - mvp[1][2],
                           mvp[2][3] - mvp[2][2],
                           mvp[3][3] - mvp[3][2]
    );

    const Frustum frustum(left, right, bottom, top, near, far);
    return frustum;
}

bool Camera::isInputEnabled() const {
    return m_inputEnabled;
}

void Camera::setInput(const bool m_input_enabled) {
    this->m_inputEnabled = m_input_enabled;
}

const glm::vec3 &Camera::getPos() const {
    return m_cameraPos;
}

const glm::vec3 &Camera::getFront() const {
    return m_cameraFront;
}

void Camera::setAspectRatio(const float m_aspect_ratio) {
    m_aspectRatio = m_aspect_ratio;
}
