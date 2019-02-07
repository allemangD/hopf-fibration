//
// Created by allem on 2/7/2019.
//

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdlib>
#include <math.h>
#include <string>
#include <vector>

#include "util.h"

#define RES_MAJOR 256
#define RES_MINOR 8

#define WIDTH_BARR .050f
#define WIDTH_LINE .02f

#define Z_RADIUS 10.f

#define PI (float) (M_PI)

struct Unifs {
    glm::mat4 proj = glm::identity<glm::mat4>();
    glm::vec4 color = glm::vec4(1);
};

struct State {
    GLuint vao{};
    GLuint vbo{};
    GLuint ibo{};
    GLuint ubo{};

    GLuint prog{};
    const GLuint UNIF_BINDING_POINT = 1;

    std::vector<glm::vec4> verts{};
    std::vector<unsigned> inds{};
    Unifs unifs;

    void add_ring(float xi, float eta) {
        std::vector<glm::vec4> _verts;
        std::vector<unsigned> _inds;

        for (int i = 0; i < RES_MAJOR; ++i) {
            auto nu = 4 * PI * i / RES_MAJOR;

            auto v = glm::vec4(  // todo find parameterization given quaternion
                cos((nu + xi) / 2) * sin(eta),
                sin((nu + xi) / 2) * sin(eta),
                cos((nu - xi) / 2) * cos(eta),
                sin((nu - xi) / 2) * cos(eta)
            );

            // todo build torus about projection
            _verts.emplace_back(v.xzy() / (1.f - v.w), 1);
        }

        _inds.push_back(0);
        for (unsigned i = 1; i < RES_MAJOR; ++i) {
            _inds.push_back(i);
            _inds.push_back(i);
        }
        _inds.push_back(0);

        auto offset = (unsigned) verts.size();

        for (auto v: _verts)
            verts.push_back(v);
        for (auto i : _inds)
            inds.push_back(offset + i);
    }

    void updateUnifs() {
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        util::bufferData(GL_UNIFORM_BUFFER, unifs, GL_STREAM_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void init(GLFWwindow *window) {
        GLuint vs = util::buildShader(GL_VERTEX_SHADER, "vs", {"shaders/main.vert"});
        GLuint fs = util::buildShader(GL_FRAGMENT_SHADER, "fs", {"shaders/main.frag"});
        prog = util::buildProgram(false, "prog", {vs, fs});

        for (int i = 0; i < 32; ++i) {
            for (int j = 0; j <= 4; ++j) {
                float xi = 2 * PI * i / 32;
                const float buf = 0.0f;
                float eta = buf + (PI / 2 - 2 * buf) * j / 4;
                add_ring(xi, eta);
            }
        }

        glGenBuffers(1, &ubo);
        glBindBufferBase(GL_UNIFORM_BUFFER, UNIF_BINDING_POINT, ubo);
        updateUnifs();

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        util::bufferData(GL_ARRAY_BUFFER, verts, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        util::bufferData(GL_ELEMENT_ARRAY_BUFFER, inds, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        GLint pos = glGetAttribLocation(prog, "iPos");
        if (pos >= 0) {
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glVertexAttribPointer((GLuint) pos, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBindVertexArray(0);
    }

    void render(GLFWwindow *window, float dt, int frame) {
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);

        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(vao);
        unifs.color = glm::vec4(1, 0, 0, 1);
        updateUnifs();
        glUseProgram(prog);
        glDrawElements(GL_LINES, (GLsizei) inds.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
    }

    void update(GLFWwindow *window, float dt, int frame) {
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);

        float ar = (float) w / (float) h;
        unifs.proj = glm::ortho(-ar, ar, -1.f, 1.f, -Z_RADIUS, Z_RADIUS);
    }

    void deinit(GLFWwindow *window) {

    }
};

void run(State *state, const std::string &title) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    auto window = glfwCreateWindow(1280, 720, title.c_str(), nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(0);

    state->init(window);

    double time = glfwGetTime();
    int frame = 0;
    while (!glfwWindowShouldClose(window)) {
        double time_ = glfwGetTime();
        auto dt = (float) (time_ - time);

        state->update(window, dt, frame);
        state->render(window, dt, frame);

        glfwPollEvents();

        time = time_;
        frame += 1;
    }

    state->deinit(window);

    glfwDestroyWindow(window);
}

int main() {
    if (!glfwInit()) {
        return EXIT_FAILURE;
    }

    auto *state = new State();
    run(state, "Hopf Fibration");
    delete state;

    glfwTerminate();

    return EXIT_SUCCESS;
}
