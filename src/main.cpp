#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <iostream>
#include <fstream>
#include <cmath>

#include "gl/debug.hpp"
#include "gl/buffer.hpp"
#include "gl/shader.hpp"
#include "gl/vertexarray.hpp"

#include <ml/meshlib.hpp>
#include <ml/meshlib_json.hpp>

struct State {
    using Transform4 = Eigen::Transform<float, 4, Eigen::Projective>;
    using Transform3 = Eigen::Transform<float, 3, Eigen::Projective>;

    Eigen::Vector4f bg{0.07f, 0.09f, 0.10f, 1.00f};
    Eigen::Vector4f fg{0.71f, 0.53f, 0.94f, 1.00f};
    Eigen::Vector4f wf{0.95f, 0.95f, 0.95f, 1.00f};

    Eigen::Vector4f R{1.00f, 0.00f, 0.00f, 1.00f};
    Eigen::Vector4f G{0.00f, 1.00f, 0.00f, 1.00f};
    Eigen::Vector4f B{0.00f, 0.00f, 1.00f, 1.00f};
    Eigen::Vector4f Y{1.20f, 1.20f, 0.00f, 1.00f};

    Transform4 rot = Transform4::Identity();
    Transform3 view = Transform3::Identity();

    bool color_axes = false;
};

Eigen::Matrix4f rotor(int u, int v, float rad) {
    Eigen::Matrix4f res = Eigen::Matrix4f::Identity();
    res(u, u) = res(v, v) = cosf(rad);
    res(u, v) = res(v, u) = sinf(rad);
    res(u, v) *= -1;
    return res;
}

template<typename T_>
T_ mix(const T_ &a, const T_ &b, const typename T_::Scalar &x) {
    return a * (1 - x) + b * x;
}

void show_overlay(State &state) {
    static std::string gl_vendor = (const char *) glGetString(GL_VENDOR);
    static std::string gl_renderer = (const char *) glGetString(GL_RENDERER);
    static std::string gl_version = (const char *) glGetString(GL_VERSION);
    static std::string glsl_version = (const char *) glGetString(GL_SHADING_LANGUAGE_VERSION);

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoMove;

    ImGuiStyle &style = ImGui::GetStyle();
    const auto PAD = style.DisplaySafeAreaPadding;
    auto window_pos = PAD;

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f * style.Alpha);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Appearing);
    ImGui::Begin("Graphics Information", nullptr, window_flags);
    ImGui::Text("GL Vendor    | %s", gl_vendor.c_str());
    ImGui::Text("GL Renderer  | %s", gl_renderer.c_str());
    ImGui::Text("GL Version   | %s", gl_version.c_str());
    ImGui::Text("GLSL Version | %s", glsl_version.c_str());

    auto v2 = ImGui::GetWindowSize();
    window_pos.y += v2.y + PAD.y;
    ImGui::End();

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f * style.Alpha);
    ImGui::Begin("Controls", nullptr, window_flags);

    ImGuiIO &io = ImGui::GetIO();
    ImGui::Text("FPS          | %.2f", io.Framerate);

    ImGui::Separator();

    ImGui::ColorEdit3("Background", state.bg.data(), ImGuiColorEditFlags_Float);
    ImGui::ColorEdit3("Foreground", state.fg.data(), ImGuiColorEditFlags_Float);
    ImGui::ColorEdit3("Wireframe", state.wf.data(), ImGuiColorEditFlags_Float);

    ImGui::Checkbox("Show RGBY axis colors", &state.color_axes);

    if (io.MouseDown[0] && !io.WantCaptureMouse) {
        Eigen::Matrix4f rot = Eigen::Matrix4f::Identity();
        Eigen::Vector2f del{io.MouseDelta.x, io.MouseDelta.y};
        del /= 200.0f;

        if (io.KeyShift) {
            del /= 5.0f;
        }

        if (io.KeyCtrl) {
            Eigen::Matrix4f rx = rotor(0, 3, -del.x());
            Eigen::Matrix4f ry = rotor(1, 3, del.y());
            rot = rx * ry;
        } else {
            Eigen::Matrix4f rx = rotor(0, 2, -del.x());
            Eigen::Matrix4f ry = rotor(1, 2, del.y());
            rot = rx * ry;
        }

        state.rot.linear() = rot * state.rot.linear();
    }

    if ((io.MouseWheel != 0) && !io.WantCaptureMouse) {
        float scale = 1.0f + io.MouseWheel * 0.05f;
        state.view.linear() *= scale;
    }

    ImGui::End();
}

void set_style() {
    ImGui::StyleColorsDark();

    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 4;
    style.FrameRounding = 2;
    style.DisplaySafeAreaPadding.x = 10;
    style.DisplaySafeAreaPadding.y = 10;
}


Eigen::Vector4f hopf_map(float e1, float e2, float n) {
    Eigen::Vector4f res;
    res <<
        cosf((e2 + e1) / 2) * sinf(n),
        sinf((e2 + e1) / 2) * sinf(n),
        cosf((e2 - e1) / 2) * cosf(n),
        sinf((e2 - e1) / 2) * cosf(n);
    return res;
}

auto make_hopf(size_t latitudes, size_t longitudes, size_t link_res) {
    Eigen::Matrix4Xf points(4, latitudes * longitudes * link_res);
    ml::Matrix1Xui lines(1, latitudes * longitudes * link_res);

    Eigen::Index idx = 0;

    for (int i = 0; i < latitudes; ++i) {
        float n = (float) i / (float) (latitudes - 1) * M_PIf32 / 2.0f;

        for (int j = 0; j < longitudes; ++j) {
            float e1 = (float) j / (float) longitudes * M_PIf32 * 2.0f;

            for (int k = 0; k < link_res; ++k) {
                float e2 = (float) k / (float) link_res * M_PIf32 * 4.0f;

                lines.col(idx) << idx;
                points.col(idx) = hopf_map(e1, e2, n).normalized();
                idx++;
            }
        }
    }

    return ml::Mesh(points, lines);
}

int run(GLFWwindow *window, ImGuiContext *context) {
    State state;

    Buffer<GLuint> ind_buf;
    Buffer<Eigen::Vector4f> vert_buf;

    VertexArray<Eigen::Vector4f> vao(vert_buf);
    glVertexArrayElementBuffer(vao, ind_buf);

    using PointsType = Eigen::Matrix<float, 4, Eigen::Dynamic>;
    using CellsType = Eigen::Matrix<unsigned, 3, Eigen::Dynamic>;
    using Mesh = ml::Mesh<PointsType, CellsType>;

    auto mesh = make_hopf(5, 32, 1024);

    auto elements = (GLint) ind_buf.upload(mesh.cells.reshaped());
    vert_buf.upload(mesh.points.colwise());

    VertexShader vs4d(std::ifstream("res/shaders/4d.vert.glsl"));
    FragmentShader fs(std::ifstream("res/shaders/main.frag.glsl"));

    Program pgm(vs4d, fs);

    glEnable(GL_DEPTH_TEST);

    Eigen::Projective3f proj;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        show_overlay(state);
        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(state.bg[0], state.bg[1], state.bg[2], state.bg[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto aspect = (float) display_h / (float) display_w;
        proj = Eigen::AlignedScaling3f(aspect, 1.0, -0.6);

        Eigen::Matrix4f rot = state.rot.linear();
        Eigen::Matrix4f view = state.view.matrix();

        glUseProgram(pgm);
        glBindVertexArray(vao);
        glUniform4fv(0, 1, state.fg.data());
        glUniform1f(1, (GLfloat) glfwGetTime());
        glUniformMatrix4fv(2, 1, false, proj.data());
        glUniformMatrix4fv(3, 1, false, rot.data());
        glUniformMatrix4fv(4, 1, false, view.data());
        glDrawElements(GL_POINTS, elements, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
        glUseProgram(0);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    return EXIT_SUCCESS;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW:Failed initialization" << std::endl;
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    auto *window = glfwCreateWindow(1280, 720, "Cosets Visualization", nullptr, nullptr);
    if (!window) {
        std::cerr << "GLFW:Failed to create window" << std::endl;
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(log_gl_debug_callback, nullptr);
    glDebugMessageControl(
        GL_DONT_CARE, GL_DEBUG_TYPE_OTHER,
        GL_DEBUG_SEVERITY_NOTIFICATION,
        0, nullptr, GL_FALSE
    );
#endif

    IMGUI_CHECKVERSION();
    auto *context = ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    set_style();

    int exit_code = EXIT_SUCCESS;

    try {
        exit_code = run(window, context);
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        exit_code = EXIT_FAILURE;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return exit_code;
}
