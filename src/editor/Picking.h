#pragma once

#include "renderer/Camera.h"
#include "scene/Scene.h"

#include <glm/glm.hpp>

namespace helios::editor {

struct Ray {
  glm::vec3 origin{};
  glm::vec3 direction{};
};

/// Build a world-space ray from viewport-local pixel coords (top-left origin, Y down).
Ray ScreenToRay(float localX, float localY, float viewportW, float viewportH,
                const renderer::EditorCamera& camera);

/// Closest entity hit along the ray (cube AABB in local space). Returns 0 if none.
scene::EntityId PickEntity(const Ray& ray, const scene::Scene& scene, float maxDistance = 1.0e9f);

glm::mat4 EntityWorldMatrix(const scene::Entity& e);

} // namespace helios::editor
