//
// Created by allem on 2/7/2019.
//

#ifndef HOPF_FIBRATION_UTIL_H
#define HOPF_FIBRATION_UTIL_H

#include <glad/glad.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

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

#endif //HOPF_FIBRATION_UTIL_H
