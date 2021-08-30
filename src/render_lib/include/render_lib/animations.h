#pragma once

#include <common/global.h>
#include <gg/gg.h>
#include <glm/vec2.hpp>

namespace animations {

using easing_func_t = double (*)(double);

namespace easing_funcs {

double linear(double t) { return t; }
double ease_out_quad(double t) { return t * (2.0 - t); }

auto default_ = ease_out_quad;

} // namespace easing_funcs

template <typename Value> struct Animation {
    Animation(Value *animated_value, Value target_value, steady_clock::duration duration,
              easing_func_t easing_func, std::function<void()> finish_cb)
        : animated_value(animated_value), source_value(*animated_value), target_value(target_value),
          start_time(steady_clock::now()), duration(duration), easing_func(easing_func),
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

// This ugly thing in general not needed. Added only until glm:: stuff still
// here and needs to be animated.
template <typename Value> struct Multiply {
    static Value apply(const Value &value, double t) { return value * t; }
};
template <> struct Multiply<glm::vec2> {
    static glm::vec2 apply(const glm::vec2 &value, double t) {
        return glm::vec2(value.x * t, value.y * t);
    }
};

template <typename Value>
inline void process_tick(vector<unique_ptr<Animation<Value>>> &animations,
                         steady_clock::time_point current_time) {
    for (auto &anim : animations) {
        assert(!anim->completed);
        const double t = std::clamp(details::duration_as_double(current_time - anim->start_time) /
                                        details::duration_as_double(anim->duration),
                                    0.0, 1.0);
        const double et = anim->easing_func(t);
        if (std::abs(et - 1.0) < 1e-3) {
            anim->completed = true;
            *anim->animated_value = anim->target_value;
            anim->finish_cb();
        } else {
            *anim->animated_value =
                details::Multiply<Value>::apply(anim->target_value - anim->source_value, et);
        }
    }
    animations.erase(std::remove_if(std::begin(animations), std::end(animations),
                                    [](auto &anim) { return anim->completed; }),
                     std::end(animations));
}
} // namespace details

void do_nothing() {}

struct AnimationsEngine {
    vector<unique_ptr<Animation<v2>>> v2_animations;
    vector<unique_ptr<Animation<double>>> double_animations;
    vector<unique_ptr<Animation<glm::vec2>>> glmvec2_animations;

    template <typename Value>
    void animate(Value *value_ref, Value target_value, steady_clock::duration duration,
                 std::function<void()> finish_cb = do_nothing) {
        this->add(std::make_unique<animations::Animation<Value>>(
            value_ref, target_value, duration, easing_funcs::default_, std::move(finish_cb)));
    }

    template <typename Value>
    void animate(Value *value_ref, Value target_value, steady_clock::duration duration,
                 easing_func_t easing_func, std::function<void()> finish_cb = do_nothing) {
        this->add(std::make_unique<animations::Animation<Value>>(value_ref, target_value, duration,
                                                                 easing_func, do_nothing));
    }

    template <typename Value> void add(std::unique_ptr<Animation<Value>> animation) {
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

    void process_frame(steady_clock::time_point current_time = steady_clock::now()) {
        details::process_tick(this->v2_animations, current_time);
        details::process_tick(this->double_animations, current_time);
        details::process_tick(this->glmvec2_animations, current_time);
    }
};
} // namespace animations