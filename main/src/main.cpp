//
// Created by allem on 2/7/2019.
//

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <string>

struct State {
    void deinit(GLFWwindow *window) {

    }

    void init(GLFWwindow *window) {

    }

    void render(GLFWwindow *window, float dt, int frame) {

    }

    void update(GLFWwindow *window, float dt, int frame) {

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
