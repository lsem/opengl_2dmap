#pragma once

#include "common/global.h"
#include "common/log.h"
#include "render_lib/camera_control.h"
#include "render_units/lines/lines_unit.h"

namespace {
v2 from_glmvec2(glm::vec2 v) { return v2{v.x, v.y}; }
} // namespace

// Helper to display cam control internal stuff.
struct CameraControlVisuals {
    LinesUnit lines; // todo: do nopt hold lines renderer here but only data.

    bool init_render(string shaders_root) {
        if (!this->lines.load_shaders(shaders_root)) {
            log_err("failed loading shaders for camera control visuals");
            return false;
        }
        return true;
    }

    // todo: idea: should we take something like rendeing_ctx just to give a
    // hint that this method uses opengl rendering context?
    void render(camera::Cam2d &cam, camera_control::CameraControl &cam_control) {
        if (cam_control.rotation) {
            this->lines.assign_lines(
                {{from_glmvec2(cam.unproject(cam_control.screen_center)),
                  from_glmvec2(cam.unproject(cam_control.rot_start_point)), colors::grey},
                 {from_glmvec2(cam.unproject(cam_control.screen_center)),
                  from_glmvec2(cam.unproject(cam_control.rot_curr_point)), colors::grey}});
            this->lines.render_frame(cam);
        }
    }
};
