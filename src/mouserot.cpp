#include "mouserot.h"

#include "spdlog/spdlog.h"
#include "utils.h"

#include <bitset>
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
    this->dev = nullptr;
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
    if (this->dev != nullptr) {
        libevdev_free(this->dev);
        close(this->pdev_fd);
    }

    if (this->vdev_fd != -1) {
        ioctl(this->vdev_fd, UI_DEV_DESTROY);
        close(this->vdev_fd);
    }
};

void MouseRot::open_mouse()
{
    // this->pdev_fd = open(this->device_path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    this->pdev_fd = open(this->device_path.c_str(), O_RDONLY | O_CLOEXEC);
    if (this->pdev_fd == -1)
        throw std::system_error(errno, std::generic_category(), "Failed opening device");

    int rc = libevdev_new_from_fd(this->pdev_fd, &dev);
    if (rc < 0)
        throw std::system_error(errno, std::generic_category(), "Failed to init libevdev");

    spdlog::info(
        "Opened device:\n\tName: {}\n\tBus: {:x}\n\tVendor: {:x}\n\tProduct: {:x}",
        libevdev_get_name(dev),
        libevdev_get_id_bustype(dev),
        libevdev_get_id_vendor(dev),
        libevdev_get_id_product(dev));

    if (!looks_like_mouse(dev))
        throw std::runtime_error("Device does not look like a mouse!");
};

void MouseRot::grab_mouse()
{
    if (this->pdev_fd == -1)
        throw std::runtime_error("No open mouse device");

    if (!this->dev)
        throw std::runtime_error("Can't grab mouse device we haven't initialized");

    int res = libevdev_grab(this->dev, LIBEVDEV_GRAB);
    if (res < 0)
        throw std::system_error(
            res,
            std::generic_category(),
            "Failed to grab device. Maybe mouserot is already running?");
};

template <size_t N> class Bitset {
    std::array<uint8_t, N / 8 + 1> arr;

public:
    Bitset()
    {
        this->reset();
    };

    void reset()
    {
        this->arr.fill(0);
    }

    size_t size()
    {
        return N;
    }

    uint8_t* data()
    {
        return this->arr.data();
    }

    bool test(size_t index)
    {
        return this->arr[index / 8] & (1 << (index % 8));
    }
};

void MouseRot::create_virtual_mouse()
{
    if (this->pdev_fd == -1)
        throw std::runtime_error("Can't create virtual mouse before opening physical mouse");

    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0)
        throw std::system_error(errno, std::generic_category(), "Failed to open /dev/uinput");

    this->vdev_fd = fd;

    struct uinput_setup usetup {};

    // https://github.com/gvalkov/python-evdev/blob/0d496bf8a5bce2d5c60147609cb79df1386dbf23/evdev/input.c#L130
    // https://github.com/gvalkov/python-evdev/blob/0d496bf8a5bce2d5c60147609cb79df1386dbf23/evdev/uinput.c#L302

    Bitset<EV_MAX> ev_bits;
    Bitset<KEY_MAX> code_bits;

    std::map<int, int> ev_type_map = {
        {EV_KEY, UI_SET_KEYBIT},
        {EV_ABS, UI_SET_ABSBIT},
        {EV_REL, UI_SET_RELBIT},
        {EV_MSC, UI_SET_MSCBIT},
        {EV_SW, UI_SET_SWBIT},
        {EV_LED, UI_SET_LEDBIT},
        {EV_FF, UI_SET_FFBIT},
        {EV_SND, UI_SET_SNDBIT},
    };

    ioctl(this->pdev_fd, EVIOCGBIT(0, ev_bits.size()), ev_bits.data());

    spdlog::info("Probing physical mouse capabilities");

    for (int ev_type = 0; ev_type < EV_MAX; ++ev_type) {
        if (!ev_bits.test(ev_type))
            continue;

        if (ev_type == EV_SYN)
            continue;

        std::string info = libevdev_event_type_get_name(ev_type);
        info += " (";

        if (ev_type_map.find(ev_type) == ev_type_map.end()) {
            std::string ev_type_name = libevdev_event_type_get_name(ev_type);
            spdlog::warn("ev_type {} does not map to a UI_SET_*BIT value", ev_type_name);
            continue;
        }

        // set that we want to enable this evbit
        ioctl(this->vdev_fd, UI_SET_EVBIT, ev_type);

        code_bits.reset();
        ioctl(this->pdev_fd, EVIOCGBIT(ev_type, code_bits.size()), code_bits.data());

        bool first = true;

        for (int ev_code = 0; ev_code < KEY_MAX; ++ev_code) {
            if (!code_bits.test(ev_code))
                continue;

            int req = ev_type_map[ev_type];
            ioctl(this->vdev_fd, req, ev_code);

            std::string event_code_name = libevdev_event_code_get_name(ev_type, ev_code);

            if (first) {
                first = false;
            } else {
                info += ", ";
            }

            info += event_code_name;
        }

        info += ")";
        spdlog::info(info);
    }

    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;  // Dummy vendor
    usetup.id.product = 0x5678; // Dummy product
    strcpy(usetup.name, "mouserot virtual mouse");

    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);
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

    while (true) {
        // returns -EAGAIN if nothing is there
        rc = libevdev_next_event(this->dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
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

    struct input_event event {};

    event.input_event_usec = 0;
    event.input_event_sec = 0;
    event.type = ev_type;
    event.code = ev_code;
    event.value = value;

    write(this->vdev_fd, &event, sizeof(event));
};