#include "Renderer.hpp"
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <limits>
#include <set>
#include <stdexcept>
glm::vec3 Camera::eye() const {
  float y = glm::radians(yaw), p = glm::radians(pitch);
  return target + distance * glm::vec3(std::cos(p) * std::sin(y), std::sin(p),
                                       std::cos(p) * std::cos(y));
}
glm::mat4 Camera::view() const {
  return glm::lookAt(eye(), target, glm::vec3(0, 1, 0));
}
glm::mat4 Camera::projection(float a) const {
  return orthographic ? glm::ortho(-distance * a * .5f, distance * a * .5f,
                                   -distance * .5f, distance * .5f, .05f, 500.f)
                      : glm::perspective(glm::radians(50.f), a, .05f, 500.f);
}
void Renderer::init(const std::string &assets) {
  glGenTextures(3, textures);
  const char *names[] = {"ajedrez.ppm", "ladrillo.ppm", "azul.ppm"};
  for (int i = 0; i < 3; i++) {
    std::ifstream f(assets + "/" + names[i]);
    std::string magic;
    int w = 0, h = 0, max = 0;
    f >> magic >> w >> h >> max;
    if (magic != "P3" || w <= 0 || h <= 0 || max != 255)
      throw std::runtime_error("Textura PPM invalida");
    std::vector<unsigned char> data(size_t(w * h * 3));
    for (auto &c : data) {
      int v;
      f >> v;
      if (!f)
        throw std::runtime_error("Textura incompleta");
      c = static_cast<unsigned char>(v);
    }
    glBindTexture(GL_TEXTURE_2D, textures[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 data.data());
  }
}
void Renderer::shutdown() { glDeleteTextures(3, textures); }
static void meshDraw(const Mesh &m) {
  glBegin(GL_TRIANGLES);
  for (auto &v : m.vertices) {
    glNormal3fv(glm::value_ptr(v.normal));
    glTexCoord2fv(glm::value_ptr(v.uv));
    glVertex3fv(glm::value_ptr(v.position));
  }
  glEnd();
}
void Renderer::draw(const Scene &s, int selected, float time, bool animate,
                    int x, int y, int width, int height) {
  if (width <= 0 || height <= 0)
    return;
  glViewport(x, y, width, height);
  glEnable(GL_SCISSOR_TEST);
  glScissor(x, y, width, height);
  glClearColor(.065f, .082f, .11f, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glEnable(GL_NORMALIZE);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);
  auto p = camera.projection(float(width) / height), v = camera.view();
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(glm::value_ptr(p));
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(glm::value_ptr(v));
  if (grid) {
    glColor3f(.22f, .26f, .32f);
    glBegin(GL_LINES);
    for (int i = -10; i <= 10; i++) {
      glVertex3f(float(i), 0, -10);
      glVertex3f(float(i), 0, 10);
      glVertex3f(-10, 0, float(i));
      glVertex3f(10, 0, float(i));
    }
    glEnd();
    glLineWidth(2);
    glBegin(GL_LINES);
    glColor3f(1, .25f, .25f);
    glVertex3f(0, .01f, 0);
    glVertex3f(3, .01f, 0);
    glColor3f(.3f, 1, .3f);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 3, 0);
    glColor3f(.3f, .5f, 1);
    glVertex3f(0, .01f, 0);
    glVertex3f(0, .01f, 3);
    glEnd();
    glLineWidth(1);
  }
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  GLfloat pos[] = {s.light.position.x, s.light.position.y, s.light.position.z,
                   1};
  GLfloat amb[] = {s.light.ambient, s.light.ambient, s.light.ambient, 1};
  GLfloat light[] = {s.light.color.x * s.light.intensity,
                     s.light.color.y * s.light.intensity,
                     s.light.color.z * s.light.intensity, 1};
  glLightfv(GL_LIGHT0, GL_POSITION, pos);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light);
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
  std::set<int> hits;
  if (collisionEnabled)
    for (auto pair : s.collisions(time, animate)) {
      hits.insert(pair.first);
      hits.insert(pair.second);
    }
  for (auto &o : s.objects) {
    auto model = s.world(o.id, time, animate);
    glPushMatrix();
    glMultMatrixf(glm::value_ptr(model));
    auto &m = o.material;
    GLfloat a[] = {m.color.x * m.ambient, m.color.y * m.ambient,
                   m.color.z * m.ambient, 1},
            d[] = {m.color.x * m.diffuse, m.color.y * m.diffuse,
                   m.color.z * m.diffuse, 1},
            sp[] = {m.specular, m.specular, m.specular, 1};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, a);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, d);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, sp);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS,
                glm::clamp(m.shininess, 0.f, 128.f));
    if (m.texture > 0 && m.texture <= 3) {
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, textures[m.texture - 1]);
    } else
      glDisable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    meshDraw(o.mesh);
    if (o.id == selected || hits.count(o.id)) {
      glDisable(GL_LIGHTING);
      glDisable(GL_TEXTURE_2D);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glEnable(GL_POLYGON_OFFSET_LINE);
      glPolygonOffset(-1, -1);
      glLineWidth(2);
      if (hits.count(o.id))
        glColor3f(1, .2f, .2f);
      else
        glColor3f(1, .75f, .15f);
      meshDraw(o.mesh);
      glLineWidth(1);
      glDisable(GL_POLYGON_OFFSET_LINE);
      glEnable(GL_LIGHTING);
    }
    glPopMatrix();
  }
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_SCISSOR_TEST);
}
// Moller-Trumbore ray/triangle intersections in world coordinates.
int Renderer::pick(const Scene &s, float time, bool animate, float nx, float ny,
                   float aspect) const {
  auto inv = glm::inverse(camera.projection(aspect) * camera.view());
  auto a = inv * glm::vec4(nx, ny, -1, 1), b = inv * glm::vec4(nx, ny, 1, 1);
  glm::vec3 origin = glm::vec3(a) / a.w,
            dir = glm::normalize(glm::vec3(b) / b.w - origin);
  float nearest = std::numeric_limits<float>::max();
  int selected = -1;
  for (auto &o : s.objects) {
    auto m = s.world(o.id, time, animate);
    auto &vs = o.mesh.vertices;
    for (size_t i = 0; i + 2 < vs.size(); i += 3) {
      auto p = glm::vec3(m * glm::vec4(vs[i].position, 1)),
           q = glm::vec3(m * glm::vec4(vs[i + 1].position, 1)),
           r = glm::vec3(m * glm::vec4(vs[i + 2].position, 1));
      auto e1 = q - p, e2 = r - p, h = glm::cross(dir, e2);
      float det = glm::dot(e1, h);
      if (std::abs(det) < 1e-7f)
        continue;
      float invDet = 1 / det;
      auto t = origin - p;
      float u = glm::dot(t, h) * invDet;
      if (u < 0 || u > 1)
        continue;
      auto cross = glm::cross(t, e1);
      float v = glm::dot(dir, cross) * invDet;
      if (v < 0 || u + v > 1)
        continue;
      float distance = glm::dot(e2, cross) * invDet;
      if (distance > 0 && distance < nearest) {
        nearest = distance;
        selected = o.id;
      }
    }
  }
  return selected;
}
