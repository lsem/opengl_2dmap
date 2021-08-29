#pragma once

#include "camera.h"

class IRenderUnit {
  public:
    virtual void render_frame(const camera::Cam2d &cam) = 0;
};
