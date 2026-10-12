#include "Scene.hpp"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
static void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
int main() {
  try {
    Scene s;
    int a = s.add("Cubo"), b = s.add("Cubo");
    s.get(a)->transform.position = {2, 0, 0};
    s.get(b)->transform.position = {1, 0, 0};
    check(s.setParent(b, a), "Parent");
    check(!s.setParent(a, b), "Cycle prevention");
    auto p = glm::vec3(s.world(b) * glm::vec4(0, 0, 0, 1));
    check(glm::length(p - glm::vec3(3, 0, 0)) < 1e-5f,
          "Hierarchy world transform");
    s.get(a)->animation = {true, false, 0, {2, 0, 0}, {4, 0, 0}, 2};
    p = glm::vec3(s.world(b, 1, true) * glm::vec4(0, 0, 0, 1));
    check(std::abs(p.x - 4) < 1e-5f, "Interpolation with child");
    s.get(b)->transform.position = {0, 0, 0};
    check(s.collisions(0, false).size() == 1, "Collision overlap");
    s.get(b)->transform.position = {4, 0, 0};
    check(s.collisions(0, false).empty(), "Collision separation");
    s.save("test_scene.json");
    Scene loaded;
    loaded.load("test_scene.json");
    check(loaded.objects.size() == 2 && loaded.get(b)->parent == a,
          "Persistence hierarchy");
    check(loaded.get(a)->animation.enabled, "Persistence animation");
    int c = loaded.duplicate(a);
    check(c != a && loaded.get(c), "Duplication");
    loaded.remove(a);
    check(!loaded.get(a) && !loaded.get(b) && loaded.get(c),
          "Recursive deletion");
    for (auto type : {"Cubo", "Esfera", "Cilindro", "Plano"})
      check(!primitive(type).vertices.empty(), "Primitive geometry");
    // A flipped winding makes two-sided lighting invert sphere normals.
    auto sphere = primitive("Esfera");
    for (size_t i = 0; i + 2 < sphere.vertices.size(); i += 3) {
      auto &a = sphere.vertices[i];
      auto &b = sphere.vertices[i + 1];
      auto &c = sphere.vertices[i + 2];
      auto faceNormal =
          glm::cross(b.position - a.position, c.position - a.position);
      if (glm::length(faceNormal) > 1e-7f)
        check(glm::dot(faceNormal, a.normal + b.normal + c.normal) > 0,
              "Sphere outward winding");
    }
    std::ofstream obj("test.obj");
    obj << "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nf -4 -3 -2 -1\n";
    obj.close();
    check(loadOBJ("test.obj").vertices.size() == 6,
          "OBJ polygon negative indices");
    int imported = s.add("OBJ", "test.obj");
    s.save("obj_scene.json");
    std::remove("test.obj");
    Scene portable;
    portable.load("obj_scene.json");
    check(portable.get(imported)->mesh.vertices.size() == 6,
          "Portable OBJ persistence");
    s.remove(imported);
    std::remove("obj_scene.json");
    std::ofstream bad("bad.json");
    bad << "{}";
    bad.close();
    bool failed = false;
    try {
      s.load("bad.json");
    } catch (...) {
      failed = true;
    }
    check(failed && s.objects.size() == 2, "Transactional load");
    std::remove("test_scene.json");
    std::remove("test.obj");
    std::remove("bad.json");
    std::cout
        << "PASS: hierarchy, cycles, interpolation, collisions, persistence, "
           "duplication, deletion, geometry, OBJ, invalid file\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
