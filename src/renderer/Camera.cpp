#include "renderer/Camera.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

namespace helios::renderer {
void EditorCamera::SetViewportSize(float width, float height) {
  m_ViewportWidth = width;
  m_ViewportHeight = height > 0.0f ? height : 1.0f;
}

void EditorCamera::Update(float dt, bool hovered) {
  if (!hovered) {
    return;
  }
  GLFWwindow* win = glfwGetCurrentContext();
  if (!win) {
    return;
  }

  static double lastX = 0.0;
  static double lastY = 0.0;
  double x = 0.0;
  double y = 0.0;
  glfwGetCursorPos(win, &x, &y);
  const float dx = static_cast<float>(x - lastX);
  const float dy = static_cast<float>(y - lastY);
  lastX = x;
  lastY = y;

  if (m_Mode == CameraMode::Orbit) {
    if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
      m_Yaw += dx * m_Sensitivity * dt * 15.0f;
      m_Pitch += (m_InvertY ? dy : -dy) * m_Sensitivity * dt * 15.0f;
      m_Pitch = glm::clamp(m_Pitch, -1.5f, 1.5f);
    }
    if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
      const glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), glm::normalize(m_Target - Position())));
      const glm::vec3 up = glm::vec3(0, 1, 0);
      m_Target += right * (dx * 0.01f) + up * (dy * 0.01f);
    }
  } else {
    const glm::vec3 forward = glm::normalize(m_Target - m_FlyPos);
    const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) m_FlyPos += forward * dt * 8.0f;
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) m_FlyPos -= forward * dt * 8.0f;
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) m_FlyPos -= right * dt * 8.0f;
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) m_FlyPos += right * dt * 8.0f;
    m_Yaw += dx * m_Sensitivity * dt * 10.0f;
    m_Pitch += (m_InvertY ? dy : -dy) * m_Sensitivity * dt * 10.0f;
    m_Pitch = glm::clamp(m_Pitch, -1.5f, 1.5f);
    m_Target = m_FlyPos + glm::vec3(
                          cos(m_Pitch) * cos(m_Yaw),
                          sin(m_Pitch),
                          cos(m_Pitch) * sin(m_Yaw));
  }
}

void EditorCamera::Focus(const glm::vec3& target, float radius) {
  m_Target = target;
  m_Distance = glm::max(2.0f, radius * 3.0f);
}

void EditorCamera::Reset() {
  m_Target = glm::vec3(0.0f);
  m_Distance = 12.0f;
  m_Pitch = 0.4f;
  m_Yaw = -0.8f;
}

glm::mat4 EditorCamera::View() const {
  return glm::lookAt(Position(), m_Target, glm::vec3(0, 1, 0));
}

glm::mat4 EditorCamera::Projection() const {
  return glm::perspective(glm::radians(m_Fov), m_ViewportWidth / m_ViewportHeight, m_Near, m_Far);
}

glm::vec3 EditorCamera::Position() const {
  if (m_Mode == CameraMode::Fly) {
    return m_FlyPos;
  }
  return m_Target - glm::vec3(
                      cos(m_Pitch) * cos(m_Yaw),
                      sin(m_Pitch),
                      cos(m_Pitch) * sin(m_Yaw)) *
                        m_Distance;
}
CameraMode EditorCamera::Mode() const { return m_Mode; }
void EditorCamera::SetMode(CameraMode mode) { m_Mode = mode; }
float& EditorCamera::Sensitivity() { return m_Sensitivity; }
bool& EditorCamera::InvertY() { return m_InvertY; }
} // namespace helios::renderer
