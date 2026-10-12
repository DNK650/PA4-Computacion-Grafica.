#pragma once
#include "Mesh.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
struct Transform {
  glm::vec3 position{0}, rotation{0}, scale{1};
  glm::mat4 matrix() const;
};
struct Material {
  glm::vec3 color{0.25f, 0.65f, 0.9f};
  float ambient = .2f, diffuse = .8f, specular = .6f, shininess = 32;
  int texture = 0;
};
struct Animation {
  bool enabled = false, loop = true;
  int channel = 1;
  glm::vec3 from{0}, to{0, 360, 0};
  float duration = 4;
};
struct Object {
  int id = 0, parent = -1;
  std::string name, type, modelPath;
  Transform transform;
  Material material;
  Animation animation;
  Mesh mesh;
};
struct Light {
  glm::vec3 position{4, 7, 5}, color{1};
  float ambient = .25f, intensity = 1;
};
struct Scene {
  std::vector<Object> objects;
  int nextId = 1;
  Light light;
  Object *get(int id);
  const Object *get(int id) const;
  int add(const std::string &type, const std::string &path = "");
  int duplicate(int id);
  void remove(int id);
  bool setParent(int child, int parent);
  glm::mat4 world(int id, float time = 0, bool animate = false) const;
  std::vector<std::pair<int, int>> collisions(float time, bool animate) const;
  void save(const std::string &path) const;
  void load(const std::string &path);
};
