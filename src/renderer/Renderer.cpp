#include "renderer/Renderer.h"

#include "core/Log.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace helios::renderer {
namespace {
unsigned int Compile(unsigned int type, const char* src) {
  unsigned int shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);
  int ok = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char buffer[1024]{};
    glGetShaderInfoLog(shader, 1024, nullptr, buffer);
    core::Log::Message(core::LogLevel::Error, std::string("Shader compile failed: ") + buffer);
  }
  return shader;
}
} // namespace

bool Renderer::Init() {
  const char* vs = R"(
    #version 410 core
    layout(location = 0) in vec3 aPos;
    uniform mat4 uMVP;
    void main() { gl_Position = uMVP * vec4(aPos, 1.0); }
  )";
  const char* fs = R"(
    #version 410 core
    uniform vec4 uColor;
    out vec4 FragColor;
    void main() { FragColor = uColor; }
  )";
  const unsigned int v = Compile(GL_VERTEX_SHADER, vs);
  const unsigned int f = Compile(GL_FRAGMENT_SHADER, fs);
  m_Program = glCreateProgram();
  glAttachShader(m_Program, v);
  glAttachShader(m_Program, f);
  glLinkProgram(m_Program);
  glDeleteShader(v);
  glDeleteShader(f);

  static constexpr float cube[] = {
    -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
     0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
    -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
     0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
    -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
     0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
     0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
     0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f,
    -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
     0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f
  };
  glGenVertexArrays(1, &m_Vao);
  glGenBuffers(1, &m_Vbo);
  glBindVertexArray(m_Vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);
  glEnable(GL_DEPTH_TEST);
  Resize(m_Width, m_Height);
  return true;
}

void Renderer::Shutdown() {
  if (m_DepthStencilRbo) glDeleteRenderbuffers(1, &m_DepthStencilRbo);
  if (m_ColorAttachment) glDeleteTextures(1, &m_ColorAttachment);
  if (m_Framebuffer) glDeleteFramebuffers(1, &m_Framebuffer);
  if (m_Vbo) glDeleteBuffers(1, &m_Vbo);
  if (m_Vao) glDeleteVertexArrays(1, &m_Vao);
  if (m_Program) glDeleteProgram(m_Program);
}

void Renderer::Resize(int width, int height) {
  width = width > 0 ? width : 1;
  height = height > 0 ? height : 1;
  if (width == m_Width && height == m_Height && m_Framebuffer != 0) return;
  m_Width = width;
  m_Height = height;

  if (!m_Framebuffer) glGenFramebuffers(1, &m_Framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

  if (!m_ColorAttachment) glGenTextures(1, &m_ColorAttachment);
  glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);

  if (!m_DepthStencilRbo) glGenRenderbuffers(1, &m_DepthStencilRbo);
  glBindRenderbuffer(GL_RENDERBUFFER, m_DepthStencilRbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Width, m_Height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthStencilRbo);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    core::Log::Message(core::LogLevel::Error, "Viewport framebuffer is incomplete");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::BeginFrame(const glm::vec4& clearColor) {
  glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
  glViewport(0, 0, m_Width, m_Height);
  glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  m_Stats.objectCount = 0;
  m_Stats.triangleCount = 0;
  m_Stats.drawCalls = 0;
}

void Renderer::RenderScene(const scene::Scene& scene, const EditorCamera& camera, ViewMode mode, bool /*showBounds*/) {
  glUseProgram(m_Program);
  glBindVertexArray(m_Vao);
  glPolygonMode(GL_FRONT_AND_BACK, mode == ViewMode::Wireframe ? GL_LINE : GL_FILL);
  const glm::mat4 vp = camera.Projection() * camera.View();
  const int mvpLoc = glGetUniformLocation(m_Program, "uMVP");
  const int colorLoc = glGetUniformLocation(m_Program, "uColor");

  for (const auto& e : scene.Entities()) {
    if (!e.enabled || e.hidden) continue;
    glm::mat4 model(1.0f);
    model = glm::translate(model, e.transform.position);
    model = glm::rotate(model, e.transform.rotation.x, glm::vec3(1, 0, 0));
    model = glm::rotate(model, e.transform.rotation.y, glm::vec3(0, 1, 0));
    model = glm::rotate(model, e.transform.rotation.z, glm::vec3(0, 0, 1));
    model = glm::scale(model, e.transform.scale);
    const glm::mat4 mvp = vp * model;
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);
    glUniform4f(colorLoc, e.material.color.r, e.material.color.g, e.material.color.b, e.material.color.a);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    ++m_Stats.objectCount;
    m_Stats.triangleCount += 12;
    ++m_Stats.drawCalls;
  }
}

void Renderer::EndFrame() {
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
const RendererStats& Renderer::Stats() const { return m_Stats; }
void Renderer::PushFrameTime(float ms) {
  m_Stats.frameGraph[m_FrameCursor] = ms;
  m_FrameCursor = (m_FrameCursor + 1) % m_Stats.frameGraph.size();
  m_Stats.fps = ms > 0.0f ? 1000.0f / ms : 0.0f;
}
} // namespace helios::renderer
