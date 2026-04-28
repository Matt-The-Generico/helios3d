#pragma once

#include "renderer/Camera.h"
#include "scene/Scene.h"

#include <array>
#include <glm/glm.hpp>
#include <unordered_set>

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
  void RenderScene(const scene::Scene& scene, const EditorCamera& camera, ViewMode mode,
                   const std::unordered_set<scene::EntityId>* selection,
                   float selectionHighlightMul, float selectionPulseT);
  void RenderOriginAxes(const EditorCamera& camera, float axisLength);
  void EndFrame();
  const RendererStats& Stats() const;
  void PushFrameTime(float ms);

  unsigned int ColorAttachment() const { return m_ColorAttachment; }

private:
  unsigned int m_Program = 0;
  unsigned int m_Vao = 0;
  unsigned int m_Vbo = 0;
  unsigned int m_AxisVao = 0;
  unsigned int m_AxisVbo = 0;
  unsigned int m_Framebuffer = 0;
  unsigned int m_ColorAttachment = 0;
  unsigned int m_DepthStencilRbo = 0;
  int m_Width = 1;
  int m_Height = 1;
  RendererStats m_Stats{};
  size_t m_FrameCursor = 0;
};
} // namespace helios::renderer
