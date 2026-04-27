#include "core/Window.h"

#include "core/Log.h"

#include <GLFW/glfw3.h>

namespace helios::core {
Window::~Window() { Destroy(); }

bool Window::Create(const WindowDesc& desc) {
  m_Width = desc.width;
  m_Height = desc.height;
  m_Window = glfwCreateWindow(desc.width, desc.height, desc.title.c_str(), nullptr, nullptr);
  if (!m_Window) {
    Log::Message(LogLevel::Error, "Failed to create GLFW window");
    return false;
  }
  glfwSetWindowUserPointer(m_Window, this);
  glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* w, int width, int height) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (self) {
      self->SetSize(width, height);
    }
  });
  return true;
}

void Window::Destroy() {
  if (m_Window) {
    glfwDestroyWindow(m_Window);
    m_Window = nullptr;
  }
}

void Window::PollEvents() const { glfwPollEvents(); }
void Window::SwapBuffers() const { glfwSwapBuffers(m_Window); }
bool Window::ShouldClose() const { return glfwWindowShouldClose(m_Window); }
GLFWwindow* Window::Native() const { return m_Window; }
int Window::Width() const { return m_Width; }
int Window::Height() const { return m_Height; }
void Window::SetSize(int width, int height) {
  m_Width = width;
  m_Height = height;
}
} // namespace helios::core
