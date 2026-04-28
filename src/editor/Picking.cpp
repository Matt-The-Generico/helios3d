#include "editor/Picking.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace helios::editor {
namespace {

bool RayAabb(const glm::vec3& ro, const glm::vec3& rd, const glm::vec3& bmin, const glm::vec3& bmax,
             float& tHit) {
  float tmin = 0.0f;
  float tmax = 1.0e30f;
  for (int i = 0; i < 3; ++i) {
    const float o = ro[i];
    const float d = rd[i];
    const float bn = bmin[i];
    const float bx = bmax[i];
    if (std::fabs(d) < 1.0e-8f) {
      if (o < bn || o > bx) return false;
      continue;
    }
    float t1 = (bn - o) / d;
    float t2 = (bx - o) / d;
    if (t1 > t2) std::swap(t1, t2);
    tmin = std::max(tmin, t1);
    tmax = std::min(tmax, t2);
    if (tmin > tmax) return false;
  }
  if (tmax < 0.0f) return false;
  tHit = tmin >= 0.0f ? tmin : tmax;
  return true;
}

glm::vec3 AbsVec(const glm::vec3& v) { return glm::abs(v); }
} // namespace

glm::mat4 EntityWorldMatrix(const scene::Entity& e) {
  glm::mat4 m(1.0f);
  m = glm::translate(m, e.transform.position);
  m = glm::rotate(m, e.transform.rotation.x, glm::vec3(1, 0, 0));
  m = glm::rotate(m, e.transform.rotation.y, glm::vec3(0, 1, 0));
  m = glm::rotate(m, e.transform.rotation.z, glm::vec3(0, 0, 1));
  m = glm::scale(m, e.transform.scale);
  return m;
}

Ray ScreenToRay(float localX, float localY, float viewportW, float viewportH,
                const renderer::EditorCamera& camera) {
  Ray ray{};
  const glm::vec3 camPos = camera.Position();
  if (viewportW <= 0.0f || viewportH <= 0.0f) {
    ray.origin = camPos;
    ray.direction = camera.Forward();
    return ray;
  }
  const float ndcX = (2.0f * localX) / viewportW - 1.0f;
  const float ndcY = 1.0f - (2.0f * localY) / viewportH;
  const glm::mat4 invVP = glm::inverse(camera.Projection() * camera.View());
  glm::vec4 pNear = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
  glm::vec4 pFar = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
  pNear /= pNear.w;
  pFar /= pFar.w;
  ray.origin = camPos;
  ray.direction = glm::normalize(glm::vec3(pFar) - camPos);
  return ray;
}

scene::EntityId PickEntity(const Ray& ray, const scene::Scene& scene, float maxDistance) {
  scene::EntityId best = 0;
  float bestT = maxDistance;
  for (const auto& e : scene.Entities()) {
    if (!e.enabled || e.hidden) continue;

    glm::vec3 half(0.5f);
    if (e.primitive == scene::PrimitiveType::Empty) {
      half = glm::vec3(0.15f);
    } else {
      half = 0.5f * AbsVec(e.transform.scale);
    }

    const glm::mat4 inv = glm::inverse(EntityWorldMatrix(e));
    const glm::vec3 ro = glm::vec3(inv * glm::vec4(ray.origin, 1.0f));
    const glm::vec3 rd = glm::normalize(glm::vec3(inv * glm::vec4(ray.direction, 0.0f)));

    float t = 0.0f;
    const glm::vec3 bmin = -half;
    const glm::vec3 bmax = half;
    if (RayAabb(ro, rd, bmin, bmax, t) && t >= 0.0f && t < bestT) {
      bestT = t;
      best = e.id;
    }
  }
  return best;
}

} // namespace helios::editor
