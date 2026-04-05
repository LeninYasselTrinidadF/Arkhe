#include "ui/views/dep/dep_camera.hpp"
#include "ui/constants.hpp"
#include "raylib.h"
#include "raymath.h"

void dep_camera_update(Camera2D& dep_cam, bool in_canvas, bool canvas_blocked,
                       bool over_node, bool dragging) {
    if (in_canvas && !canvas_blocked) {
        bool pan = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)
            || (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !over_node && !dragging);
        if (pan) {
            Vector2 d = GetMouseDelta();
            dep_cam.target.x -= d.x / dep_cam.zoom;
            dep_cam.target.y -= d.y / dep_cam.zoom;
        }
    }
    if (float wh = GetMouseWheelMove(); wh != 0.f && in_canvas && !canvas_blocked) {
        dep_cam.offset = GetMousePosition();
        dep_cam.target = GetScreenToWorld2D(GetMousePosition(), dep_cam);
        dep_cam.zoom   = Clamp(dep_cam.zoom + wh * 0.12f, 0.05f, 6.f);
    }
    dep_cam.offset = { (float)CX(), (float)CCY() };
}
