#pragma once

#include "editor/EditorSettings.h"
#include "renderer/Camera.h"
#include "renderer/Renderer.h"
#include "scene/Scene.h"

#include <deque>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace helios::editor {
struct Notification {
  std::string text;
  float ttl = 2.0f;
};

class EditorLayer {
public:
  void Init();
  void Shutdown();
  void Update(float dt);
  void Render();
  scene::Scene& ActiveScene();

private:
  struct Snapshot {
    std::string jsonPath;
  };

  void SyncCameraFromSettings();
  void DrawDockspace();
  void DrawMenuBar();
  void DrawHierarchy();
  void DrawInspector();
  void DrawViewport();
  void DrawAssets();
  void DrawStats();
  void DrawNotifications();
  void DrawConsole();
  void HandleShortcuts();
  void SaveSceneAs(const std::string& path);
  void LoadScene(const std::string& path);
  void PushUndoState();
  void Undo();
  void Redo();
  void EnsureBootstrapScene();

  EditorSettings m_Settings{};
  scene::Scene m_Scene;
  renderer::Renderer m_Renderer;
  renderer::EditorCamera m_Camera;
  renderer::ViewMode m_ViewMode = renderer::ViewMode::Lit;
  bool m_ShowBounds = false;
  bool m_Snap = false;
  float m_SnapStep = 0.5f;
  bool m_LocalSpace = true;
  bool m_PlayMode = false;
  bool m_ViewportFullscreen = false;
  bool m_ShowDemoWindow = false;
  bool m_Grid = true;
  bool m_HoveredViewport = false;
  bool m_DarkTheme = true;
  float m_UIScale = 1.0f;
  std::string m_CurrentScenePath = "assets/scenes/autosave.json";
  float m_AutosaveTimer = 0.0f;
  std::vector<Snapshot> m_Undo;
  std::vector<Snapshot> m_Redo;
  std::deque<Notification> m_Notifications;

  /// Last frame viewport had focus for scroll zoom (updated in Render).
  bool m_ViewportHoveredLastFrame = false;

  float m_AnimationTime = 0.0f;

  /// ImGuizmo::OPERATION as int (see ImGuizmo.h).
  int m_GizmoOperation = 0;
  bool m_MultiGizmoWasUsing = false;
  glm::vec3 m_MultiPivotStart{};
  std::unordered_map<scene::EntityId, glm::vec3> m_MultiDragStart{};
};
} // namespace helios::editor
