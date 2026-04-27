#include "core/Application.h"

#include "core/Log.h"
#include "core/Time.h"

#include <glad/glad.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

namespace helios::core {
bool Application::Init() {
  if (!glfwInit()) return false;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  if (!m_Window.Create({})) return false;
  glfwMakeContextCurrent(m_Window.Native());
  glfwSwapInterval(1);
  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) return false;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(m_Window.Native(), true);
  ImGui_ImplOpenGL3_Init("#version 410");

  Log::Init();
  m_Editor.Init();
  m_LastFrame = glfwGetTime();
  return true;
}

void Application::Run() {
  while (!m_Window.ShouldClose()) {
    const double now = glfwGetTime();
    const double delta = now - m_LastFrame;
    m_LastFrame = now;
    Time::Tick(delta);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    m_Editor.Update(Time::DeltaTime());
    m_Editor.Render();

    ImGui::Render();
    glViewport(0, 0, m_Window.Width(), m_Window.Height());
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      GLFWwindow* backup = glfwGetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(backup);
    }

    m_Window.SwapBuffers();
    m_Window.PollEvents();
  }
}

void Application::Shutdown() {
  m_Editor.Shutdown();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  m_Window.Destroy();
  glfwTerminate();
  Log::Shutdown();
}
} // namespace helios::core
