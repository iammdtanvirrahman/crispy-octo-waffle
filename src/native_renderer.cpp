#include "native_renderer.hpp"

#if defined(VOXEL_NATIVE_GLFW)

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <utility>
#include <vector>

namespace voxel {
namespace {

struct GLMesh {
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;
    std::size_t indexCount = 0;
};

void matIdentity(float* m) {
    for (int i = 0; i < 16; ++i) m[i] = 0.0f;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void matPerspective(float fovy, float aspect, float znear, float zfar, float* m) {
    const float f = 1.0f / std::tan(fovy * 0.5f);
    for (int i = 0; i < 16; ++i) m[i] = 0.0f;
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zfar + znear) / (znear - zfar);
    m[11] = -1.0f;
    m[14] = (2.0f * zfar * znear) / (znear - zfar);
}

void matView(const Camera& c, float* m) {
    const float cy = std::cos(c.yaw), sy = std::sin(c.yaw);
    const float cp = std::cos(c.pitch), sp = std::sin(c.pitch);
    const float r00 = cy,     r01 = sy * sp,  r02 = sy * cp;
    const float r10 = 0.0f,   r11 = cp,       r12 = -sp;
    const float r20 = -sy,    r21 = cy * sp,  r22 = cy * cp;
    matIdentity(m);
    m[0] = r00; m[4] = r10; m[8] = r20;
    m[1] = r01; m[5] = r11; m[9] = r21;
    m[2] = r02; m[6] = r12; m[10] = r22;
    m[12] = -(r00 * c.position.x + r10 * c.position.y + r20 * c.position.z);
    m[13] = -(r01 * c.position.x + r11 * c.position.y + r21 * c.position.z);
    m[14] = -(r02 * c.position.x + r12 * c.position.y + r22 * c.position.z);
}

GLuint compile(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048]{};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Shader compile failed: " << log << '\n';
    }
    return shader;
}

GLuint makeProgram() {
    static constexpr const char* vs = R"glsl(
        #version 330 core
        layout(location=0) in vec3 aPos;
        layout(location=1) in vec3 aNormal;
        layout(location=2) in vec2 aUV;
        layout(location=3) in float aBlock;
        uniform mat4 uVP;
        out vec3 vNormal;
        flat out int vBlock;
        void main(){
            gl_Position = uVP * vec4(aPos,1.0);
            vNormal = aNormal;
            vBlock = int(aBlock);
        }
    )glsl";
    static constexpr const char* fs = R"glsl(
        #version 330 core
        in vec3 vNormal;
        flat in int vBlock;
        out vec4 FragColor;
        vec3 palette(int b){
            if(b==1) return vec3(0.36,0.63,0.24);
            if(b==2) return vec3(0.54,0.35,0.20);
            if(b==3) return vec3(0.48,0.51,0.54);
            if(b==4) return vec3(0.78,0.70,0.43);
            if(b==5) return vec3(0.58,0.40,0.22);
            if(b==6) return vec3(0.22,0.55,0.27);
            if(b==7) return vec3(0.45,0.78,0.80);
            if(b==8) return vec3(0.17,0.15,0.12);
            if(b==9) return vec3(0.28,0.48,0.18);
            return vec3(0.65);
        }
        void main(){
            vec3 n = normalize(vNormal);
            float light = 0.55 + 0.45 * max(dot(n, normalize(vec3(0.35,0.8,0.2))), 0.0);
            FragColor = vec4(palette(vBlock) * light, 1.0);
        }
    )glsl";
    GLuint p = glCreateProgram();
    GLuint v = compile(GL_VERTEX_SHADER, vs);
    GLuint f = compile(GL_FRAGMENT_SHADER, fs);
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

} // namespace

struct NativeRenderer::Impl {
    GLFWwindow* window = nullptr;
    GLuint program = 0;
    GLMesh mesh{};
    NativeRenderer* owner = nullptr;
};

NativeRenderer::NativeRenderer(int width, int height, std::string title)
    : impl_(std::make_unique<Impl>()) {
    impl_->owner = this;
    if (!glfwInit()) return;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    impl_->window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!impl_->window) { glfwTerminate(); return; }
    glfwMakeContextCurrent(impl_->window);
    glfwSwapInterval(0);
    glfwSetInputMode(impl_->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowUserPointer(impl_->window, this);
    glfwSetCursorPosCallback(impl_->window, [](GLFWwindow* w, double x, double y) {
        auto* self = static_cast<NativeRenderer*>(glfwGetWindowUserPointer(w));
        static double lastX = x;
        static double lastY = y;
        if (!self) return;
        self->mouseDX_ += static_cast<float>(x - lastX);
        self->mouseDY_ += static_cast<float>(y - lastY);
        lastX = x;
        lastY = y;
    });
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    impl_->program = makeProgram();
}

NativeRenderer::~NativeRenderer() {
    if (!impl_) return;
    if (impl_->mesh.ebo) glDeleteBuffers(1, &impl_->mesh.ebo);
    if (impl_->mesh.vbo) glDeleteBuffers(1, &impl_->mesh.vbo);
    if (impl_->mesh.vao) glDeleteVertexArrays(1, &impl_->mesh.vao);
    if (impl_->program) glDeleteProgram(impl_->program);
    if (impl_->window) glfwDestroyWindow(impl_->window);
    glfwTerminate();
}

bool NativeRenderer::valid() const noexcept { return impl_ && impl_->window && impl_->program; }
bool NativeRenderer::shouldClose() const noexcept { return !valid() || glfwWindowShouldClose(impl_->window); }

void NativeRenderer::pollEvents() noexcept { if (valid()) glfwPollEvents(); }
void NativeRenderer::beginFrame() noexcept {
    if (!valid()) return;
    glClearColor(0.53f, 0.74f, 0.96f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void NativeRenderer::endFrame() noexcept { if (valid()) glfwSwapBuffers(impl_->window); }

void NativeRenderer::uploadMesh(const Mesh& mesh) {
    if (!valid()) return;
    if (!impl_->mesh.vao) glGenVertexArrays(1, &impl_->mesh.vao);
    if (!impl_->mesh.vbo) glGenBuffers(1, &impl_->mesh.vbo);
    if (!impl_->mesh.ebo) glGenBuffers(1, &impl_->mesh.ebo);
    glBindVertexArray(impl_->mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, impl_->mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex)), mesh.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, impl_->mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(std::uint32_t)), mesh.indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,px)));
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,nx)));
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,u)));
    glVertexAttribPointer(3,1,GL_UNSIGNED_BYTE,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,block)));
    glEnableVertexAttribArray(0); glEnableVertexAttribArray(1); glEnableVertexAttribArray(2); glEnableVertexAttribArray(3);
    impl_->mesh.indexCount = mesh.indices.size();
}

void NativeRenderer::draw(const Camera& camera, float aspect) noexcept {
    if (!valid() || impl_->mesh.indexCount == 0) return;
    float p[16], v[16], vp[16];
    matPerspective(camera.fov * 3.14159265f / 180.0f, std::max(aspect,0.01f), camera.nearPlane, camera.farPlane, p);
    matView(camera, v);
    for(int col=0; col<4; ++col) for(int row=0; row<4; ++row){
        vp[col*4+row]=0;
        for(int k=0;k<4;++k) vp[col*4+row]+=p[k*4+row]*v[col*4+k];
    }
    glUseProgram(impl_->program);
    glUniformMatrix4fv(glGetUniformLocation(impl_->program,"uVP"),1,GL_FALSE,vp);
    glBindVertexArray(impl_->mesh.vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(impl_->mesh.indexCount), GL_UNSIGNED_INT, nullptr);
}

} // namespace voxel

#else

namespace voxel {
struct NativeRenderer::Impl {};
NativeRenderer::NativeRenderer(int, int, std::string) : impl_(std::make_unique<Impl>()) {}
NativeRenderer::~NativeRenderer() = default;
bool NativeRenderer::valid() const noexcept { return false; }
bool NativeRenderer::shouldClose() const noexcept { return true; }
void NativeRenderer::beginFrame() noexcept {}
void NativeRenderer::endFrame() noexcept {}
void NativeRenderer::uploadMesh(const Mesh&) {}
void NativeRenderer::draw(const Camera&, float) noexcept {}
void NativeRenderer::pollEvents() noexcept {}
} // namespace voxel

#endif
