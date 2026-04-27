#pragma once

#include "core/Window.h"
#include "editor/EditorLayer.h"

namespace helios::core {
class Application {
public:
  bool Init();
  void Run();
  void Shutdown();

private:
  Window m_Window;
  editor::EditorLayer m_Editor;
  double m_LastFrame = 0.0;
};
} // namespace helios::core
