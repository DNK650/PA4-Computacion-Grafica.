#include "Mesh.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
static constexpr float pi = 3.14159265359f;
static void tri(Mesh &m, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 n,
                glm::vec2 u = {0, 0}, glm::vec2 v = {1, 0},
                glm::vec2 w = {1, 1}) {
  m.vertices.push_back({a, n, u});
  m.vertices.push_back({b, n, v});
  m.vertices.push_back({c, n, w});
}
Mesh primitive(const std::string &type) {
  Mesh m;
  if (type == "Plano") {
    tri(m, {-0.5f, 0, -0.5f}, {-0.5f, 0, 0.5f}, {0.5f, 0, 0.5f}, {0, 1, 0});
    tri(m, {-0.5f, 0, -0.5f}, {0.5f, 0, 0.5f}, {0.5f, 0, -0.5f}, {0, 1, 0},
        {0, 0}, {1, 1}, {0, 1});
    m.minimum.y = m.maximum.y = 0;
  } else if (type == "Esfera") {
    auto p = [](float a, float b) {
      return glm::vec3(std::sin(b) * std::cos(a), std::cos(b),
                       std::sin(b) * std::sin(a)) *
             0.5f;
    };
    for (int j = 0; j < 16; j++)
      for (int i = 0; i < 24; i++) {
        float a = i * 2 * pi / 24, b = j * pi / 16, a2 = (i + 1) * 2 * pi / 24,
              b2 = (j + 1) * pi / 16;
        glm::vec3 q[4] = {p(a, b), p(a2, b), p(a2, b2), p(a, b2)};
        glm::vec2 uv[4] = {{i / 24.f, j / 16.f},
                           {(i + 1) / 24.f, j / 16.f},
                           {(i + 1) / 24.f, (j + 1) / 16.f},
                           {i / 24.f, (j + 1) / 16.f}};
        for (int k : {0, 1, 2, 0, 2, 3})
          m.vertices.push_back({q[k], glm::normalize(q[k]), uv[k]});
      }
  } else if (type == "Cilindro") {
    for (int i = 0; i < 32; i++) {
      float a = i * 2 * pi / 32, b = (i + 1) * 2 * pi / 32;
      glm::vec3 p{0.5f * std::cos(a), -0.5f, 0.5f * std::sin(a)},
          q{0.5f * std::cos(b), -0.5f, 0.5f * std::sin(b)},
          r = q + glm::vec3(0, 1, 0), s = p + glm::vec3(0, 1, 0);
      glm::vec3 n = glm::normalize(
          glm::vec3(std::cos((a + b) / 2), 0, std::sin((a + b) / 2)));
      tri(m, p, r, q, n);
      tri(m, p, s, r, n, {0, 0}, {0, 1}, {1, 1});
      tri(m, {0, 0.5f, 0}, r, s, {0, 1, 0});
      tri(m, {0, -0.5f, 0}, p, q, {0, -1, 0});
    }
  } else {
    glm::vec3 v[8] = {{-.5f, -.5f, -.5f}, {.5f, -.5f, -.5f}, {.5f, .5f, -.5f},
                      {-.5f, .5f, -.5f},  {-.5f, -.5f, .5f}, {.5f, -.5f, .5f},
                      {.5f, .5f, .5f},    {-.5f, .5f, .5f}};
    int faces[6][4] = {{4, 5, 6, 7}, {1, 0, 3, 2}, {0, 4, 7, 3},
                       {5, 1, 2, 6}, {3, 7, 6, 2}, {0, 1, 5, 4}};
    glm::vec3 ns[6] = {{0, 0, 1}, {0, 0, -1}, {-1, 0, 0},
                       {1, 0, 0}, {0, 1, 0},  {0, -1, 0}};
    for (int f = 0; f < 6; f++) {
      auto *x = faces[f];
      tri(m, v[x[0]], v[x[1]], v[x[2]], ns[f]);
      tri(m, v[x[0]], v[x[2]], v[x[3]], ns[f], {0, 0}, {1, 1}, {0, 1});
    }
  }
  return m;
}
Mesh loadOBJ(const std::string &path) {
  std::ifstream f(path);
  if (!f)
    throw std::runtime_error("No se pudo abrir OBJ: " + path);
  std::vector<glm::vec3> ps, ns;
  std::vector<glm::vec2> ts;
  Mesh m;
  std::string line;
  auto idx = [](int i, int n) {
    int k = i > 0 ? i - 1 : n + i;
    if (i == 0 || k < 0 || k >= n)
      throw std::runtime_error("Indice OBJ invalido");
    return k;
  };
  while (std::getline(f, line)) {
    std::istringstream s(line);
    std::string tag;
    s >> tag;
    if (tag == "v") {
      glm::vec3 p;
      s >> p.x >> p.y >> p.z;
      ps.push_back(p);
    } else if (tag == "vn") {
      glm::vec3 n;
      s >> n.x >> n.y >> n.z;
      ns.push_back(n);
    } else if (tag == "vt") {
      glm::vec2 t;
      s >> t.x >> t.y;
      ts.push_back(t);
    } else if (tag == "f") {
      std::vector<Vertex> face;
      std::string token;
      while (s >> token) {
        if (token[0] == '#')
          break;
        std::string fields[3];
        std::istringstream t(token);
        for (auto &x : fields)
          std::getline(t, x, '/');
        Vertex v{};
        v.position = ps.at(idx(std::stoi(fields[0]), int(ps.size())));
        if (!fields[1].empty())
          v.uv = ts.at(idx(std::stoi(fields[1]), int(ts.size())));
        if (!fields[2].empty())
          v.normal = ns.at(idx(std::stoi(fields[2]), int(ns.size())));
        face.push_back(v);
      }
      for (size_t i = 1; i + 1 < face.size(); i++) {
        Vertex a = face[0], b = face[i], c = face[i + 1];
        auto cross =
            glm::cross(b.position - a.position, c.position - a.position);
        if (glm::length(cross) < 1e-7f)
          continue;
        auto n = glm::normalize(cross);
        if (glm::length(a.normal) < 1e-6f)
          a.normal = n;
        if (glm::length(b.normal) < 1e-6f)
          b.normal = n;
        if (glm::length(c.normal) < 1e-6f)
          c.normal = n;
        m.vertices.insert(m.vertices.end(), {a, b, c});
      }
    }
  }
  if (m.vertices.empty())
    throw std::runtime_error("OBJ sin triangulos");
  m.minimum = m.maximum = m.vertices[0].position;
  for (auto &v : m.vertices) {
    m.minimum = glm::min(m.minimum, v.position);
    m.maximum = glm::max(m.maximum, v.position);
  }
  return m;
}
