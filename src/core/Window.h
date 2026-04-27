#pragma once

#include <string>

struct GLFWwindow;

namespace helios::core {
struct WindowDesc {
  int width = 1600;
  int height = 900;
  std::string title = "Helios3D Engine";
};

class Window {
public:
  Window() = default;
  ~Window();
  bool Create(const WindowDesc& desc);
  void Destroy();
  void PollEvents() const;
  void SwapBuffers() const;
  bool ShouldClose() const;
  GLFWwindow* Native() const;
  int Width() const;
  int Height() const;
  void SetSize(int width, int height);

private:
  GLFWwindow* m_Window = nullptr;
  int m_Width = 0;
  int m_Height = 0;
};
} // namespace helios::core
