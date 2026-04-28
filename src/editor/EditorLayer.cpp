#include "editor/EditorLayer.h"

#include "editor/Picking.h"
#include "core/Log.h"
#include "core/Time.h"
#include "scene/SceneSerializer.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>
#include <filesystem>
#include <unordered_set>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <cstdint>

namespace helios::editor {
namespace {
std::filesystem::path ResolveAssetsRoot() {
  const std::filesystem::path local("assets");
  if (std::filesystem::exists(local)) return local;
  const std::filesystem::path oneUp = std::filesystem::path("..") / "assets";
  if (std::filesystem::exists(oneUp)) return oneUp;
  const std::filesystem::path twoUp = std::filesystem::path("..") / ".." / "assets";
  if (std::filesystem::exists(twoUp)) return twoUp;
  return local;
}

glm::vec3 SelectionCentroid(const scene::Scene& scene, const std::unordered_set<scene::EntityId>& sel) {
  glm::vec3 c(0.0f);
  int n = 0;
  for (const auto id : sel) {
    const auto* e = scene.Find(id);
    if (e) {
      c += e->transform.position;
      ++n;
    }
  }
  return n > 0 ? c / static_cast<float>(n) : glm::vec3(0.0f);
}
} // namespace

void EditorLayer::SyncCameraFromSettings() { m_Camera.SetInputConfig(m_Settings.camera); }

void EditorLayer::Init() {
  m_GizmoOperation = static_cast<int>(ImGuizmo::TRANSLATE);
  SyncCameraFromSettings();
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
  m_AnimationTime += dt;
  if (m_ViewportHoveredLastFrame) {
    m_Camera.ApplyScroll(ImGui::GetIO().MouseWheel);
  }
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
  if (m_Scene.Selection().size() > 1) {
    ImGui::Text("%zu entities selected", m_Scene.Selection().size());
    ImGui::Separator();
    ImGui::TextUnformatted("Use the viewport gizmo (W/E/R) to move selection.");
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
  const ImGuiHoveredFlags hoverFlags = ImGuiHoveredFlags_AllowWhenBlockedByActiveItem;
  m_HoveredViewport = ImGui::IsWindowHovered(hoverFlags);

  ImGui::TextUnformatted("RMB orbit · MMB pan · Scroll / +/- zoom · W/E/R gizmo · click pick (Shift add, Ctrl toggle)");
  ImGui::Checkbox("Grid Snap", &m_Snap);
  ImGui::SameLine();
  ImGui::DragFloat("Step", &m_SnapStep, 0.01f, 0.01f, 10.0f);
  ImGui::SameLine();
  ImGui::Checkbox("Local Space", &m_LocalSpace);

  ImVec2 size = ImGui::GetContentRegionAvail();
  if (size.x < 1.0f) size.x = 1.0f;
  if (size.y < 1.0f) size.y = 1.0f;

  m_Camera.SetViewportSize(size.x, size.y);
  m_Renderer.Resize(static_cast<int>(size.x), static_cast<int>(size.y));
  m_Renderer.BeginFrame({0.08f, 0.09f, 0.11f, 1.0f});
  const auto& sel = m_Scene.Selection();
  m_Renderer.RenderScene(m_Scene, m_Camera, m_ViewMode, &sel, m_Settings.selectionHighlight, m_AnimationTime);
  m_Renderer.RenderOriginAxes(m_Camera, m_Settings.axisLength);
  m_Renderer.EndFrame();

  ImGui::Image(static_cast<ImTextureID>(static_cast<intptr_t>(m_Renderer.ColorAttachment())), size, ImVec2(0, 1),
               ImVec2(1, 0));

  const ImVec2 imgMin = ImGui::GetItemRectMin();
  const bool imgHovered = ImGui::IsItemHovered(hoverFlags);

  ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
  ImGuizmo::SetOrthographic(false);
  ImGuizmo::SetRect(imgMin.x, imgMin.y, size.x, size.y);

  glm::mat4 view = m_Camera.View();
  glm::mat4 proj = m_Camera.Projection();
  float viewF[16];
  float projF[16];
  std::memcpy(viewF, glm::value_ptr(view), sizeof(viewF));
  std::memcpy(projF, glm::value_ptr(proj), sizeof(projF));

  if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
    if (ImGui::IsKeyPressed(ImGuiKey_W)) m_GizmoOperation = static_cast<int>(ImGuizmo::TRANSLATE);
    if (ImGui::IsKeyPressed(ImGuiKey_E)) m_GizmoOperation = static_cast<int>(ImGuizmo::ROTATE);
    if (ImGui::IsKeyPressed(ImGuiKey_R)) m_GizmoOperation = static_cast<int>(ImGuizmo::SCALE);
  }

  const ImGuizmo::MODE gizmoMode = m_LocalSpace ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
  float snapData[3] = {m_SnapStep, m_SnapStep, m_SnapStep};
  const ImGuizmo::OPERATION op = static_cast<ImGuizmo::OPERATION>(m_GizmoOperation);
  const float* snapPtr =
    (m_Snap && op == ImGuizmo::TRANSLATE) ? snapData : nullptr;

  if (!sel.empty()) {
    bool anyLocked = false;
    for (const auto id : sel) {
      const auto* ent = m_Scene.Find(id);
      if (ent && ent->locked) anyLocked = true;
    }

    if (!anyLocked) {
      if (sel.size() == 1) {
        auto* e = m_Scene.Find(*sel.begin());
        if (e) {
          glm::mat4 M = EntityWorldMatrix(*e);
          float mat[16];
          std::memcpy(mat, glm::value_ptr(M), sizeof(mat));
          ImGuizmo::Manipulate(viewF, projF, op, gizmoMode, mat, nullptr, snapPtr);
          if (ImGuizmo::IsUsing()) {
            float t[3];
            float r[3];
            float s[3];
            ImGuizmo::DecomposeMatrixToComponents(mat, t, r, s);
            e->transform.position = glm::vec3(t[0], t[1], t[2]);
            e->transform.rotation =
              glm::vec3(glm::radians(r[0]), glm::radians(r[1]), glm::radians(r[2]));
            e->transform.scale = glm::vec3(s[0], s[1], s[2]);
          }
        }
      } else {
        const glm::vec3 c = SelectionCentroid(m_Scene, sel);
        glm::mat4 M = glm::translate(glm::mat4(1.0f), c);
        float mat[16];
        std::memcpy(mat, glm::value_ptr(M), sizeof(mat));
        ImGuizmo::Manipulate(viewF, projF, ImGuizmo::TRANSLATE, ImGuizmo::WORLD, mat, nullptr, snapPtr);
        const bool usingGizmo = ImGuizmo::IsUsing();
        if (usingGizmo && !m_MultiGizmoWasUsing) {
          m_MultiPivotStart = c;
          m_MultiDragStart.clear();
          for (const auto id : sel) {
            if (const auto* ent = m_Scene.Find(id)) m_MultiDragStart[id] = ent->transform.position;
          }
        }
        if (usingGizmo) {
          const glm::mat4 M2 = glm::make_mat4(mat);
          const glm::vec3 newC = glm::vec3(M2[3]);
          const glm::vec3 delta = newC - m_MultiPivotStart;
          for (const auto id : sel) {
            auto it = m_MultiDragStart.find(id);
            auto* ent = m_Scene.Find(id);
            if (ent && !ent->locked && it != m_MultiDragStart.end()) {
              ent->transform.position = it->second + delta;
            }
          }
        }
        m_MultiGizmoWasUsing = usingGizmo;
      }
    }
  } else {
    m_MultiGizmoWasUsing = false;
  }

  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && imgHovered && !ImGuizmo::IsOver() &&
      !ImGuizmo::IsUsing()) {
    const ImVec2 mp = ImGui::GetMousePos();
    const float lx = mp.x - imgMin.x;
    const float ly = mp.y - imgMin.y;
    const Ray ray = ScreenToRay(lx, ly, size.x, size.y, m_Camera);
    const scene::EntityId hit = PickEntity(ray, m_Scene);
    const ImGuiIO& io = ImGui::GetIO();
    if (hit != 0) {
      if (io.KeyShift) {
        m_Scene.AddSelection(hit);
      } else if (io.KeyCtrl) {
        m_Scene.ToggleSelection(hit);
      } else {
        m_Scene.SelectSingle(hit);
      }
    } else if (!io.KeyShift && !io.KeyCtrl) {
      m_Scene.ClearSelection();
    }
  }

  m_ViewportHoveredLastFrame = m_HoveredViewport || imgHovered;

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
  if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    m_Scene.ClearSelection();
  }
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
