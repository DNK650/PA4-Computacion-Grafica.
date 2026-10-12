#include "Scene.hpp"
#include <fstream>
#include <limits>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
using json = nlohmann::json;
// Column vectors: scale, rotate in X/Y/Z, then translate.
glm::mat4 Transform::matrix() const {
  auto m = glm::translate(glm::mat4(1), position);
  m = glm::rotate(m, glm::radians(rotation.z), {0, 0, 1});
  m = glm::rotate(m, glm::radians(rotation.y), {0, 1, 0});
  m = glm::rotate(m, glm::radians(rotation.x), {1, 0, 0});
  return glm::scale(m, scale);
}
Object *Scene::get(int id) {
  for (auto &o : objects)
    if (o.id == id)
      return &o;
  return nullptr;
}
const Object *Scene::get(int id) const {
  for (auto &o : objects)
    if (o.id == id)
      return &o;
  return nullptr;
}
int Scene::add(const std::string &type, const std::string &path) {
  Object o;
  o.id = nextId++;
  o.type = type;
  o.name = type + " " + std::to_string(o.id);
  o.modelPath = path;
  o.mesh = type == "OBJ" ? loadOBJ(path) : primitive(type);
  objects.push_back(o);
  return o.id;
}
int Scene::duplicate(int id) {
  auto *o = get(id);
  if (!o)
    return -1;
  Object copy = *o;
  copy.id = nextId++;
  copy.name += " copia";
  copy.transform.position.x += 1;
  objects.push_back(copy);
  return copy.id;
}
void Scene::remove(int id) {
  std::set<int> ids{id};
  bool change = true;
  while (change) {
    change = false;
    for (auto &o : objects)
      if (ids.count(o.parent) && !ids.count(o.id)) {
        ids.insert(o.id);
        change = true;
      }
  }
  objects.erase(std::remove_if(objects.begin(), objects.end(),
                               [&](auto &o) { return ids.count(o.id); }),
                objects.end());
}
// Walk ancestors before assigning a parent to keep the graph acyclic.
bool Scene::setParent(int child, int parent) {
  auto *o = get(child);
  if (!o || child == parent)
    return false;
  int p = parent;
  std::set<int> seen;
  while (p != -1) {
    if (p == child || !seen.insert(p).second)
      return false;
    auto *a = get(p);
    if (!a)
      return false;
    p = a->parent;
  }
  o->parent = parent;
  return true;
}
// Animations change a temporary local transform, preserving editable values.
glm::mat4 Scene::world(int id, float time, bool animate) const {
  auto *o = get(id);
  if (!o)
    return glm::mat4(1);
  Transform t = o->transform;
  auto &a = o->animation;
  if (animate && a.enabled) {
    float u = a.duration > 0 ? time / a.duration : 1;
    u = a.loop ? u - std::floor(u) : glm::clamp(u, 0.f, 1.f);
    auto value = glm::mix(a.from, a.to, u);
    if (a.channel == 0)
      t.position = value;
    else if (a.channel == 1)
      t.rotation = value;
    else
      t.scale = value;
  }
  auto m = t.matrix();
  return o->parent == -1 ? m : world(o->parent, time, animate) * m;
}
std::vector<std::pair<int, int>> Scene::collisions(float time,
                                                   bool animate) const {
  struct Box {
    glm::vec3 lo, hi;
  };
  std::vector<Box> bs;
  for (auto &o : objects) {
    Box b{glm::vec3(std::numeric_limits<float>::max()),
          glm::vec3(std::numeric_limits<float>::lowest())};
    auto m = world(o.id, time, animate);
    for (int k = 0; k < 8; k++) {
      auto p = glm::vec3(
          m * glm::vec4(k & 1 ? o.mesh.maximum.x : o.mesh.minimum.x,
                        k & 2 ? o.mesh.maximum.y : o.mesh.minimum.y,
                        k & 4 ? o.mesh.maximum.z : o.mesh.minimum.z, 1));
      b.lo = glm::min(b.lo, p);
      b.hi = glm::max(b.hi, p);
    }
    bs.push_back(b);
  }
  std::vector<std::pair<int, int>> result;
  for (size_t i = 0; i < bs.size(); i++)
    for (size_t j = i + 1; j < bs.size(); j++) {
      auto &a = bs[i];
      auto &b = bs[j];
      if (a.lo.x <= b.hi.x && a.hi.x >= b.lo.x && a.lo.y <= b.hi.y &&
          a.hi.y >= b.lo.y && a.lo.z <= b.hi.z && a.hi.z >= b.lo.z)
        result.emplace_back(objects[i].id, objects[j].id);
    }
  return result;
}
static json vec(glm::vec3 v) { return {v.x, v.y, v.z}; }
static glm::vec3 v3(const json &j) {
  if (!j.is_array() || j.size() != 3)
    throw std::runtime_error("Vector invalido");
  glm::vec3 v{j[0].get<float>(), j[1].get<float>(), j[2].get<float>()};
  if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z))
    throw std::runtime_error("Vector no finito");
  return v;
}
void Scene::save(const std::string &path) const {
  json j;
  j["version"] = 1;
  j["light"] = {{"position", vec(light.position)},
                {"color", vec(light.color)},
                {"ambient", light.ambient},
                {"intensity", light.intensity}};
  j["objects"] = json::array();
  for (auto &o : objects) {
    j["objects"].push_back({{"id", o.id},
                            {"parent", o.parent},
                            {"name", o.name},
                            {"type", o.type},
                            {"model", o.modelPath},
                            {"position", vec(o.transform.position)},
                            {"rotation", vec(o.transform.rotation)},
                            {"scale", vec(o.transform.scale)},
                            {"material",
                             {{"color", vec(o.material.color)},
                              {"ambient", o.material.ambient},
                              {"diffuse", o.material.diffuse},
                              {"specular", o.material.specular},
                              {"shininess", o.material.shininess},
                              {"texture", o.material.texture}}},
                            {"animation",
                             {{"enabled", o.animation.enabled},
                              {"loop", o.animation.loop},
                              {"channel", o.animation.channel},
                              {"from", vec(o.animation.from)},
                              {"to", vec(o.animation.to)},
                              {"duration", o.animation.duration}}}});
    if (o.type == "OBJ") {
      json vertices = json::array();
      for (auto &v : o.mesh.vertices)
        vertices.push_back({{"p", vec(v.position)},
                            {"n", vec(v.normal)},
                            {"uv", {v.uv.x, v.uv.y}}});
      j["objects"].back()["mesh"] = vertices;
    }
  }
  std::ofstream f(path);
  if (!f)
    throw std::runtime_error("No se puede escribir la escena");
  f << j.dump(2);
  if (!f)
    throw std::runtime_error("Error al guardar");
}
// Build a temporary scene: invalid files never replace the current scene.
void Scene::load(const std::string &path) {
  std::ifstream f(path);
  if (!f)
    throw std::runtime_error("No se puede abrir la escena");
  json j;
  f >> j;
  if (j.at("version") != 1)
    throw std::runtime_error("Version no compatible");
  Scene tmp;
  std::set<int> ids;
  for (auto &x : j.at("objects")) {
    auto type = x.at("type").get<std::string>();
    if (type != "OBJ" && type != "Cubo" && type != "Plano" &&
        type != "Esfera" && type != "Cilindro")
      throw std::runtime_error("Tipo no compatible");
    tmp.add(type == "OBJ" && x.contains("mesh") ? "Cubo" : type, x.at("model"));
    auto *o = &tmp.objects.back();
    o->type = type;
    o->modelPath = x.at("model");
    if (type == "OBJ" && x.contains("mesh")) {
      o->mesh.vertices.clear();
      for (auto &v : x.at("mesh"))
        o->mesh.vertices.push_back({v3(v.at("p")), v3(v.at("n")),
                                    glm::vec2(v.at("uv")[0].get<float>(),
                                              v.at("uv")[1].get<float>())});
      if (o->mesh.vertices.empty() || o->mesh.vertices.size() % 3 != 0)
        throw std::runtime_error("Malla OBJ invalida");
      o->mesh.minimum = o->mesh.maximum = o->mesh.vertices[0].position;
      for (auto &v : o->mesh.vertices) {
        o->mesh.minimum = glm::min(o->mesh.minimum, v.position);
        o->mesh.maximum = glm::max(o->mesh.maximum, v.position);
      }
    }
    o->id = x.at("id");
    if (o->id < 1 || !ids.insert(o->id).second)
      throw std::runtime_error("ID duplicado o invalido");
    o->parent = x.at("parent");
    o->name = x.at("name");
    o->transform = {v3(x.at("position")), v3(x.at("rotation")),
                    v3(x.at("scale"))};
    auto &m = x.at("material");
    o->material = {v3(m.at("color")), m.at("ambient"),   m.at("diffuse"),
                   m.at("specular"),  m.at("shininess"), m.at("texture")};
    auto &a = x.at("animation");
    o->animation = {a.at("enabled"),  a.at("loop"),   a.at("channel"),
                    v3(a.at("from")), v3(a.at("to")), a.at("duration")};
    if (o->animation.duration <= 0 || o->animation.channel < 0 ||
        o->animation.channel > 2)
      throw std::runtime_error("Animacion invalida");
  }
  for (auto &o : tmp.objects) {
    int p = o.parent;
    o.parent = -1;
    if (!tmp.setParent(o.id, p))
      throw std::runtime_error("Jerarquia invalida");
  }
  auto &l = j.at("light");
  tmp.light = {v3(l.at("position")), v3(l.at("color")), l.at("ambient"),
               l.at("intensity")};
  tmp.nextId = 1;
  for (auto &o : tmp.objects)
    tmp.nextId = std::max(tmp.nextId, o.id + 1);
  *this = std::move(tmp);
}
