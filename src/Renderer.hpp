#pragma once
#include "Scene.hpp"
#include <GLFW/glfw3.h>
struct Camera {
  glm::vec3 target{0, 0, 0};
  float yaw = 45, pitch = 25, distance = 9;
  bool orthographic = false;
  glm::vec3 eye() const;
  glm::mat4 view() const;
  glm::mat4 projection(float aspect) const;
};
class Renderer {
  unsigned textures[3]{};

public:
  Camera camera;
  bool wireframe = false, grid = true, collisionEnabled = false;
  void init(const std::string &assets);
  void shutdown();
  void draw(const Scene &scene, int selected, float time, bool animate, int x,
            int y, int width, int height);
  int pick(const Scene &scene, float time, bool animate, float nx, float ny,
           float aspect) const;
};
