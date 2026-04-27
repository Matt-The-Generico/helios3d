#pragma once

#include <glm/glm.hpp>

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace helios::scene {
using EntityId = uint64_t;

enum class PrimitiveType { Cube, Sphere, Plane, Cylinder, Cone, Torus, Empty };

struct Transform {
  glm::vec3 position{0.0f};
  glm::vec3 rotation{0.0f};
  glm::vec3 scale{1.0f};
};

struct Material {
  glm::vec4 color{0.8f, 0.8f, 0.8f, 1.0f};
  float roughness = 0.5f;
  float metallic = 0.0f;
  glm::vec2 tiling{1.0f, 1.0f};
  glm::vec2 offset{0.0f, 0.0f};
  std::string texturePath;
};

struct Entity {
  EntityId id = 0;
  std::string name;
  PrimitiveType primitive = PrimitiveType::Cube;
  Transform transform{};
  Material material{};
  bool enabled = true;
  bool hidden = false;
  bool locked = false;
  std::string tag = "Default";
  int layer = 0;
  std::optional<EntityId> parent;
  std::vector<EntityId> children;
};

class Scene {
public:
  Entity& CreateEntity(const std::string& name, PrimitiveType primitive = PrimitiveType::Cube);
  bool DeleteEntity(EntityId id);
  Entity* Find(EntityId id);
  const Entity* Find(EntityId id) const;
  std::vector<Entity>& Entities();
  const std::vector<Entity>& Entities() const;
  void Clear();
  EntityId DuplicateEntity(EntityId id);
  void SetParent(EntityId child, std::optional<EntityId> parent);

  void SelectSingle(EntityId id);
  void ToggleSelection(EntityId id);
  void AddSelection(EntityId id);
  void ClearSelection();
  bool IsSelected(EntityId id) const;
  const std::unordered_set<EntityId>& Selection() const;

private:
  EntityId m_NextId = 1;
  std::vector<Entity> m_Entities;
  std::unordered_set<EntityId> m_Selected;
};
} // namespace helios::scene
