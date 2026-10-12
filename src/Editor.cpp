    #include "Editor.hpp"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <imgui.h>
void Editor::init() {
  renderer.init(ASSET_DIR);
  std::snprintf(objPath, sizeof(objPath), "%s/piramide.obj", ASSET_DIR);
}
void Editor::demo() {
  scene = Scene{};
  int floor = scene.add("Plano");
  scene.get(floor)->transform.scale = {8, 1, 8};
  scene.get(floor)->material.texture = 1;
  scene.get(floor)->material.color = {1, 1, 1};
  int body = scene.add("Cubo");
  auto *o = scene.get(body);
  o->name = "Base articulada";
  o->transform.position = {0, 1, 0};
  o->transform.scale = {1, 1, 1};
  o->animation.enabled = true;
  o->animation.to = {0, 360, 0};
  int arm = scene.add("Cilindro");
  o = scene.get(arm);
  o->name = "Brazo hijo";
  o->transform.position = {0.7f, 1.5f, 0};
  o->transform.scale = {.5f, 2, .5f};
  scene.setParent(arm, body);
  int ball = scene.add("Esfera");
  scene.get(ball)->transform.position = {2, .5f, 0};
  selected = body;
  time = 0;
  preview = false;
  playing = false;
  status = "Ejemplo creado. Puedes editarlo o crear una escena vacia.";
}
void Editor::ui(float width, float height) {
  float left = 240, right = 310, top = 52;
  viewportX = left;
  viewportY = top;
  viewportW = std::max(1.f, width - left - right);
  viewportH = std::max(1.f, height - top);
  auto fixed = [&](const char *title, float x, float y, float w, float h) {
    ImGui::SetNextWindowPos({x, y});
    ImGui::SetNextWindowSize({w, h});
    ImGui::Begin(title, nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoCollapse);
  };
  fixed("Editor de Escenas 3D | PA4", 0, 0, width, top);
  if (ImGui::Button("Nueva escena")) {
    scene = Scene{};
    selected = -1;
    time = 0;
    playing = preview = false;
    status = "Escena vacia.";
  }
  ImGui::SameLine();
  if (ImGui::Button("Ejemplo"))
    demo();
  ImGui::SameLine();
  ImGui::Checkbox("Ortogonal", &renderer.camera.orthographic);
  ImGui::SameLine();
  ImGui::Checkbox("Wireframe", &renderer.wireframe);
  ImGui::SameLine();
  ImGui::Checkbox("Cuadricula", &renderer.grid);
  ImGui::End();
  fixed("Objetos y archivos", 0, top, left, height - top);
  ImGui::TextUnformatted("Crear primitivas");
  for (const char *type : {"Cubo", "Esfera", "Cilindro", "Plano"}) {
    if (ImGui::Button(type, {100, 0})) {
      selected = scene.add(type);
      scene.get(selected)->transform.position.y = .5f;
    }
    if (std::string(type) == "Cubo" || std::string(type) == "Cilindro")
      ImGui::SameLine();
  }
  ImGui::Separator();
  ImGui::TextUnformatted("Escena (selecciona un objeto)");
  ImGui::BeginChild("lista", {0, 180}, true);
  for (auto &o : scene.objects) {
    std::string label = o.name + "##" + std::to_string(o.id);
    if (ImGui::Selectable(label.c_str(), o.id == selected))
      selected = o.id;
    if (o.parent != -1) {
      ImGui::Indent();
      ImGui::TextDisabled("Hijo de #%d", o.parent);
      ImGui::Unindent();
    }
  }
  ImGui::EndChild();
  if (ImGui::Button("Duplicar") && selected != -1)
    selected = scene.duplicate(selected);
  ImGui::SameLine();
  if (ImGui::Button("Eliminar") && selected != -1) {
    scene.remove(selected);
    selected = -1;
    status = "Objeto y descendientes eliminados.";
  }
  ImGui::Separator();
  ImGui::TextUnformatted("Modelo OBJ (ruta completa)");
  ImGui::InputText("##obj", objPath, sizeof(objPath));
  if (ImGui::Button("Importar OBJ")) {
    try {
      selected = scene.add("OBJ", objPath);
      status = "OBJ importado.";
    } catch (const std::exception &e) {
      status = e.what();
    }
  }
  ImGui::Separator();
  ImGui::TextUnformatted("Archivo de escena JSON");
  ImGui::InputText("##scene", scenePath, sizeof(scenePath));
  if (ImGui::Button("Guardar")) {
    try {
      scene.save(scenePath);
      status = "Guardado en " + std::filesystem::absolute(scenePath).string();
    } catch (const std::exception &e) {
      status = e.what();
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Cargar")) {
    try {
      scene.load(scenePath);
      selected = -1;
      time = 0;
      playing = preview = false;
      status = "Escena recuperada.";
    } catch (const std::exception &e) {
      status = e.what();
    }
  }
  ImGui::Separator();
  ImGui::TextWrapped("%s", status.c_str());
  ImGui::Separator();
  ImGui::TextWrapped(
      "Camara: boton derecho + mover raton. Rueda: zoom. WASD: desplazar. Q/E: "
      "subir/bajar. F: enfocar. Clic izquierdo: seleccionar.");
  ImGui::End();
  fixed("Propiedades", width - right, top, right, height - top);
  if (auto *o = scene.get(selected)) {
    char name[128];
    std::snprintf(name, sizeof(name), "%s", o->name.c_str());
    if (ImGui::InputText("Nombre", name, sizeof(name)))
      o->name = name;
    ImGui::Text("Tipo: %s | ID: %d", o->type.c_str(), o->id);
    ImGui::SeparatorText("Transformacion local");
    ImGui::DragFloat3("Posicion", &o->transform.position.x, .05f);
    ImGui::DragFloat3("Rotacion", &o->transform.rotation.x, 1);
    ImGui::DragFloat3("Escala", &o->transform.scale.x, .02f, .01f, 100, "%.2f",
                      ImGuiSliderFlags_AlwaysClamp);
    const auto *parent = scene.get(o->parent);
    if (ImGui::BeginCombo("Padre",
                          parent ? parent->name.c_str() : "Sin padre")) {
      if (ImGui::Selectable("Sin padre", o->parent == -1))
        scene.setParent(o->id, -1);
      for (auto &candidate : scene.objects)
        if (candidate.id != o->id) {
          std::string label =
              candidate.name + "##parent" + std::to_string(candidate.id);
          if (ImGui::Selectable(label.c_str(), candidate.id == o->parent) &&
              !scene.setParent(o->id, candidate.id))
            status = "Relacion rechazada: produciria un ciclo.";
        }
      ImGui::EndCombo();
    }
    ImGui::TextWrapped("Transformaciones relativas al padre.");
    if (ImGui::CollapsingHeader("Material y textura",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::ColorEdit3("Color", &o->material.color.x);
      ImGui::SliderFloat("Ambiental", &o->material.ambient, 0, 1);
      ImGui::SliderFloat("Difusa", &o->material.diffuse, 0, 1);
      ImGui::SliderFloat("Especular", &o->material.specular, 0, 1);
      ImGui::SliderFloat("Brillo", &o->material.shininess, 1, 128);
      const char *names[] = {"Sin textura", "Ajedrez", "Ladrillo", "Azul"};
      ImGui::Combo("Textura", &o->material.texture, names, 4);
    }
    if (ImGui::CollapsingHeader("Animacion", ImGuiTreeNodeFlags_DefaultOpen)) {
      auto &a = o->animation;
      ImGui::Checkbox("Activar animacion", &a.enabled);
      const char *channels[] = {"Posicion", "Rotacion", "Escala"};
      if (ImGui::Combo("Propiedad", &a.channel, channels, 3)) {
        a.from = a.channel == 0   ? o->transform.position
                 : a.channel == 1 ? o->transform.rotation
                                  : o->transform.scale;
        a.to = a.from;
        if (a.channel == 1)
          a.to.y += 360;
        else if (a.channel == 0)
          a.to.x += 3;
        else
          a.to *= 2;
      }
      ImGui::DragFloat3("Desde", &a.from.x, .1f);
      ImGui::DragFloat3("Hasta", &a.to.x, .1f);
      ImGui::DragFloat("Duracion (s)", &a.duration, .1f, .1f, 120, "%.1f",
                       ImGuiSliderFlags_AlwaysClamp);
      ImGui::Checkbox("Repetir", &a.loop);
    }
  } else
    ImGui::TextWrapped(
        "Selecciona un objeto en la lista o en el viewport para editarlo.");
  if (ImGui::CollapsingHeader("Iluminacion")) {
    ImGui::DragFloat3("Posicion luz", &scene.light.position.x, .1f);
    ImGui::ColorEdit3("Color luz", &scene.light.color.x);
    ImGui::SliderFloat("Ambiente global", &scene.light.ambient, 0, 1);
    ImGui::SliderFloat("Intensidad", &scene.light.intensity, 0, 3);
  }
  if (ImGui::CollapsingHeader("Colisiones AABB")) {
    ImGui::Checkbox("Detectar y resaltar", &renderer.collisionEnabled);
    if (renderer.collisionEnabled) {
      auto hits = scene.collisions(time, preview);
      ImGui::Text("Pares en contacto: %d", int(hits.size()));
      for (auto pair : hits)
        ImGui::Text("#%d <-> #%d", pair.first, pair.second);
    }
    ImGui::TextWrapped("Cajas alineadas a ejes del mundo. Deteccion "
                       "aproximada; no aplica respuesta fisica.");
  }
  ImGui::SeparatorText("Reproduccion");
  if (ImGui::Button("Reproducir")) {
    playing = true;
    preview = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Pausar"))
    playing = false;
  if (ImGui::Button("Detener")) {
    playing = false;
    preview = false;
    time = 0;
  }
  ImGui::SameLine();
  if (ImGui::Button("Reiniciar")) {
    time = 0;
    preview = true;
  }
  ImGui::Text("Tiempo: %.2f s", time);
  ImGui::End();
}
void Editor::input(GLFWwindow *window, float dt) {
  auto &io = ImGui::GetIO();
  bool inside =
      io.MousePos.x >= viewportX && io.MousePos.x < viewportX + viewportW &&
      io.MousePos.y >= viewportY && io.MousePos.y < viewportY + viewportH;
  auto &c = renderer.camera;
  if (inside && !io.WantCaptureMouse) {
    if (ImGui::IsMouseDown(1)) {
      c.yaw -= io.MouseDelta.x * .3f;
      c.pitch = glm::clamp(c.pitch + io.MouseDelta.y * .3f, -85.f, 85.f);
    }
    c.distance = glm::clamp(c.distance - io.MouseWheel * .7f, .5f, 100.f);
    if (ImGui::IsMouseClicked(0)) {
      float nx = 2 * (io.MousePos.x - viewportX) / viewportW - 1,
            ny = 1 - 2 * (io.MousePos.y - viewportY) / viewportH;
      selected =
          renderer.pick(scene, time, preview, nx, ny, viewportW / viewportH);
    }
  }
  if (inside && !io.WantCaptureKeyboard) {
    glm::vec3 forward = c.target - c.eye();
    forward.y = 0;
    forward = glm::normalize(forward);
    auto right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    float speed = dt * 4;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      c.target += forward * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      c.target -= forward * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      c.target -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      c.target += right * speed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
      c.target.y -= speed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
      c.target.y += speed;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && scene.get(selected))
      c.target = glm::vec3(scene.world(selected, time, preview) *
                           glm::vec4(0, 0, 0, 1));
  }
  if (playing)
    time += dt;
}
