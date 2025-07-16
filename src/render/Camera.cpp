#include "Camera.h"

#include <cmath>

#include "../math/Plane.h"
#include "ext/matrix_clip_space.hpp"
#include "ext/matrix_transform.hpp"

Camera::Camera(const unsigned int windowWidth,
               const unsigned int windowHeight) : m_lastX(static_cast<float>(windowWidth) / 2.0f),
                                                  m_lastY(static_cast<float>(windowHeight) / 2.0f),
                                                  m_yaw(-90.0f), m_pitch(0.0f),
                                                  m_firstMouse(true), m_cameraPos(glm::vec3(0.0f, 3 * 16, .0f)),
                                                  m_cameraFront(glm::vec3(0.0f, 0.0f, -1.0f)),
                                                  m_cameraUp(glm::vec3(0.0f, 1.0f, 0.0f)),
                                                  m_FOVDegrees(45.f),
                                                  m_aspectRatio(
                                                      static_cast<float>(windowWidth) / static_cast<float>(
                                                          windowHeight)),
                                                  m_nearPlane(.1f), m_farPlane(1024.0f) {
}

void Camera::processInput(GLFWwindow *window, const float deltaTime) {
    const float cameraSpeed = 20.0f * deltaTime;
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

void Camera::mouseCallback(GLFWwindow *window, const double xpos, const double ypos) {
    auto* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (!cam || !cam->m_inputEnabled) return;

    cam->handleMouse(xpos, ypos);
}

float Camera::distanceToCamera(const Chunk& chunk) const {
    const auto x = static_cast<float>(chunk.m_x_start());
    const auto y = static_cast<float>(chunk.m_y_start());
    const auto z = static_cast<float>(chunk.m_z_start());
    const auto size = static_cast<float>(Chunk::m_size1());
    const glm::vec3 position(x, y, z);
    const glm::vec3 center = position + glm::vec3(x + size, y + size, z + size) * 0.5f;

    return glm::distance(m_cameraPos, center);
}

void Camera::resetMousePosition(GLFWwindow *window) {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    m_lastX = static_cast<float>(width) / 2.0f;
    m_lastY = static_cast<float>(height) / 2.0f;
    m_firstMouse = true;
}

glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(glm::radians(m_FOVDegrees), m_aspectRatio, m_nearPlane, m_farPlane);
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_cameraUp);;
}

Frustum Camera::getFrustum(glm::mat4 modelViewProjecMatrix) {
    constexpr float padding = 1.0f; // Prevent popping

    auto left = Plane(modelViewProjecMatrix[0][3] + modelViewProjecMatrix[0][0],
                       modelViewProjecMatrix[1][3] + modelViewProjecMatrix[1][0],
                       modelViewProjecMatrix[2][3] + modelViewProjecMatrix[2][0],
                       modelViewProjecMatrix[3][3] + modelViewProjecMatrix[3][0]
    );
    left.set_m_d(left.m_d1() + padding);

    auto right = Plane(modelViewProjecMatrix[0][3] - modelViewProjecMatrix[0][0],
                        modelViewProjecMatrix[1][3] - modelViewProjecMatrix[1][0],
                        modelViewProjecMatrix[2][3] - modelViewProjecMatrix[2][0],
                        modelViewProjecMatrix[3][3] - modelViewProjecMatrix[3][0]
    );
    right.set_m_d(right.m_d1() + padding);

    auto bottom = Plane(modelViewProjecMatrix[0][3] + modelViewProjecMatrix[0][1],
                         modelViewProjecMatrix[1][3] + modelViewProjecMatrix[1][1],
                         modelViewProjecMatrix[2][3] + modelViewProjecMatrix[2][1],
                         modelViewProjecMatrix[3][3] + modelViewProjecMatrix[3][1]
    );
    bottom.set_m_d(bottom.m_d1() + padding);

    auto top = Plane(modelViewProjecMatrix[0][3] - modelViewProjecMatrix[0][1],
                      modelViewProjecMatrix[1][3] - modelViewProjecMatrix[1][1],
                      modelViewProjecMatrix[2][3] - modelViewProjecMatrix[2][1],
                      modelViewProjecMatrix[3][3] - modelViewProjecMatrix[3][1]
    );
    top.set_m_d(top.m_d1() + padding);

    const auto near = Plane(modelViewProjecMatrix[0][3] + modelViewProjecMatrix[0][2],
                       modelViewProjecMatrix[1][3] + modelViewProjecMatrix[1][2],
                       modelViewProjecMatrix[2][3] + modelViewProjecMatrix[2][2],
                       modelViewProjecMatrix[3][3] + modelViewProjecMatrix[3][2]
    );

    const auto far = Plane(modelViewProjecMatrix[0][3] - modelViewProjecMatrix[0][2],
                      modelViewProjecMatrix[1][3] - modelViewProjecMatrix[1][2],
                      modelViewProjecMatrix[2][3] - modelViewProjecMatrix[2][2],
                      modelViewProjecMatrix[3][3] - modelViewProjecMatrix[3][2]
    );

    Frustum frustum(left, right, bottom, top, near, far);
    return frustum;
}

bool Camera::m_input_enabled() const {
    return m_inputEnabled;
}

void Camera::set_m_input_enabled(const bool m_input_enabled) {
    this->m_inputEnabled = m_input_enabled;
}
