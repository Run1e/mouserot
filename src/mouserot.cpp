#include "mouserot.h"

#include "spdlog/spdlog.h"
#include "utils.h"

#include <cmath>
#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <iomanip>
#include <linux/uinput.h>
#include <math.h>
#include <string>
#include <unistd.h>

MouseRot::MouseRot(const std::string& device_path) : device_path(device_path)
{
    this->pdev = nullptr;
    this->vdev = nullptr;

    this->vdev_fd = -1;
    this->pdev_fd = -1;

    this->scale = 1.0;

    // initialize misc
    this->sin_val = 0.0;
    this->cos_val = 1.0;
    this->x_rem = 0.0;
    this->y_rem = 0.0;
};

MouseRot::~MouseRot()
{
    if (this->pdev != nullptr) {
        libevdev_free(this->pdev);
        close(this->pdev_fd);
    }

    if (this->vdev_fd != -1) {
        libevdev_uinput_destroy(this->vdev);
        close(this->vdev_fd);
    }
};

void MouseRot::open_mouse()
{
    // this->pdev_fd = open(this->device_path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    this->pdev_fd = open(this->device_path.c_str(), O_RDONLY | O_CLOEXEC);
    if (this->pdev_fd == -1)
        throw std::system_error(errno, std::generic_category(), "Failed opening device");

    int rc = libevdev_new_from_fd(this->pdev_fd, &this->pdev);
    if (rc < 0)
        throw std::system_error(errno, std::generic_category(), "Failed to init libevdev");

    spdlog::info(
        "Opened device:\n\tName: {}\n\tBus: {:x}\n\tVendor: {:x}\n\tProduct: {:x}",
        libevdev_get_name(this->pdev),
        libevdev_get_id_bustype(this->pdev),
        libevdev_get_id_vendor(this->pdev),
        libevdev_get_id_product(this->pdev));

    if (!looks_like_mouse(this->pdev))
        throw std::runtime_error("Device does not look like a mouse!");
};

void MouseRot::grab_mouse()
{
    if (this->pdev_fd == -1)
        throw std::runtime_error("No open mouse device");

    if (!this->pdev)
        throw std::runtime_error("Can't grab mouse device we haven't initialized");

    int res = libevdev_grab(this->pdev, LIBEVDEV_GRAB);
    if (res < 0)
        throw std::system_error(
            res,
            std::generic_category(),
            "Failed to grab device. Maybe mouserot is already running?");
};

void MouseRot::create_virtual_mouse()
{
    if (this->pdev_fd == -1)
        throw std::runtime_error("Can't create virtual mouse before opening physical mouse");

    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0)
        throw std::system_error(errno, std::generic_category(), "Failed to open /dev/uinput");

    this->vdev_fd = fd;

    int rc = libevdev_uinput_create_from_device(this->pdev, fd, &this->vdev);
    if (rc < 0)
        throw std::system_error(errno, std::generic_category(), "Failed to create uinput device");
};

void MouseRot::set_rotation_deg(float degrees)
{
    float mult = static_cast<float>(M_PI / 180.0);
    this->set_rotation_rad(degrees * mult);
};

void MouseRot::set_rotation_rad(float radians)
{
    this->rotation = radians;
    this->sin_val = std::sin(radians);
    this->cos_val = std::cos(radians);
};

void MouseRot::set_scale(float scale)
{
    this->scale = scale;
};

void MouseRot::loop()
{
    int rc = 0;
    struct input_event ev;
    struct libevdev* pdev = this->pdev;

    while (true) {
        // returns -EAGAIN if nothing is there
        rc = libevdev_next_event(pdev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        SPDLOG_TRACE("rc: {}", rc);

        if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            SPDLOG_TRACE(
                "Event: {} {} {} rc: {}",
                libevdev_event_type_get_name(ev.type),
                libevdev_event_code_get_name(ev.type, ev.code),
                ev.value,
                rc);

            this->handle(ev);

        } else if (rc == LIBEVDEV_READ_STATUS_SYNC) {
            SPDLOG_DEBUG("LIBEVDEV_READ_STATUS_SYNC");
            continue;
        } else if (rc == -EAGAIN) {
            continue;
        } else {
            throw std::system_error(errno, std::generic_category(), "Failed reading from device");
        }
    };
};

void MouseRot::handle(const struct input_event& ev)
{
    float dx, dy;
    float ix, iy;
    float value;

    int move_x;
    int move_y;

    // https://www.desmos.com/calculator/rjfhpd184z
    // apply rotation
    if (ev.type == EV_REL) {
        if (ev.code == REL_X) {
            value = static_cast<float>(ev.value);
            dx = value * this->cos_val;
            dy = value * this->sin_val;

        } else if (ev.code == REL_Y) {
            value = static_cast<float>(ev.value);
            dx = -value * this->sin_val;
            dy = value * this->cos_val;

        } else {
            this->emit(ev.type, ev.code, ev.value);
            return;
        }

        // scale and find remaining fractional part
        this->x_rem = std::modf(dx * this->scale + this->x_rem, &ix);
        this->y_rem = std::modf(dy * this->scale + this->y_rem, &iy);

        // cast integral parts to int
        move_x = static_cast<int>(ix);
        move_y = static_cast<int>(iy);

        // and move if applicable
        if (move_x != 0)
            this->emit(EV_REL, REL_X, move_x);

        if (move_y != 0)
            this->emit(EV_REL, REL_Y, move_y);
    } else {
        this->emit(ev.type, ev.code, ev.value);
    }
};

void MouseRot::emit(unsigned short ev_type, unsigned short ev_code, int value)
{
    if (this->vdev_fd == -1)
        return;

    struct input_event event{};

    event.input_event_usec = 0;
    event.input_event_sec = 0;
    event.type = ev_type;
    event.code = ev_code;
    event.value = value;

    write(this->vdev_fd, &event, sizeof(event));
};
