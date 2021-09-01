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
              std::function<void()> finish_cb, unsigned int id)
        : animated_value(animated_value), source_value(*animated_value), target_value(target_value),
          start_time(start_time), duration(duration), easing_func(easing_func),
          finish_cb(finish_cb), completed(false), cancelled(false), abandoned(true), id(id) {}

    // Stops animation, leaves value in current/intermediate state, completion callback is not
    // called.
    void cancel() { cancelled = true; }
    void abandon() { abandoned = true; }

    Value *animated_value;
    Value source_value;
    Value target_value;
    steady_clock::time_point start_time;
    steady_clock::duration duration;
    easing_func_t easing_func;
    std::function<void()> finish_cb;
    bool completed;
    bool cancelled;
    bool abandoned;
    unsigned int id;
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
        if (anim.cancelled) {
            continue;
        }
        const double t = std::clamp(details::duration_as_double(current_time - anim.start_time) /
                                        details::duration_as_double(anim.duration),
                                    0.0, 1.0);
        if (std::abs(t - 1.0) < 1e-3) {
            anim.completed = true;
            *anim.animated_value = anim.target_value;
            anim.finish_cb();
        } else {
            *anim.animated_value =
                anim.source_value + (anim.target_value - anim.source_value) * anim.easing_func(t);
        }
    }
    animations.erase(std::remove_if(std::begin(animations), std::end(animations),
                                    [](auto &anim) {
                                        return (anim.completed || anim.cancelled);
                                    }),
                     std::end(animations));
}
} // namespace details

void do_nothing() {}

class AnimationsEngine;

// Animation handle is type-specific, if you need group cancelling, you will likely need type erased
// version of this, possibly with batch operations, feel free to implement.
template <class Value> struct AnimationHandle {
    AnimationsEngine *engine_ptr;
    unsigned int animation_id;

    AnimationHandle(AnimationsEngine *engine_ptr, unsigned int id)
        : engine_ptr(engine_ptr), animation_id(id) {}
    ~AnimationHandle();

    void cancel();
};

struct AnimationsEngine {
    unsigned int next_animation_id = 0;
    vector<Animation<v2>> v2_animations;
    vector<Animation<double>> double_animations;
    vector<Animation<glm::vec2>> glmvec2_animations;

    // Selects appropriate animations vector for value type.
    template <typename Value> vector<Animation<Value>> &animations_vec() {
        if constexpr (std::is_same_v<Value, v2>) {
            return v2_animations;
        } else if constexpr (std::is_same_v<Value, double>) {
            return double_animations;
        } else if constexpr (std::is_same_v<Value, glm::vec2>) {
            return glmvec2_animations;
        } else {
            static_assert("Animation of given type is not supported");
        }
    }

    template <typename Value>
    std::shared_ptr<AnimationHandle<Value>> animate(Value *value_ref, Value target_value,
                                                    steady_clock::duration duration,
                                                    std::function<void()> finish_cb = do_nothing) {
        animations_vec<Value>().emplace_back(animations::Animation<Value>(
            value_ref, target_value, steady_clock::now(), duration, easing_funcs::default_,
            std::move(finish_cb), next_animation_id++));
        auto &added_anim = animations_vec<Value>().back();
        added_anim.abandoned = false;
        return std::make_shared<AnimationHandle<Value>>(this, added_anim.id);
    }

    template <typename Value>
    std::shared_ptr<AnimationHandle<Value>>
    animate(Value *value_ref, Value target_value, steady_clock::duration duration,
            easing_func_t easing_func, std::function<void()> finish_cb = do_nothing) {
        animations_vec<Value>().emplace_back(
            animations::Animation<Value>(value_ref, target_value, steady_clock::now(), duration,
                                         easing_func, do_nothing, next_animation_id++));
        auto &added_anim = animations_vec<Value>().back();
        added_anim.abandoned = false;
        return std::make_shared<AnimationHandle<Value>>(this, added_anim.id);
    }

    void tick(steady_clock::time_point current_time = steady_clock::now()) {
        details::process_tick(this->v2_animations, current_time);
        details::process_tick(this->double_animations, current_time);
        details::process_tick(this->glmvec2_animations, current_time);
    }

    template <typename Value> void abandon(unsigned animation_id) {
        auto &animations_vec = this->animations_vec<Value>();
        auto it = std::find_if(animations_vec.begin(), animations_vec.end(),
                               [&](auto &anim) { return anim.id == animation_id; });
        assert(it != animations_vec.end()); // where did you get this handle?
        it->abandon();
    }

    template <typename Value> void cancel(unsigned animation_id) {
        auto &animations_vec = this->animations_vec<Value>();
        auto it = std::find_if(animations_vec.begin(), animations_vec.end(),
                               [&](auto &anim) { return anim.id == animation_id; });
        assert(it != animations_vec.end()); // where did you get this handle?
        it->cancel();
    }
};

template <class Value> AnimationHandle<Value>::~AnimationHandle() {
    //log_debug("~AnimationHandle, abandoning {}", this->animation_id);
    this->engine_ptr->template abandon<Value>(this->animation_id);
}

template <class Value> void AnimationHandle<Value>::cancel() {
    //log_debug("~AnimationHandle, cancelling {}", this->animation_id);
    this->engine_ptr->template cancel<Value>(this->animation_id);
}

} // namespace animations