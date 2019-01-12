#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

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
    float xi;
    float eta;
};

struct State {
    GLuint vao{};
    GLuint vbo{}, ubo{};
    GLuint ubo_bp = 1;

    GLuint prog{};

    std::vector<HopfCircle> circles{};

    explicit State(GLFWwindow *window) {
        const int N = 32;
        for (int k = 0; k < N; ++k) {
            circles.push_back({2 * PI * k / N, 0.025});
            circles.push_back({2 * PI * k / N, PI / 2 - 0.025});

            circles.push_back({2 * PI * k / N, 0.3f});
            circles.push_back({2 * PI * k / N, 0.4f});
            circles.push_back({2 * PI * k / N, 0.5f});

            circles.push_back({2 * PI * k / N, 1.1f});
            circles.push_back({2 * PI * k / N, 1.2f});
            circles.push_back({2 * PI * k / N, 1.3f});
        }

        printf("vendor: %s\nrenderer: %s\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER));

        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ubo);
        glGenVertexArrays(1, &vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        util::bufferData<HopfCircle>(GL_ARRAY_BUFFER, circles, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBufferBase(GL_UNIFORM_BUFFER, ubo_bp, ubo);

        GLuint vs = util::buildShader(GL_VERTEX_SHADER, "vs", {"shaders/main.vert"});
        GLuint gs = util::buildShader(GL_GEOMETRY_SHADER, "gs", {"shaders/main.geom"});
        GLuint fs = util::buildShader(GL_FRAGMENT_SHADER, "fs", {"shaders/main.frag"});
        prog = util::buildProgram(false, "prog", {vs, gs, fs});

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

        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        util::bufferData<float>(GL_UNIFORM_BUFFER, {
            1.f / 4 / ar, 0, 0, 0,
            0, 0, 1.f / 10, 0,
            0, 1.f / 4, 0, 0,
            0, 0, 0, 1
        }, GL_STREAM_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void render(GLFWwindow *window, float dt, int frame) {
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(prog);
        glBindVertexArray(vao);
        glPointSize(5);
        glLineWidth(3);
        glDrawArrays(GL_POINTS, 0, (unsigned) circles.size());
        glBindVertexArray(0);
        glUseProgram(0);

        glFinish();
        glfwSwapBuffers(window);
    }

    void deinit(GLFWwindow *window) {
    }
};

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