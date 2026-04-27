#include "editor/EditorLayer.h"

#include "core/Log.h"
#include "core/Time.h"
#include "scene/SceneSerializer.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>

#include <filesystem>
#include <sstream>
#include <cstdio>
#include <cstdint>

namespace helios::editor {
namespace {
const char* PrimitiveNames[] = {"Cube", "Sphere", "Plane", "Cylinder", "Cone", "Torus", "Empty"};

std::filesystem::path ResolveAssetsRoot() {
  const std::filesystem::path local("assets");
  if (std::filesystem::exists(local)) return local;
  const std::filesystem::path oneUp = std::filesystem::path("..") / "assets";
  if (std::filesystem::exists(oneUp)) return oneUp;
  const std::filesystem::path twoUp = std::filesystem::path("..") / ".." / "assets";
  if (std::filesystem::exists(twoUp)) return twoUp;
  return local;
}
}

void EditorLayer::Init() {
  m_Renderer.Init();
  EnsureBootstrapScene();
  if (std::filesystem::exists("assets/scenes/recovery.json")) {
    scene::SceneSerializer::Load(m_Scene, "assets/scenes/recovery.json");
    m_Notifications.push_back({"Crash recovery loaded", 4.0f});
  }
}

void EditorLayer::Shutdown() {
  scene::SceneSerializer::Save(m_Scene, "assets/scenes/recovery.json");
  m_Renderer.Shutdown();
}

void EditorLayer::EnsureBootstrapScene() {
  if (!scene::SceneSerializer::Load(m_Scene, "assets/scenes/default_scene.json")) {
    auto& a = m_Scene.CreateEntity("Cube_A", scene::PrimitiveType::Cube);
    a.transform.position = {-1.5f, 0.0f, 0.0f};
    auto& b = m_Scene.CreateEntity("Cube_B", scene::PrimitiveType::Cube);
    b.transform.position = {1.5f, 0.0f, 0.0f};
    scene::SceneSerializer::Save(m_Scene, "assets/scenes/default_scene.json");
  }
}

void EditorLayer::Update(float dt) {
  m_Camera.Update(dt, m_HoveredViewport);
  m_Renderer.PushFrameTime(dt * 1000.0f);
  m_AutosaveTimer += dt;
  if (m_AutosaveTimer > 20.0f) {
    scene::SceneSerializer::Save(m_Scene, "assets/scenes/autosave.json");
    m_AutosaveTimer = 0.0f;
  }
  for (auto& n : m_Notifications) n.ttl -= dt;
  while (!m_Notifications.empty() && m_Notifications.front().ttl <= 0.0f) {
    m_Notifications.pop_front();
  }
}

void EditorLayer::Render() {
  DrawDockspace();
  DrawMenuBar();
  DrawHierarchy();
  DrawInspector();
  DrawViewport();
  DrawAssets();
  DrawStats();
  DrawConsole();
  DrawNotifications();
  HandleShortcuts();
}

void EditorLayer::DrawDockspace() {
  ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);
  flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::Begin("##DockSpace", nullptr, flags);
  ImGui::PopStyleVar(2);
  ImGui::DockSpace(ImGui::GetID("HeliosDockSpace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
  ImGui::End();
}

void EditorLayer::DrawMenuBar() {
  if (!ImGui::BeginMainMenuBar()) return;
  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("New Scene")) {
      m_Scene.Clear();
      PushUndoState();
    }
    if (ImGui::MenuItem("Open Default")) LoadScene("assets/scenes/default_scene.json");
    if (ImGui::MenuItem("Save", "Ctrl+S")) SaveSceneAs(m_CurrentScenePath);
    if (ImGui::MenuItem("Save As")) SaveSceneAs("assets/scenes/manual_save.json");
    if (ImGui::BeginMenu("Recent")) {
      for (const auto& file : scene::SceneSerializer::RecentFiles()) {
        if (ImGui::MenuItem(file.c_str())) LoadScene(file);
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenu();
  }
  if (ImGui::BeginMenu("Edit")) {
    if (ImGui::MenuItem("Undo", "Ctrl+Z")) Undo();
    if (ImGui::MenuItem("Redo", "Ctrl+Y")) Redo();
    if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
      for (auto id : m_Scene.Selection()) m_Scene.DuplicateEntity(id);
    }
    ImGui::EndMenu();
  }
  if (ImGui::BeginMenu("View")) {
    ImGui::MenuItem("Fullscreen Viewport", nullptr, &m_ViewportFullscreen);
    ImGui::MenuItem("Dark Theme", nullptr, &m_DarkTheme);
    ImGui::MenuItem("ImGui Demo", nullptr, &m_ShowDemoWindow);
    ImGui::EndMenu();
  }
  ImGui::EndMainMenuBar();
}

void EditorLayer::DrawHierarchy() {
  if (!ImGui::Begin("Hierarchy")) {
    ImGui::End();
    return;
  }
  if (ImGui::Button("Add Cube")) m_Scene.CreateEntity("Cube", scene::PrimitiveType::Cube);
  ImGui::SameLine();
  if (ImGui::Button("Add Empty")) m_Scene.CreateEntity("Empty", scene::PrimitiveType::Empty);
  ImGui::Separator();

  for (auto& e : m_Scene.Entities()) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (m_Scene.IsSelected(e.id)) flags |= ImGuiTreeNodeFlags_Selected;
    const bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(e.id)), flags, "%s", e.name.c_str());
    if (ImGui::IsItemClicked()) {
      const auto& io = ImGui::GetIO();
      if (io.KeyCtrl || io.KeyShift) m_Scene.ToggleSelection(e.id);
      else m_Scene.SelectSingle(e.id);
    }
    if (ImGui::BeginPopupContextItem()) {
      if (ImGui::MenuItem("Duplicate")) m_Scene.DuplicateEntity(e.id);
      if (ImGui::MenuItem("Delete")) m_Scene.DeleteEntity(e.id);
      ImGui::EndPopup();
    }
    if (open) ImGui::TreePop();
  }
  ImGui::End();
}

void EditorLayer::DrawInspector() {
  if (!ImGui::Begin("Inspector")) {
    ImGui::End();
    return;
  }
  if (m_Scene.Selection().empty()) {
    ImGui::TextUnformatted("No selection");
    ImGui::End();
    return;
  }
  const auto selected = *m_Scene.Selection().begin();
  auto* e = m_Scene.Find(selected);
  if (!e) {
    ImGui::End();
    return;
  }
  char buffer[256]{};
  std::snprintf(buffer, sizeof(buffer), "%s", e->name.c_str());
  if (ImGui::InputText("Name", buffer, sizeof(buffer))) e->name = buffer;
  ImGui::Checkbox("Enabled", &e->enabled);
  ImGui::SameLine(); ImGui::Checkbox("Hidden", &e->hidden);
  ImGui::SameLine(); ImGui::Checkbox("Locked", &e->locked);
  ImGui::DragFloat3("Position", &e->transform.position.x, 0.05f);
  ImGui::DragFloat3("Rotation", &e->transform.rotation.x, 0.05f);
  ImGui::DragFloat3("Scale", &e->transform.scale.x, 0.05f, 0.01f, 100.0f);
  ImGui::ColorEdit4("Color", &e->material.color.x);
  ImGui::SliderFloat("Roughness", &e->material.roughness, 0.0f, 1.0f);
  ImGui::SliderFloat("Metallic", &e->material.metallic, 0.0f, 1.0f);
  ImGui::DragFloat2("Tiling", &e->material.tiling.x, 0.01f);
  ImGui::DragFloat2("Offset", &e->material.offset.x, 0.01f);
  std::snprintf(buffer, sizeof(buffer), "%s", e->material.texturePath.c_str());
  if (ImGui::InputText("Texture Path", buffer, sizeof(buffer))) e->material.texturePath = buffer;
  if (ImGui::Button("Reset Transform")) e->transform = {};
  ImGui::End();
}

void EditorLayer::DrawViewport() {
  if (!ImGui::Begin("Viewport")) {
    ImGui::End();
    return;
  }
  m_HoveredViewport = ImGui::IsWindowHovered();
  ImVec2 size = ImGui::GetContentRegionAvail();
  if (size.x < 1.0f) size.x = 1.0f;
  if (size.y < 1.0f) size.y = 1.0f;
  m_Camera.SetViewportSize(size.x, size.y);
  m_Renderer.Resize(static_cast<int>(size.x), static_cast<int>(size.y));
  m_Renderer.BeginFrame({0.08f, 0.09f, 0.11f, 1.0f});
  m_Renderer.RenderScene(m_Scene, m_Camera, m_ViewMode, m_ShowBounds);
  m_Renderer.EndFrame();
  ImGui::Image(static_cast<ImTextureID>(static_cast<intptr_t>(m_Renderer.ColorAttachment())), size, ImVec2(0, 1), ImVec2(1, 0));
  ImGui::Text("Viewport active. Controls: RMB orbit, MMB pan");
  ImGui::Checkbox("Grid Snap", &m_Snap);
  ImGui::SameLine();
  ImGui::DragFloat("Step", &m_SnapStep, 0.01f, 0.01f, 10.0f);
  ImGui::SameLine();
  ImGui::Checkbox("Local Space", &m_LocalSpace);
  ImGui::End();
}

void EditorLayer::DrawAssets() {
  if (!ImGui::Begin("Assets")) {
    ImGui::End();
    return;
  }
  const std::filesystem::path root = ResolveAssetsRoot();
  std::error_code ec;
  if (!std::filesystem::exists(root, ec)) {
    ImGui::TextUnformatted("Assets folder not found.");
    ImGui::End();
    return;
  }
  for (const auto& p : std::filesystem::recursive_directory_iterator(root, std::filesystem::directory_options::skip_permission_denied, ec)) {
    if (ec) break;
    if (p.is_directory()) continue;
    std::string item = p.path().string();
    ImGui::Selectable(item.c_str());
    if (ImGui::BeginDragDropSource()) {
      ImGui::SetDragDropPayload("ASSET_PATH", item.c_str(), item.size() + 1);
      ImGui::TextUnformatted(item.c_str());
      ImGui::EndDragDropSource();
    }
  }
  ImGui::End();
}

void EditorLayer::DrawStats() {
  if (!ImGui::Begin("Scene Stats")) {
    ImGui::End();
    return;
  }
  const auto& stats = m_Renderer.Stats();
  ImGui::Text("Objects: %d", stats.objectCount);
  ImGui::Text("Triangles: %d", stats.triangleCount);
  ImGui::Text("Draw Calls: %d", stats.drawCalls);
  ImGui::Text("FPS: %.1f", stats.fps);
  ImGui::PlotLines("Frame Time (ms)", stats.frameGraph.data(), static_cast<int>(stats.frameGraph.size()));
  ImGui::End();
}

void EditorLayer::DrawConsole() {
  if (!ImGui::Begin("Console")) {
    ImGui::End();
    return;
  }
  ImGui::TextUnformatted("Logging routed to stdout for now.");
  if (ImGui::Button("Reload Shaders")) {
    m_Renderer.Shutdown();
    m_Renderer.Init();
    m_Notifications.push_back({"Shaders reloaded", 2.0f});
  }
  ImGui::End();
}

void EditorLayer::DrawNotifications() {
  ImGui::SetNextWindowBgAlpha(0.6f);
  if (ImGui::Begin("##Notif", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize)) {
    for (const auto& n : m_Notifications) {
      ImGui::TextUnformatted(n.text.c_str());
    }
  }
  ImGui::End();
}

void EditorLayer::HandleShortcuts() {
  const ImGuiIO& io = ImGui::GetIO();
  if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) SaveSceneAs(m_CurrentScenePath);
  if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D)) {
    for (auto id : m_Scene.Selection()) m_Scene.DuplicateEntity(id);
  }
  if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) Undo();
  if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) Redo();
  if (ImGui::IsKeyPressed(ImGuiKey_F) && !m_Scene.Selection().empty()) {
    if (auto* e = m_Scene.Find(*m_Scene.Selection().begin())) m_Camera.Focus(e->transform.position);
  }
  if (ImGui::IsKeyPressed(ImGuiKey_H) && !m_Scene.Selection().empty()) {
    if (auto* e = m_Scene.Find(*m_Scene.Selection().begin())) e->hidden = !e->hidden;
  }
}

void EditorLayer::SaveSceneAs(const std::string& path) {
  if (scene::SceneSerializer::Save(m_Scene, path)) {
    m_CurrentScenePath = path;
    m_Notifications.push_back({"Scene saved", 1.5f});
  }
}

void EditorLayer::LoadScene(const std::string& path) {
  if (scene::SceneSerializer::Load(m_Scene, path)) {
    m_CurrentScenePath = path;
    m_Notifications.push_back({"Scene loaded", 1.5f});
  }
}

void EditorLayer::PushUndoState() {
  scene::SceneSerializer::Save(m_Scene, "assets/scenes/.undo_tmp.json");
  m_Undo.push_back({"assets/scenes/.undo_tmp.json"});
  if (m_Undo.size() > 32) m_Undo.erase(m_Undo.begin());
}

void EditorLayer::Undo() {
  if (m_Undo.empty()) return;
  m_Redo.push_back({m_CurrentScenePath});
  LoadScene(m_Undo.back().jsonPath);
  m_Undo.pop_back();
}

void EditorLayer::Redo() {
  if (m_Redo.empty()) return;
  LoadScene(m_Redo.back().jsonPath);
  m_Redo.pop_back();
}

scene::Scene& EditorLayer::ActiveScene() { return m_Scene; }
} // namespace helios::editor
