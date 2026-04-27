#pragma once

#include "renderer/Camera.h"
#include "scene/Scene.h"

#include <array>
#include <glm/glm.hpp>

namespace helios::renderer {
enum class ViewMode { Lit, Unlit, Wireframe };

struct RendererStats {
  int objectCount = 0;
  int triangleCount = 0;
  int drawCalls = 0;
  float fps = 0.0f;
  std::array<float, 120> frameGraph{};
};

class Renderer {
public:
  bool Init();
  void Shutdown();
  void Resize(int width, int height);
  void BeginFrame(const glm::vec4& clearColor);
  void RenderScene(const scene::Scene& scene, const EditorCamera& camera, ViewMode mode, bool showBounds);
  void EndFrame();
  const RendererStats& Stats() const;
  void PushFrameTime(float ms);

private:
  unsigned int m_Program = 0;
  unsigned int m_Vao = 0;
  unsigned int m_Vbo = 0;
  unsigned int m_Framebuffer = 0;
  unsigned int m_ColorAttachment = 0;
  unsigned int m_DepthStencilRbo = 0;
  int m_Width = 1;
  int m_Height = 1;
  RendererStats m_Stats{};
  size_t m_FrameCursor = 0;

public:
  unsigned int ColorAttachment() const { return m_ColorAttachment; }
};
} // namespace helios::renderer
