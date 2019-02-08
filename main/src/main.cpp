#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>

#include "glga.hpp"

#define PI 3.14159f

namespace util {
    template<typename T>
    void bufferData(GLenum target, std::vector<T> data, GLenum usage) {
        glBufferData(target, data.size() * sizeof(T), &data.front(), usage);
    }

    template<typename T>
    void bufferData(GLenum target, T &data, GLenum usage) {
        glBufferData(target, sizeof(T), &data, usage);
    }

    std::string readFile(const std::string &path) {
        std::ifstream file(path);
        if (!file) return std::string();

        file.ignore(std::numeric_limits<std::streamsize>::max());
        auto size = file.gcount();

        if (size > 0x10000) return std::string();

        file.clear();
        file.seekg(0, std::ios_base::beg);

        std::stringstream sstr;
        sstr << file.rdbuf();
        file.close();

        return sstr.str();
    }

    void shaderFiles(GLuint shader, std::vector<std::string> &paths) {
        std::vector<std::string> strs;
        std::vector<const char *> c_strs;

        for (const auto &path : paths) strs.push_back(readFile(path));
        for (const auto &str:strs) c_strs.push_back(str.c_str());

        glShaderSource(shader, (GLsizei) c_strs.size(), &c_strs.front(), nullptr);
    }

    std::string shaderInfoLog(GLuint shader) {
        GLint log_len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
        char log[log_len];
        glGetShaderInfoLog(shader, log_len, nullptr, log);
        return std::string(log);
    }

    std::string programInfoLog(GLuint program) {
        GLint log_len;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
        char log[log_len];
        glGetProgramInfoLog(program, log_len, nullptr, log);
        return std::string(log);
    }

    GLuint buildShader(GLenum kind, const std::string &name, std::vector<std::string> paths) {
        GLuint shader = glCreateShader(kind);
        shaderFiles(shader, paths);

        glCompileShader(shader);

        GLint comp;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &comp);
        if (!comp) {
            std::string log = shaderInfoLog(shader);
            fprintf(stderr, "SHADER ERROR (%s):\n%s", name.c_str(), log.c_str());
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    GLuint buildProgram(bool separable, const std::string &name, std::vector<GLuint> shaders) {
        GLuint program = glCreateProgram();

        if (separable)
            glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE);

        for (GLuint shader : shaders)
            glAttachShader(program, shader);

        glLinkProgram(program);

        GLint link;
        glGetProgramiv(program, GL_LINK_STATUS, &link);
        if (!link) {
            std::string log = programInfoLog(program);
            fprintf(stderr, "PROGRAM ERROR (%s):\n%s", name.c_str(), log.c_str());
            glDeleteProgram(program);
            return 0;
        }

        return program;
    }
}

struct HopfCircle {
    float xi; // longitude
    float eta; // latitude

    HopfCircle(float xi, float eta) : xi(xi), eta(eta) {}

    HopfCircle() : HopfCircle(0, 0) {}
};

struct State {
    GLuint vao{};
    GLuint vbo{}, ubo{};
    GLuint unif_bright{};
    GLuint ubo_bp = 1;

    GLuint prog{};

    std::vector<HopfCircle> circles{};
    ga::Mat rot = ga::unit::identity();
    float view = 4.f;

    float t = 0;
    float dt = 0;

    void regen() {
        circles.clear();

        const int N = 32;
        const int M = 4;
        const float PAD = 0.0125;

        for (int x = 0; x <= N; ++x) {
            for (int e = 1; e < M; ++e) {
                float xi = 2 * PI * x / N;
                float eta = (PI / 2 - 2 * PAD) * e / M + PAD;
                circles.emplace_back(xi, eta);
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        util::bufferData(GL_ARRAY_BUFFER, circles, GL_STREAM_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    explicit State(GLFWwindow *window) {
        printf("vendor: %s\nrenderer: %s\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER));

        float lw[2];
        glGetFloatv(GL_LINE_WIDTH_RANGE, lw);
        printf("line width range: %.2f to %.2f\n", lw[0], lw[1]);

        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ubo);
        glGenVertexArrays(1, &vao);

        regen();

        glBindBufferBase(GL_UNIFORM_BUFFER, ubo_bp, ubo);

        GLuint vs = util::buildShader(GL_VERTEX_SHADER, "vs", {"shaders/main.vert"});
        GLuint gs = util::buildShader(GL_GEOMETRY_SHADER, "gs", {"shaders/main.geom"});
        GLuint fs = util::buildShader(GL_FRAGMENT_SHADER, "fs", {"shaders/main.frag"});
        prog = util::buildProgram(false, "prog", {vs, gs, fs});

        unif_bright = (GLuint) glGetUniformLocation(prog, "bright");

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        auto xi = glGetAttribLocation(prog, "xi");
        if (xi >= 0) {
            glEnableVertexAttribArray((unsigned) xi);
            glVertexAttribPointer((unsigned) xi, 1, GL_FLOAT, GL_FALSE,
                sizeof(HopfCircle),
                (void *) offsetof(HopfCircle, xi));
        }

        auto eta = glGetAttribLocation(prog, "eta");
        if (eta >= 0) {
            glEnableVertexAttribArray((unsigned) eta);
            glVertexAttribPointer((unsigned) eta, 1, GL_FLOAT, GL_FALSE,
                sizeof(HopfCircle),
                (void *) offsetof(HopfCircle, eta));
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(0);
    }

    void update(GLFWwindow *window, float dt, int frame) {
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        auto ar = (float) w / h;

        t += dt;

        float c = std::cos(t / 8);
        float s = std::sin(t / 8);

        float c_ = std::cos(3.f / 8);
        float s_ = std::sin(3.f / 8);

        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        util::bufferData<ga::Mat>(GL_UNIFORM_BUFFER, {
            ga::Mat(
                ga::Vec(1.f / view / ar, 0, 0, 0),
                ga::Vec(0, 0, 1.f / 10, 0),
                ga::Vec(0, 1.f / view, 0, 0),
                ga::Vec(0, 0, 0, 1.f)),
            rot
        }, GL_STREAM_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void render(GLFWwindow *window, float dt, int frame) {
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDepthFunc(GL_LEQUAL);

        glUseProgram(prog);
        glBindVertexArray(vao);
        glEnable(GL_DEPTH_TEST);

        glLineWidth(10);
        glUniform1f(unif_bright, 0);
        glDrawArrays(GL_POINTS, 0, (unsigned) circles.size());

        glLineWidth(1);
        glUniform1f(unif_bright, 1);
        glDrawArrays(GL_POINTS, 0, (unsigned) circles.size());

        glBindVertexArray(0);
        glUseProgram(0);

        glFinish();
        glfwSwapBuffers(window);
    }

    void on_scroll(GLFWwindow *window, double xoffset, double yoffset) {
        printf("scroll %.2f %.2f\n", xoffset, yoffset);

        int xw = glfwGetKey(window, GLFW_KEY_Q);
        int yw = glfwGetKey(window, GLFW_KEY_W);
        int zw = glfwGetKey(window, GLFW_KEY_E);
        int xy = glfwGetKey(window, GLFW_KEY_A);
        int yz = glfwGetKey(window, GLFW_KEY_S);
        int zx = glfwGetKey(window, GLFW_KEY_D);

        int zoom = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL);

        float t = (float) yoffset * PI / 48;
        float c = std::cos(t);
        float s = std::sin(t);

        if (xw) {
            rot = ga::mul(rot, ga::Mat(
                ga::Vec(c, 0, 0, s),
                ga::Vec(0, 1, 0, 0),
                ga::Vec(0, 0, 1, 0),
                ga::Vec(-s, 0, 0, c)
            ));
        }

        if (yw) {
            rot = ga::mul(rot, ga::Mat(
                ga::Vec(1, 0, 0, 0),
                ga::Vec(0, c, 0, s),
                ga::Vec(0, 0, 1, 0),
                ga::Vec(0, -s, 0, c)
            ));
        }

        if (zw) {
            rot = ga::mul(rot, ga::Mat(
                ga::Vec(1, 0, 0, 0),
                ga::Vec(0, 1, 0, 0),
                ga::Vec(0, 0, c, s),
                ga::Vec(0, 0, -s, c)
            ));
        }

        if (xy) {
            rot = ga::mul(rot, ga::Mat(
                ga::Vec(c, s, 0, 0),
                ga::Vec(-s, c, 0, 0),
                ga::Vec(0, 0, 1, 0),
                ga::Vec(0, 0, 0, 1)
            ));
        }

        if (yz) {
            rot = ga::mul(rot, ga::Mat(
                ga::Vec(1, 0, 0, 0),
                ga::Vec(0, c, s, 0),
                ga::Vec(0, -s, c, 0),
                ga::Vec(0, 0, 0, 1)
            ));
        }

        if (zx) {
            rot = ga::mul(rot, ga::Mat(
                ga::Vec(c, 0, -s, 0),
                ga::Vec(0, 1, 0, 0),
                ga::Vec(s, 0, c, 0),
                ga::Vec(0, 0, 0, 1)
            ));
        }

        if (zoom) {
            view *= 1 - t / 5;
        }

        std::ofstream matfile;
        matfile.open("matrix.txt");
        matfile << "vec((\n";
        matfile << "vec((" << rot.x.x << ", " << rot.x.y << ", " << rot.x.z << ", " << rot.x.w << ")),\n";
        matfile << "vec((" << rot.y.x << ", " << rot.y.y << ", " << rot.y.z << ", " << rot.y.w << ")),\n";
        matfile << "vec((" << rot.z.x << ", " << rot.z.y << ", " << rot.z.z << ", " << rot.z.w << ")),\n";
        matfile << "vec((" << rot.w.x << ", " << rot.w.y << ", " << rot.w.z << ", " << rot.w.w << ")),\n";
        matfile << ")),";
        matfile.close();
    }

    void deinit(GLFWwindow *window) {
    }
};

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    auto *state = (State *) glfwGetWindowUserPointer(window);
    state->on_scroll(window, xoffset, yoffset);
}

void run() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    auto window = glfwCreateWindow(1280, 720, "Hopf Fibration", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(0);

    auto state = State(window);
    glfwSetWindowUserPointer(window, &state);

    glfwSetScrollCallback(window, scroll_callback);

    double time = glfwGetTime();
    int frame = 0;
    while (!glfwWindowShouldClose(window)) {
        double time_ = glfwGetTime();
        auto dt = (float) (time_ - time);

        state.update(window, dt, frame);
        state.render(window, dt, frame);

        glfwPollEvents();

        time = time_;
        frame += 1;
    }

    state.deinit(window);

    glfwDestroyWindow(window);
}

int main() {
    if (!glfwInit()) {
        return EXIT_FAILURE;
    }

    run();

    glfwTerminate();
    return EXIT_SUCCESS;
}