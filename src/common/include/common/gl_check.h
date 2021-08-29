#pragma once

#include "log.h"
#include <glad/glad.h>

inline void check_opengl_err(const char *stmt, const char *fname, int line) {
    if (auto err = glGetError(); err != GL_NO_ERROR) {
        log_err("OpenGL error: 0x{:X}, at {}:{} - for {}", err, fname, line, stmt);
        abort();
    }
}

#define GL_CHECK(Statement)                                                                        \
    do {                                                                                           \
        Statement;                                                                                 \
        check_opengl_err(#Statement, __FILE__, __LINE__);                                          \
    } while (false)
