#pragma once

#include <common/global.h>
#include <gg/gg.h>
#include <glm/vec2.hpp>

namespace animations {

using easing_func_t = double (*)(double);

namespace easing_funcs {

inline double linear(double t) { return t; }
inline double ease_out_quad(double t) { return t * (2.0 - t); }

auto default_ = ease_out_quad;

} // namespace easing_funcs

template <typename Value> struct Animation {
    Animation(Value *animated_value, Value target_value, steady_clock::time_point start_time,
              steady_clock::duration duration, easing_func_t easing_func,
              std::function<void()> finish_cb)
        : animated_value(animated_value), source_value(*animated_value), target_value(target_value),
          start_time(start_time), duration(duration), easing_func(easing_func),
          finish_cb(finish_cb), completed(false) {}

    Value *animated_value;
    Value source_value;
    Value target_value;
    steady_clock::time_point start_time;
    steady_clock::duration duration;
    easing_func_t easing_func;
    std::function<void()> finish_cb;
    bool completed;
};

namespace details {
inline double duration_as_double(steady_clock::duration d) {
    return std::chrono::duration_cast<std::chrono::microseconds>(d).count() * 1'000'000.0;
}

// For some reason, glm vec scalar multiplication produces 1-d vector..
inline glm::vec2 operator*(const glm::vec2 &v, double d) { return glm::vec2(v.x * d, v.y * d); }

template <typename Value>
inline void process_tick(vector<Animation<Value>> &animations,
                         steady_clock::time_point current_time) {
    for (auto &anim : animations) {
        assert(!anim.completed);
        const double t = std::clamp(details::duration_as_double(current_time - anim.start_time) /
                                        details::duration_as_double(anim.duration),
                                    0.0, 1.0);
        if (std::abs(t - 1.0) < 1e-3) {
            anim.completed = true;
            *anim.animated_value = anim.target_value;
            anim.finish_cb();
        } else {
            *anim.animated_value = (anim.target_value - anim.source_value) * anim.easing_func(t);
        }
    }
    animations.erase(std::remove_if(std::begin(animations), std::end(animations),
                                    [](auto &anim) { return anim.completed; }),
                     std::end(animations));
}
} // namespace details

void do_nothing() {}

struct AnimationsEngine {
    vector<Animation<v2>> v2_animations;
    vector<Animation<double>> double_animations;
    vector<Animation<glm::vec2>> glmvec2_animations;

    template <typename Value>
    void animate(Value *value_ref, Value target_value, steady_clock::duration duration,
                 std::function<void()> finish_cb = do_nothing) {
        this->add(animations::Animation<Value>(value_ref, target_value, steady_clock::now(),
                                               duration, easing_funcs::default_,
                                               std::move(finish_cb)));
    }

    template <typename Value>
    void animate(Value *value_ref, Value target_value, steady_clock::duration duration,
                 easing_func_t easing_func, std::function<void()> finish_cb = do_nothing) {
        this->add(animations::Animation<Value>(value_ref, target_value, steady_clock::now(),
                                               duration, easing_func, do_nothing));
    }

    template <typename Value> void add(Animation<Value> animation) {
        if constexpr (std::is_same_v<Value, v2>) {
            this->v2_animations.emplace_back(std::move(animation));
        } else if constexpr (std::is_same_v<Value, double>) {
            this->double_animations.emplace_back(std::move(animation));
        } else if constexpr (std::is_same_v<Value, glm::vec2>) {
            this->glmvec2_animations.emplace_back(std::move(animation));
        } else {
            static_assert("Animation of given type is not supported");
        }
    }

    void tick(steady_clock::time_point current_time = steady_clock::now()) {
        details::process_tick(this->v2_animations, current_time);
        details::process_tick(this->double_animations, current_time);
        details::process_tick(this->glmvec2_animations, current_time);
    }
};
} // namespace animations