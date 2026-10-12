#pragma once
#include "Renderer.hpp"
#include <string>
struct Editor {
  Scene scene;
  Renderer renderer;
  int selected = -1;
  float time = 0;
  bool playing = false, preview = false;
  std::string status = "Listo. Crea un objeto para comenzar.";
  char scenePath[512] = "escena.json", objPath[512]{};
  float viewportX = 240, viewportY = 52, viewportW = 600, viewportH = 600;
  void init();
  void ui(float width, float height);
  void input(GLFWwindow *window, float dt);
  void demo();
};
