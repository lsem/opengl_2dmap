#include <errno.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "common/log.h"
#include "glad/glad.h"

#pragma once

namespace shader_program {

namespace {
// todo: use Expected.
bool load_file_content(std::filesystem::path fpath, std::string &out_data) {
    std::ifstream fs(fpath);
    if (!fs) {
        log_err("failed opening file: {}; error: {} ({})", fpath, strerror(errno), errno);
        return false;
    }
    std::stringstream data_s;
    data_s << fs.rdbuf();
    out_data = data_s.str();
    return true;
}
} // namespace

struct ShaderProgram {
    unsigned id = ~0u;

    ShaderProgram(unsigned id) : id(id) {}

    bool compile() { return false; }

    void attach() {
        assert(this->id != ~0u);
        glUseProgram(this->id);
    }
    void detach() { glUseProgram(0); }

    ShaderProgram(const ShaderProgram &) = delete;
    ShaderProgram &operator=(const ShaderProgram &) = delete;
};

// We expect that shaders put in folder <fpath> and have a name like
// <shader_idname>.vert.glsl, <shader_idname>.frag.glsl.
inline std::unique_ptr<ShaderProgram> make_from_fs_bundle(std::string fpath,
                                                          std::string shader_idname) {
    using path_t = std::filesystem::path;
    auto base_path = path_t{fpath} / shader_idname;

    std::string v_shader_src;
    if (!load_file_content(path_t{fpath} / (shader_idname + ".vert.glsl"), v_shader_src)) {
        log_err("failed loading file content of vertex shader for program: <{}>", shader_idname);
        return nullptr;
    }

    std::string f_shader_src;
    if (!load_file_content(path_t{fpath} / (shader_idname + ".frag.glsl"), f_shader_src)) {
        log_err("failed loading file content of fragment shader for program: <{}>", shader_idname);
        return nullptr;
    }

    int result = 0;
    unsigned v_shader_id, f_shader_id;
    {
        v_shader_id = glCreateShader(GL_VERTEX_SHADER);
        const char *v_shader_src_cstr = v_shader_src.c_str();
        glShaderSource(v_shader_id, 1, &v_shader_src_cstr, NULL);
        glCompileShader(v_shader_id);
        glGetShaderiv(v_shader_id, GL_COMPILE_STATUS, &result);
        if (!result) {
            char details[1024];
            glGetShaderInfoLog(v_shader_id, sizeof(details), NULL, details);
            log_err("failed compiling vertex shader for program <{}>, compilation "
                    "error:\n{}",
                    shader_idname, details);
            return nullptr;
        }
    }

    {
        f_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
        const char *f_shader_src_cstr = f_shader_src.c_str();
        glShaderSource(f_shader_id, 1, &f_shader_src_cstr, NULL);
        glCompileShader(f_shader_id);
        glGetShaderiv(f_shader_id, GL_COMPILE_STATUS, &result);
        if (!result) {
            char details[1024];
            glGetShaderInfoLog(f_shader_id, sizeof(details), NULL, details);
            log_err("failed compiling fragment shader for program <{}>, compilation "
                    "error: {}",
                    shader_idname, details);
            return nullptr;
        }
    }

    auto shader_program_id = glCreateProgram();
    glAttachShader(shader_program_id, v_shader_id);
    glAttachShader(shader_program_id, f_shader_id);
    glLinkProgram(shader_program_id);
    glGetProgramiv(shader_program_id, GL_LINK_STATUS, &result);
    if (!result) {
        char details[1024];
        glGetProgramInfoLog(shader_program_id, sizeof(details), NULL, details);
        log_err("failed linking shader program <{}>:\n {}", shader_idname, details);
        return nullptr;
    }

    glDeleteShader(v_shader_id);
    glDeleteShader(f_shader_id);

    return std::make_unique<ShaderProgram>(shader_program_id);
}

} // namespace shader_program