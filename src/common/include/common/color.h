
#pragma once

struct Color {
  Color(float r, float g, float b) : r(r), g(g), b(b) {}
  float r, g, b;
};

namespace colors {
static Color black{0.0f, 0.0f, 0.0f};
static Color green{0.0f, 1.0f, 0.0f};
static Color red{1.0f, 0.0f, 0.0f};
static Color blue{0.0f, 0.0f, 1.0f};
static Color grey{0.7f, 0.7f, 0.7f};
static Color magenta{1.0f, 0.0f, 1.0f};
} // namespace colors
