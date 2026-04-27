#pragma once

#include "scene/Scene.h"

#include <string>
#include <vector>

namespace helios::scene {
class SceneSerializer {
public:
  static bool Save(const Scene& scene, const std::string& path);
  static bool Load(Scene& scene, const std::string& path);
  static std::vector<std::string> RecentFiles();
  static void PushRecentFile(const std::string& path);
};
} // namespace helios::scene
