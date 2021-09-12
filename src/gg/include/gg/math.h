#pragma once

#include <cmath> // for sqrt

struct vec2d {
    vec2d(double x, double y) : x(x), y(y) {}
    double x;
    double y;
};
struct point2d {
    point2d(double x, double y) : x(x), y(y) {}
    double x;
    double y;
};

// vector operations

vec2d operator+(const vec2d &a, const vec2d &b) { return {a.x + b.x, a.y + b.y}; }
vec2d operator-(const vec2d &a, const vec2d &b) { return {a.x - b.x, a.y - b.y}; }
vec2d &operator+=(vec2d &a, const vec2d &b) {
    a.x += b.x;
    a.y += b.y;
    return a;
}
vec2d &operator-=(vec2d &a, const vec2d &b) {
    a.x -= b.x;
    a.y -= b.y;
    return a;
}
vec2d operator-(const vec2d &a) { return {-a.x, -a.y}; }
vec2d operator*(double k, const vec2d &a) { return {k * a.x, k * a.y}; }
double operator*(const vec2d &a, const vec2d &b) { return a.x * b.x + a.y * b.y; }
double len2(const vec2d &a) { return a * a; }
double abs(const vec2d &a) { return std::sqrt(len2(a)); }

// point operations

vec2d operator-(const point2d &a, const point2d &b) { return {a.x - b.x, a.y - b.y}; }
point2d operator+(const point2d &a, const vec2d &b) { return {a.x + b.x, a.y + b.y}; }
point2d operator-(const point2d &a, const vec2d &b) { return {a.x - b.x, a.y - b.y}; }
point2d &operator+=(point2d &a, const vec2d &b) {
    a.x += b.x;
    a.y += b.y;
    return a;
}
point2d &operator-=(point2d &a, const vec2d &b) {
    a.x -= b.x;
    a.y -= b.y;
    return a;
}
double dist2(const point2d &a, const point2d &b) { return len2(a - b); }
double dist(const point2d &a, const point2d &b) { return std::sqrt(dist2(a, b)); }
vec2d rotate_90(const vec2d &v) { return {v.y, -v.x}; }

vec2d normalize(const vec2d &v) {
    double l = abs(v);
    return {v.x / l, v.y / l};
}

vec2d normal(const point2d &a, const point2d &b) { return rotate_90(normalize(a - b)); }
