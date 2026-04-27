#include "scene/Scene.h"

#include <algorithm>

namespace helios::scene {
Entity& Scene::CreateEntity(const std::string& name, PrimitiveType primitive) {
  Entity e;
  e.id = m_NextId++;
  e.name = name;
  e.primitive = primitive;
  m_Entities.push_back(e);
  return m_Entities.back();
}

bool Scene::DeleteEntity(EntityId id) {
  const auto it = std::find_if(m_Entities.begin(), m_Entities.end(), [id](const Entity& e) { return e.id == id; });
  if (it == m_Entities.end()) {
    return false;
  }
  m_Selected.erase(id);
  for (auto& e : m_Entities) {
    e.children.erase(std::remove(e.children.begin(), e.children.end(), id), e.children.end());
    if (e.parent && *e.parent == id) {
      e.parent.reset();
    }
  }
  m_Entities.erase(it);
  return true;
}

Entity* Scene::Find(EntityId id) {
  const auto it = std::find_if(m_Entities.begin(), m_Entities.end(), [id](const Entity& e) { return e.id == id; });
  return it == m_Entities.end() ? nullptr : &(*it);
}

const Entity* Scene::Find(EntityId id) const {
  const auto it = std::find_if(m_Entities.begin(), m_Entities.end(), [id](const Entity& e) { return e.id == id; });
  return it == m_Entities.end() ? nullptr : &(*it);
}

std::vector<Entity>& Scene::Entities() { return m_Entities; }
const std::vector<Entity>& Scene::Entities() const { return m_Entities; }
void Scene::Clear() {
  m_Entities.clear();
  m_Selected.clear();
  m_NextId = 1;
}

EntityId Scene::DuplicateEntity(EntityId id) {
  const Entity* src = Find(id);
  if (!src) {
    return 0;
  }
  Entity& copy = CreateEntity(src->name + "_copy", src->primitive);
  copy.transform = src->transform;
  copy.material = src->material;
  copy.enabled = src->enabled;
  copy.hidden = src->hidden;
  copy.locked = src->locked;
  copy.tag = src->tag;
  copy.layer = src->layer;
  return copy.id;
}

void Scene::SetParent(EntityId child, std::optional<EntityId> parent) {
  Entity* c = Find(child);
  if (!c) {
    return;
  }
  if (c->parent) {
    Entity* oldParent = Find(*c->parent);
    if (oldParent) {
      oldParent->children.erase(std::remove(oldParent->children.begin(), oldParent->children.end(), child), oldParent->children.end());
    }
  }
  c->parent = parent;
  if (parent) {
    Entity* p = Find(*parent);
    if (p) {
      p->children.push_back(child);
    } else {
      c->parent.reset();
    }
  }
}

void Scene::SelectSingle(EntityId id) {
  m_Selected.clear();
  m_Selected.insert(id);
}
void Scene::ToggleSelection(EntityId id) {
  if (m_Selected.contains(id)) {
    m_Selected.erase(id);
  } else {
    m_Selected.insert(id);
  }
}
void Scene::AddSelection(EntityId id) { m_Selected.insert(id); }
void Scene::ClearSelection() { m_Selected.clear(); }
bool Scene::IsSelected(EntityId id) const { return m_Selected.contains(id); }
const std::unordered_set<EntityId>& Scene::Selection() const { return m_Selected; }
} // namespace helios::scene
