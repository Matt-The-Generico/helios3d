#pragma once

#include "renderer/Camera.h"

namespace helios::editor {

/// Editor preferences (camera + visuals). Tune here for personalized controls.
struct EditorSettings {
  renderer::CameraInputConfig camera{};
  float selectionHighlight = 1.28f;
  float axisLength = 1.35f;
};

} // namespace helios::editor
