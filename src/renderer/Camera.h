#pragma once

#include <glm/glm.hpp>

namespace helios::renderer {
enum class CameraMode { Orbit, Fly };

class EditorCamera {
public:
  void SetViewportSize(float width, float height);
  void Update(float dt, bool hovered);
  void Focus(const glm::vec3& target, float radius = 3.0f);
  void Reset();

  glm::mat4 View() const;
  glm::mat4 Projection() const;
  glm::vec3 Position() const;
  CameraMode Mode() const;
  void SetMode(CameraMode mode);
  float& Sensitivity();
  bool& InvertY();

private:
  float m_ViewportWidth = 1.0f;
  float m_ViewportHeight = 1.0f;
  float m_Fov = 45.0f;
  float m_Near = 0.1f;
  float m_Far = 1000.0f;
  float m_Pitch = 0.4f;
  float m_Yaw = -0.8f;
  float m_Distance = 12.0f;
  glm::vec3 m_Target{0.0f};
  glm::vec3 m_FlyPos{0.0f, 2.0f, 6.0f};
  CameraMode m_Mode = CameraMode::Orbit;
  float m_Sensitivity = 0.18f;
  bool m_InvertY = false;
};
} // namespace helios::renderer
