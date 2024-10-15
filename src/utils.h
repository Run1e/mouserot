#pragma once

#include <libevdev-1.0/libevdev/libevdev.h>

inline bool looks_like_mouse(struct libevdev* dev)
{
    return libevdev_has_event_type(dev, EV_REL) && libevdev_has_event_code(dev, EV_KEY, BTN_LEFT);
}