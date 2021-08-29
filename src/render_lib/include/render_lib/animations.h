#pragma once

#include <common/global.h>
#include <gg/gg.h>
#include <glm/vec2.hpp>

namespace animations {

template <typename Value> struct Animation {
    Animation(Value *animated_value, Value target_value, steady_clock::duration duration,
              std::function<void()> finish_cb)
        : animated_value(animated_value), source_value(*animated_value), target_value(target_value),
          start_time(steady_clock::now()), duration(duration), finish_cb(finish_cb), enabled(true) {
    }
    Value *animated_value;
    Value source_value;
    Value target_value;
    steady_clock::time_point start_time;
    steady_clock::duration duration;
    std::function<void()> finish_cb;
    bool enabled;
};

namespace details {
inline double duration_as_double(steady_clock::duration d) {
    return std::chrono::duration_cast<std::chrono::microseconds>(d).count() * 1'000'000.0;
}

// This ugly thing in general not needed. Added only until glm:: stuff still
// here and needs to be animated.
template <typename Value> struct Multiply {
    static Value apply(const Value &value, double t) {
        assert(t >= 0.0 && t <= 1.0);
        return value * t;
    }
};
template <> struct Multiply<glm::vec2> {
    static glm::vec2 apply(const glm::vec2 &value, double t) {
        assert(t >= 0.0 && t <= 1.0);
        return glm::vec2(value.x * t, value.y * t);
    }
};

} // namespace details

template <typename Value>
void process_tick(vector<unique_ptr<Animation<Value>>> &animations,
                  steady_clock::time_point current_time) {
    for (auto &anim : animations) {
        if (!anim->enabled) {
            continue;
        }

        const double t = std::clamp(details::duration_as_double(current_time - anim->start_time) /
                                        details::duration_as_double(anim->duration),
                                    0.0, 1.0);
        if (t == 1.0) {
            anim->enabled = false;
            *anim->animated_value = anim->target_value;
            anim->finish_cb();
        } else {

            *anim->animated_value =
                anim->source_value +
                details::Multiply<Value>::apply((anim->target_value - anim->source_value), t);
        }
    }
}

void do_nothing() {}

struct AnimationsEngine {
    vector<unique_ptr<Animation<v2>>> v2_animations;
    vector<unique_ptr<Animation<double>>> double_animations;
    vector<unique_ptr<Animation<glm::vec2>>> glmvec2_animations;

    template <typename Value>
    void animate(Value *value_ref, Value target_value, steady_clock::duration duration,
                 std::function<void()> finish_cb = do_nothing) {
        this->add(std::make_unique<animations::Animation<Value>>(value_ref, target_value, duration,
                                                                 std::move(finish_cb)));
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
        process_tick(this->v2_animations, current_time);
        process_tick(this->double_animations, current_time);
        process_tick(this->glmvec2_animations, current_time);
    }
};
} // namespace animations