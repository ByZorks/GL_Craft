#include "Camera.h"

#include <cmath>

#include "../math/Plane.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

Camera::Camera(const unsigned int windowWidth,
               const unsigned int windowHeight) : m_lastX(static_cast<float>(windowWidth) / 2.0f),
                                                  m_lastY(static_cast<float>(windowHeight) / 2.0f),
                                                  m_yaw(-90.0f), m_pitch(0.0f), m_lastYaw(0.f), m_lastPitch(.0f),
                                                  m_firstMouse(true), m_cameraPos(glm::vec3(0.0f, 110, .0f)),
                                                  m_cameraFront(glm::vec3(0.0f, 0.0f, 0.0f)),
                                                  m_cameraUp(glm::vec3(0.0f, 1.0f, 0.0f)),
                                                  m_FOVDegrees(70.f),
                                                  m_aspectRatio(
                                                      static_cast<float>(windowWidth) / static_cast<float>(
                                                          windowHeight)),
                                                  m_nearPlane(.1f), m_farPlane(1024.0f) {
}

void Camera::updateLastState() {
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
}

void Camera::processInput(GLFWwindow *window, const double deltaTime) {
    const float cameraSpeed = 15.0f * static_cast<float>(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        m_cameraPos += cameraSpeed * m_cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_cameraPos -= cameraSpeed * m_cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        m_cameraPos -= glm::normalize(glm::cross(m_cameraFront, m_cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_cameraPos += glm::normalize(glm::cross(m_cameraFront, m_cameraUp)) * cameraSpeed;
}

void Camera::handleMouse(const double xpos, const double ypos) {
    if (m_firstMouse) {
        m_lastX = static_cast<float>(xpos);
        m_lastY = static_cast<float>(ypos);
        m_firstMouse = false;
    }

    double xoffset = xpos - m_lastX;
    double yoffset = m_lastY - ypos;
    m_lastX = static_cast<float>(xpos);
    m_lastY = static_cast<float>(ypos);

    constexpr float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    m_yaw += static_cast<float>(xoffset);
    m_pitch += static_cast<float>(yoffset);

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

float Camera::distanceToCamera(const Mesh& mesh) const {
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
    const float cameraChunkZ = std::floor(m_cameraPos.z / chunkSize);

    if (m_lastCameraChunkPos.x == cameraChunkX &&
        m_lastCameraChunkPos.y == cameraChunkY &&
        m_lastCameraChunkPos.z == cameraChunkZ) {
        return false;
    }

    return true;
}

bool Camera::hasCameraChangedBlock() const {
    const float cameraBlockX = std::floor(m_cameraPos.x);
    const float cameraBlockY = std::floor(m_cameraPos.y);
    const float cameraBlockZ = std::floor(m_cameraPos.z);

    if (m_lastCameraBlockPos.x == cameraBlockX &&
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
    constexpr int waterLevel = 63;
    return m_cameraPos.y > static_cast<float>(columnHeight) && m_cameraPos.y <= waterLevel;

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

const void * Camera::getMVPData() const {
    return  &m_mvp[0][0];
}

Frustum Camera::getFrustum(glm::mat4 modelViewProjecMatrix) {
    constexpr float padding = Chunk::SIZE * 1.7f; // Prevent popping

    auto left = Plane(modelViewProjecMatrix[0][3] + modelViewProjecMatrix[0][0],
                       modelViewProjecMatrix[1][3] + modelViewProjecMatrix[1][0],
                       modelViewProjecMatrix[2][3] + modelViewProjecMatrix[2][0],
                       modelViewProjecMatrix[3][3] + modelViewProjecMatrix[3][0]
    );
    left.setD(left.m_d1() + padding);

    auto right = Plane(modelViewProjecMatrix[0][3] - modelViewProjecMatrix[0][0],
                        modelViewProjecMatrix[1][3] - modelViewProjecMatrix[1][0],
                        modelViewProjecMatrix[2][3] - modelViewProjecMatrix[2][0],
                        modelViewProjecMatrix[3][3] - modelViewProjecMatrix[3][0]
    );
    right.setD(right.m_d1() + padding);

    auto bottom = Plane(modelViewProjecMatrix[0][3] + modelViewProjecMatrix[0][1],
                         modelViewProjecMatrix[1][3] + modelViewProjecMatrix[1][1],
                         modelViewProjecMatrix[2][3] + modelViewProjecMatrix[2][1],
                         modelViewProjecMatrix[3][3] + modelViewProjecMatrix[3][1]
    );
    bottom.setD(bottom.m_d1() + padding);

    auto top = Plane(modelViewProjecMatrix[0][3] - modelViewProjecMatrix[0][1],
                      modelViewProjecMatrix[1][3] - modelViewProjecMatrix[1][1],
                      modelViewProjecMatrix[2][3] - modelViewProjecMatrix[2][1],
                      modelViewProjecMatrix[3][3] - modelViewProjecMatrix[3][1]
    );
    top.setD(top.m_d1() + padding);

    auto near = Plane(modelViewProjecMatrix[0][3] + modelViewProjecMatrix[0][2],
                       modelViewProjecMatrix[1][3] + modelViewProjecMatrix[1][2],
                       modelViewProjecMatrix[2][3] + modelViewProjecMatrix[2][2],
                       modelViewProjecMatrix[3][3] + modelViewProjecMatrix[3][2]
    );
    near.setD(near.m_d1() + padding);

    const auto far = Plane(modelViewProjecMatrix[0][3] - modelViewProjecMatrix[0][2],
                      modelViewProjecMatrix[1][3] - modelViewProjecMatrix[1][2],
                      modelViewProjecMatrix[2][3] - modelViewProjecMatrix[2][2],
                      modelViewProjecMatrix[3][3] - modelViewProjecMatrix[3][2]
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

const glm::vec3 & Camera::getPos() const {
    return m_cameraPos;
}

const glm::vec3 & Camera::getFront() const {
    return m_cameraFront;
}

void Camera::setAspectRatio(const float m_aspect_ratio) {
    m_aspectRatio = m_aspect_ratio;
}
