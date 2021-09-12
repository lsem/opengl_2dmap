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

    const void *key() const { return animated_value; }

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
        if (t == 1.0) {
            anim.completed = true;
            *anim.animated_value = anim.target_value;
            anim.finish_cb();
        } else {
            *anim.animated_value =
                anim.source_value + (anim.target_value - anim.source_value) * anim.easing_func(t);
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
    vector<Animation<float>> float_animations;
    vector<Animation<glm::vec2>> glmvec2_animations;

    template <typename Value, typename FinishCb = decltype(&do_nothing)>
    void animate(Value *value_ref, Value target_value, Value speed_per_second,
                 FinishCb &&finish_cb = do_nothing) {
        animate(value_ref, target_value, speed_per_second, easing_funcs::default_,
                std::move(finish_cb));
    }

    template <typename Value, typename FinishCb = decltype(&do_nothing)>
    void animate(Value *value_ref, Value target_value, Value speed_per_second,
                 easing_func_t easing_func, FinishCb &&finish_cb = do_nothing) {
        steady_clock::duration duration = std::chrono::milliseconds(
            static_cast<unsigned>(std::abs((target_value - *value_ref) / speed_per_second) * 1000));
        animate(value_ref, target_value, duration, easing_func, std::move(finish_cb));
    }

    template <typename Value, typename FinishCb = decltype(&do_nothing)>
    void animate(Value *value_ref, Value target_value, steady_clock::duration duration,
                 FinishCb &&finish_cb = do_nothing) {
        animate(value_ref, target_value, duration, easing_funcs::default_, std::move(finish_cb));
    }

    template <typename Value, typename FinishCb = decltype(&do_nothing)>
    void animate(Value *value_ref, Value target_value, steady_clock::duration duration,
                 easing_func_t easing_func, FinishCb&& finish_cb = do_nothing) {
        if (*value_ref == target_value) {
            // animation already done
            finish_cb();
            return;
        }
        if (duration.count() == 0) {
            // do it now
            *value_ref = target_value;
            finish_cb();
            return;
        }
        this->add(animations::Animation<Value>(value_ref, target_value, steady_clock::now(),
                                               duration, easing_func, std::move(finish_cb)));
    }

    template <typename Value> void add(Animation<Value> animation) {
        auto &v = get_vector<Value>();
        for (auto &a : v) {
            if (a.key() == animation.key()) {
                a = animation;
                return;
            }
        }
        v.emplace_back(std::move(animation));
    }

    template <typename Value>
    vector<Animation<Value>>& get_vector() {
        if constexpr (std::is_same_v<Value, v2>) {
            return v2_animations;
        } else if constexpr (std::is_same_v<Value, double>) {
            return double_animations;
        } else if constexpr (std::is_same_v<Value, float>) {
            return float_animations;
        } else if constexpr (std::is_same_v<Value, glm::vec2>) {
            return glmvec2_animations;
        } else {
            static_assert(!std::is_same_v<Value, Value>,
                          "Animation of given type is not supported");
        }
    }

    void tick(steady_clock::time_point current_time = steady_clock::now()) {
        details::process_tick(this->v2_animations, current_time);
        details::process_tick(this->double_animations, current_time);
        details::process_tick(this->float_animations, current_time);
        details::process_tick(this->glmvec2_animations, current_time);
    }
};
} // namespace animations