#pragma once
#include <common/color.h>
#include <common/global.h>

// Contains types, containers, buffer objects, shader program and renderer for
// displaying debug lines.
namespace debug_lines {

struct Line {
    p32 a;
    p32 b;
    Color color;
    std::string tag;
    Line(p32 a, p32 b, Color color, std::string tag)
        : a(a), b(b), color(color), tag(std::move(tag)) {}
};

// I want to have object that would allow me to manage memory for debug lines.
// So that I can allocate, deallocate memory, upload-data, etc.. I would also
// want to change memory for lines (we need to change colors for lines).

// routine for producing GPU data (either filled trignales or built-in opengl lines)
// In general this geometry builders are naturally coupled with shader program.
// I would say that there is render module, with contains geometry builders,
// uniforms arrays builders, things that craete proper layout. but buffers itself
// is separate thing. We can have a lot of these memory parts in GPU, caching,
// pools, etc.. In general we can take any buffer and configure vao for buffer
// that shaders need.
// tuple<size_t, size_t> produce_geometry(vector<Line> lines)

} // namespace debug_lines