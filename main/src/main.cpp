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

#define RES_MAJOR 512
#define RES_MINOR 8


#define Z_RADIUS 10.f

#define PI (float) (M_PI)

struct Unifs {
    glm::mat4 proj = glm::identity<glm::mat4>();
    glm::vec4 color = glm::vec4(1);
};

struct State {
    GLuint vao_wide{};
    GLuint vbo_wide{};
    GLuint ibo_wide{};

    GLuint vao_thin{};
    GLuint vbo_thin{};
    GLuint ibo_thin{};

    GLuint ubo{};

    GLuint prog{};
    const GLuint UNIF_BINDING_POINT = 1;

    std::vector<glm::vec4> verts_wide{};
    std::vector<unsigned> inds_wide{};

    std::vector<glm::vec4> verts_thin{};
    std::vector<unsigned> inds_thin{};

    Unifs unifs;

    glm::vec4 hopf(float xi, float nu, float eta) {
        return glm::vec4(  // todo find parameterization given quaternion
            cos((nu + xi) / 2) * sin(eta),
            sin((nu + xi) / 2) * sin(eta),
            cos((nu - xi) / 2) * cos(eta),
            sin((nu - xi) / 2) * cos(eta)
        );
    }

    glm::vec3 stereo(float xi, float nu, float eta) {
        auto h = hopf(xi, nu, eta);

        auto rot1 = glm::mat4(
            glm::vec4(-0.161583, 0.474819, 0.036271, 0.864373),
            glm::vec4(0.637664, 0.677328, 0.25526, -0.263578),
            glm::vec4(0.392037, 0.015514, -0.914032, 0.103119),
            glm::vec4(0.643116, -0.561741, 0.313199, 0.415657)
        );

        auto rot2 = glm::mat4(
            glm::vec4(+0.656973, +0.386914, +0.646219, +0.032970),
            glm::vec4(-0.459722, +0.569877, +0.159946, -0.662054),
            glm::vec4(-0.268466, -0.653407, +0.675016, -0.212936),
            glm::vec4(-0.533824, +0.314007, +0.318080, +0.717815)
        );

        auto rot3 = glm::mat4(
            glm::vec4(0.794847, -0.598309, -0.101081, -0.00544741),
            glm::vec4(-0.0472779, 0.0524503, -0.719464, 0.690936),
            glm::vec4(0.592606, 0.798574, -0.06232, -0.0849633),
            glm::vec4(0.12167, 0.0394918, 0.684308, 0.717888)
        );

        auto rot4 = glm::mat4(
            glm::vec4(-0.362372, 0.701484, 0.304057, 0.533071),
            glm::vec4(-0.582584, -0.358701, -0.598223, 0.417213),
            glm::vec4(-0.579933, -0.419501, 0.662733, -0.220209),
            glm::vec4(0.43928, -0.450868, 0.332376, 0.702342)
        );

        auto rot = rot2;

        h = rot * h;

        auto s = h.xzy() / (1 - h.w);

        s /= 2;

        return s;
    }

    void
    add_ring(std::vector<glm::vec4> &dest_verts, std::vector<unsigned> &dest_inds, float xi, float eta, float rad) {
        std::vector<glm::vec3> circle;
        std::vector<glm::vec3> torus;
        std::vector<unsigned> ind;

        for (unsigned i = 0; i < RES_MAJOR; ++i) {
            auto nu = 4 * PI * i / RES_MAJOR;

            auto v = stereo(xi, nu, eta);

            circle.push_back(v);
        }

        auto center = glm::vec3(0);
        for (auto v : circle) center += v;
        center /= (float) RES_MAJOR;

        auto A = stereo(xi, 0 * PI, eta);
        auto B = stereo(xi, 1 * PI, eta);
        auto normal = glm::normalize(cross(A - center, B - center));

        for (unsigned i = 0; i < circle.size(); ++i) {
            auto v = circle[i];

            auto b1 = normal;
            auto b2 = glm::normalize(v - center);

            for (int j = 0; j < RES_MINOR; ++j) {
                auto theta = 2 * PI * j / RES_MINOR;

                auto p = v + (cos(theta) * b1 + sin(theta) * b2) * rad;

                ind.push_back(i * RES_MINOR + j);
                ind.push_back((i + 1) * RES_MINOR + j);
                ind.push_back(i * RES_MINOR + (j + 1) % RES_MINOR);

                ind.push_back((i + 1) * RES_MINOR + j);
                ind.push_back((i + 1) * RES_MINOR + (j + 1) % RES_MINOR);
                ind.push_back(i * RES_MINOR + (j + 1) % RES_MINOR);

                torus.push_back(p);
            }
        }

        auto offset = (unsigned) dest_verts.size();

        for (auto v : torus)
            dest_verts.emplace_back(v, 1);

        for (auto i : ind)
            dest_inds.push_back(offset + i % (RES_MAJOR * RES_MINOR));
    }

    void updateUnifs() {
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        util::bufferData(GL_UNIFORM_BUFFER, unifs, GL_STREAM_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void regen() {
        printf("generating\n");

        verts_wide.clear();
        verts_thin.clear();
        inds_wide.clear();
        inds_thin.clear();

        const float XI_R = 32;
        const float ETA_R = 4;
        const float ETA_BUF = 0;

        for (int i = 0; i < XI_R; ++i) {
            float xi = 2 * PI * i / XI_R;

            for (int j = 0; j <= ETA_R; ++j) {
                float eta = ETA_BUF + (PI / 2 - 2 * ETA_BUF) * j / ETA_R;

                add_ring(verts_wide, inds_wide, xi, eta, .01);
                add_ring(verts_thin, inds_thin, xi, eta, .0025);
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo_thin);
        util::bufferData(GL_ARRAY_BUFFER, verts_thin, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_thin);
        util::bufferData(GL_ELEMENT_ARRAY_BUFFER, inds_thin, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_wide);
        util::bufferData(GL_ARRAY_BUFFER, verts_wide, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_wide);
        util::bufferData(GL_ELEMENT_ARRAY_BUFFER, inds_wide, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void init(GLFWwindow *window) {
        GLuint vs = util::buildShader(GL_VERTEX_SHADER, "vs", {"shaders/main.vert"});
        GLuint fs = util::buildShader(GL_FRAGMENT_SHADER, "fs", {"shaders/main.frag"});
        prog = util::buildProgram(false, "prog", {vs, fs});

        glGenBuffers(1, &vbo_wide);
        glGenBuffers(1, &ibo_wide);
        glGenBuffers(1, &vbo_thin);
        glGenBuffers(1, &ibo_thin);

        regen();

        GLint pos = glGetAttribLocation(prog, "iPos");

        glGenBuffers(1, &ubo);
        glBindBufferBase(GL_UNIFORM_BUFFER, UNIF_BINDING_POINT, ubo);
        updateUnifs();

        glGenVertexArrays(1, &vao_wide);
        glBindVertexArray(vao_wide);
        if (pos >= 0) {
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_wide);
            glVertexAttribPointer((GLuint) pos, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_wide);
        glBindVertexArray(0);

        glGenVertexArrays(1, &vao_thin);
        glBindVertexArray(vao_thin);
        if (pos >= 0) {
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_thin);
            glVertexAttribPointer((GLuint) pos, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_thin);
        glBindVertexArray(0);
    }

    void render(GLFWwindow *window, float dt, int frame) {
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);

        glViewport(0, 0, w, h);
        glClearColor(1, 1, 1, 1);
        unifs.color = glm::vec4(0, 0, 0, 1);

        glClear(GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(vao_wide);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        updateUnifs();
        glUseProgram(prog);
        glDrawElements(GL_TRIANGLES, (GLsizei) inds_wide.size(), GL_UNSIGNED_INT, 0);

        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(vao_thin);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        updateUnifs();
        glUseProgram(prog);
        glDrawElements(GL_TRIANGLES, (GLsizei) inds_thin.size(), GL_UNSIGNED_INT, 0);

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

    glfwSetWindowUserPointer(window, state);
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
