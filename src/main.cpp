#include "Editor.hpp"
#include <cstring>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl2.h>
#include <iostream>
int main(int argc, char **argv) {
  glfwSetErrorCallback([](int code, const char *message) {
    std::cerr << "GLFW " << code << ": " << message << '\n';
  });
  if (!glfwInit())
    return 1;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  auto *window = glfwCreateWindow(1280, 800, "Editor de Escenas 3D - PA4",
                                  nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return 1;
  }
  glfwSetWindowSizeLimits(window, 1050, 700, GLFW_DONT_CARE, GLFW_DONT_CARE);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui::GetIO().IniFilename = nullptr;
  auto &style = ImGui::GetStyle();
  style.WindowRounding = 0;
  style.FrameRounding = 4;
  style.Colors[ImGuiCol_Button] = {.12f, .32f, .45f, 1};
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL2_Init();
  int result = 0;
  {
    Editor editor;
    try {
      editor.init();
      bool smoke = argc > 1 && std::strcmp(argv[1], "--smoke") == 0;
      if (smoke)
        editor.demo();
      double previous = glfwGetTime();
      int frames = 0;
      while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        double now = glfwGetTime();
        float dt = float(std::min(now - previous, .1));
        previous = now;
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        int width, height, fbw, fbh;
        glfwGetWindowSize(window, &width, &height);
        glfwGetFramebufferSize(window, &fbw, &fbh);
        editor.ui(float(width), float(height));
        editor.input(window, dt);
        ImGui::Render();
        glViewport(0, 0, fbw, fbh);
        glClearColor(.08f, .09f, .12f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        float sx = float(fbw) / std::max(width, 1),
              sy = float(fbh) / std::max(height, 1);
        editor.renderer.draw(editor.scene, editor.selected, editor.time,
                             editor.preview, int(editor.viewportX * sx), 0,
                             int(editor.viewportW * sx),
                             int(editor.viewportH * sy));
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        if (smoke && ++frames >= 10)
          break;
      }
      editor.renderer.shutdown();
      std::cout << "Editor cerrado correctamente\n";
    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
      result = 1;
    }
  }
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return result;
}
