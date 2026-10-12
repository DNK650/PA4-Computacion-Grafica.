#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
struct Vertex {
  glm::vec3 position, normal;
  glm::vec2 uv;
};
struct Mesh {
  std::vector<Vertex> vertices;
  glm::vec3 minimum{-0.5f}, maximum{0.5f};
};
Mesh primitive(const std::string &type);
Mesh loadOBJ(const std::string &path);
