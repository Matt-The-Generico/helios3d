#include "scene/SceneSerializer.h"

#include "core/Log.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace helios::scene {
namespace {
const char* kRecentPath = "assets/scenes/recent.json";

std::filesystem::path ResolvePathForRead(const std::string& path) {
  const std::filesystem::path input(path);
  if (input.is_absolute()) return input;
  if (std::filesystem::exists(input)) return input;

  const std::filesystem::path oneUp = std::filesystem::path("..") / input;
  if (std::filesystem::exists(oneUp)) return oneUp;

  const std::filesystem::path twoUp = std::filesystem::path("..") / ".." / input;
  if (std::filesystem::exists(twoUp)) return twoUp;

  return input;
}

std::filesystem::path ResolvePathForWrite(const std::string& path) {
  const std::filesystem::path input(path);
  if (input.is_absolute()) return input;

  const std::filesystem::path oneUp = std::filesystem::path("..") / input;
  if (std::filesystem::exists(oneUp.parent_path())) return oneUp;

  const std::filesystem::path twoUp = std::filesystem::path("..") / ".." / input;
  if (std::filesystem::exists(twoUp.parent_path())) return twoUp;

  return input;
}

std::string PrimitiveName(PrimitiveType type) {
  switch (type) {
  case PrimitiveType::Cube:
    return "Cube";
  case PrimitiveType::Sphere:
    return "Sphere";
  case PrimitiveType::Plane:
    return "Plane";
  case PrimitiveType::Cylinder:
    return "Cylinder";
  case PrimitiveType::Cone:
    return "Cone";
  case PrimitiveType::Torus:
    return "Torus";
  default:
    return "Empty";
  }
}

PrimitiveType PrimitiveFromName(const std::string& type) {
  if (type == "Cube") return PrimitiveType::Cube;
  if (type == "Sphere") return PrimitiveType::Sphere;
  if (type == "Plane") return PrimitiveType::Plane;
  if (type == "Cylinder") return PrimitiveType::Cylinder;
  if (type == "Cone") return PrimitiveType::Cone;
  if (type == "Torus") return PrimitiveType::Torus;
  return PrimitiveType::Empty;
}
} // namespace

bool SceneSerializer::Save(const Scene& scene, const std::string& path) {
  const std::filesystem::path outputPath = ResolvePathForWrite(path);
  std::error_code ec;
  std::filesystem::create_directories(outputPath.parent_path(), ec);

  nlohmann::json root;
  root["entities"] = nlohmann::json::array();
  for (const auto& e : scene.Entities()) {
    nlohmann::json node;
    node["id"] = e.id;
    node["name"] = e.name;
    node["primitive"] = PrimitiveName(e.primitive);
    node["enabled"] = e.enabled;
    node["hidden"] = e.hidden;
    node["locked"] = e.locked;
    node["tag"] = e.tag;
    node["layer"] = e.layer;
    node["parent"] = e.parent ? nlohmann::json(*e.parent) : nlohmann::json(nullptr);
    node["transform"] = {
      {"position", {e.transform.position.x, e.transform.position.y, e.transform.position.z}},
      {"rotation", {e.transform.rotation.x, e.transform.rotation.y, e.transform.rotation.z}},
      {"scale", {e.transform.scale.x, e.transform.scale.y, e.transform.scale.z}}
    };
    node["material"] = {
      {"color", {e.material.color.x, e.material.color.y, e.material.color.z, e.material.color.w}},
      {"roughness", e.material.roughness},
      {"metallic", e.material.metallic},
      {"tiling", {e.material.tiling.x, e.material.tiling.y}},
      {"offset", {e.material.offset.x, e.material.offset.y}},
      {"texturePath", e.material.texturePath}
    };
    root["entities"].push_back(node);
  }
  std::ofstream out(outputPath);
  if (!out.is_open()) {
    core::Log::Message(core::LogLevel::Error, "Could not save scene: " + path);
    return false;
  }
  out << root.dump(2);
  PushRecentFile(path);
  return true;
}

bool SceneSerializer::Load(Scene& scene, const std::string& path) {
  const std::filesystem::path inputPath = ResolvePathForRead(path);
  std::ifstream in(inputPath);
  if (!in.is_open()) {
    core::Log::Message(core::LogLevel::Warning, "Scene file missing: " + path);
    return false;
  }
  nlohmann::json root;
  in >> root;

  scene.Clear();
  for (const auto& node : root["entities"]) {
    auto primitive = PrimitiveFromName(node.value("primitive", "Cube"));
    Entity& e = scene.CreateEntity(node.value("name", "Entity"), primitive);
    e.enabled = node.value("enabled", true);
    e.hidden = node.value("hidden", false);
    e.locked = node.value("locked", false);
    e.tag = node.value("tag", "Default");
    e.layer = node.value("layer", 0);
    const auto tr = node["transform"];
    e.transform.position = {tr["position"][0], tr["position"][1], tr["position"][2]};
    e.transform.rotation = {tr["rotation"][0], tr["rotation"][1], tr["rotation"][2]};
    e.transform.scale = {tr["scale"][0], tr["scale"][1], tr["scale"][2]};
    const auto mat = node["material"];
    e.material.color = {mat["color"][0], mat["color"][1], mat["color"][2], mat["color"][3]};
    e.material.roughness = mat.value("roughness", 0.5f);
    e.material.metallic = mat.value("metallic", 0.0f);
    e.material.tiling = {mat["tiling"][0], mat["tiling"][1]};
    e.material.offset = {mat["offset"][0], mat["offset"][1]};
    e.material.texturePath = mat.value("texturePath", "");
  }

  const auto& entities = scene.Entities();
  for (size_t i = 0; i < entities.size(); ++i) {
    const auto& node = root["entities"][i];
    if (!node["parent"].is_null()) {
      scene.SetParent(entities[i].id, node["parent"].get<EntityId>());
    }
  }
  PushRecentFile(path);
  return true;
}

std::vector<std::string> SceneSerializer::RecentFiles() {
  const std::filesystem::path recentPath = ResolvePathForRead(kRecentPath);
  std::ifstream in(recentPath);
  if (!in.is_open()) {
    return {};
  }
  nlohmann::json j;
  in >> j;
  if (!j.contains("recent")) {
    return {};
  }
  return j["recent"].get<std::vector<std::string>>();
}

void SceneSerializer::PushRecentFile(const std::string& path) {
  auto recent = RecentFiles();
  recent.erase(std::remove(recent.begin(), recent.end(), path), recent.end());
  recent.insert(recent.begin(), path);
  if (recent.size() > 10) recent.resize(10);
  nlohmann::json j;
  j["recent"] = recent;
  const std::filesystem::path recentPath = ResolvePathForWrite(kRecentPath);
  std::error_code ec;
  std::filesystem::create_directories(recentPath.parent_path(), ec);
  std::ofstream out(recentPath);
  if (out.is_open()) {
    out << j.dump(2);
  }
}
} // namespace helios::scene
