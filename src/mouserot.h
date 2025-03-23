#pragma once

#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <string>

class MouseRot {
    std::string device_path;

    int pdev_fd;
    struct libevdev* pdev;

    int vdev_fd;
    struct libevdev_uinput* vdev;

    float rotation;
    float scale;

    float x_rem;
    float y_rem;
    float sin_val;
    float cos_val;

public:
    MouseRot(const std::string& device_path);
    ~MouseRot();
    void open_mouse();
    void grab_mouse();
    void create_virtual_mouse();
    void set_rotation_deg(float degrees);
    void set_rotation_rad(float radians);
    void set_scale(float scale);
    void loop();
    void emit(unsigned short ev_type, unsigned short ev_code, int value);
    void handle(const struct input_event& ev);
};
