#include "Camera.h"

#include <iostream>
#include <ostream>

#include "geometric.hpp"
#include "detail/func_trigonometric.inl"

Camera::Camera(const unsigned int windowWidth,
               const unsigned int windowHeight) : m_lastX(static_cast<float>(windowWidth) / 2.0f),
                                                  m_lastY(static_cast<float>(windowHeight) / 2.0f),
                                                  m_yaw(-90.0f), m_pitch(0.0f),
                                                  m_firstMouse(true), m_cameraPos(glm::vec3(0.0f, 0.0f, 3.0f)),
                                                  m_cameraFront(glm::vec3(0.0f, 0.0f, -1.0f)),
                                                  m_cameraUp(glm::vec3(0.0f, 1.0f, 0.0f)) {
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
    if (auto *cam = static_cast<Camera *>(glfwGetWindowUserPointer(window))) cam->handleMouse(xpos, ypos);
}

glm::vec3 Camera::m_camera_pos() const {
    return m_cameraPos;
}

glm::vec3 Camera::m_camera_front() const {
    return m_cameraFront;
}

glm::vec3 Camera::m_camera_up() const {
    return m_cameraUp;
}
