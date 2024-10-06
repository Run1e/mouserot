#include <string>

class MouseRot {
    std::string device_path;

    struct libevdev* dev;
    int vdev_fd;
    int pdev_fd;

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