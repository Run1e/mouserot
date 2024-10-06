# mouserot

A simple Linux tool to change mouse scaling and rotation

## Usage

```bash
runie@dev> mouserot -h
SYNOPSIS
        mouserot apply <device> <scaling> <rotation>
        mouserot list [--by-id]
        mouserot help

OPTIONS
        --by-id     list by id
```

### List mouse devices

```bash
runie@dev> sudo mouserot list
Keychron Keychron Q3 Mouse       -> /dev/input/event3
Logitech G403                    -> /dev/input/event15
Logitech USB Receiver            -> /dev/input/event13
Razer Razer Viper V2 Pro         -> /dev/input/event7
Razer Razer Viper V2 Pro Mouse   -> /dev/input/event9
```

Alternatively get paths by id by adding `--by-id`:

```bash
runie@dev> sudo mouserot list --by-id
Keychron Keychron Q3 Mouse       -> /dev/input/by-id/usb-Keychron_Keychron_Q3-if02-event-mouse
Logitech G403                    -> /dev/input/by-id/usb-Logitech_USB_Receiver-if02-event-mouse
Logitech USB Receiver            -> /dev/input/by-id/usb-Logitech_USB_Receiver-event-mouse
Razer Razer Viper V2 Pro         -> /dev/input/by-id/usb-Razer_Razer_Viper_V2_Pro_000000000000-event-mouse
Razer Razer Viper V2 Pro Mouse   -> /dev/input/by-id/usb-Razer_Razer_Viper_V2_Pro_000000000000-if01-event-mouse
```

### Apply scaling and rotation

After getting the path to a pointer device, you can apply scaling and rotation:

```bash
runie@dev> sudo mouserot apply /dev/input/by-id/usb-Razer_Razer_Viper_V2_Pro_000000000000-event-mouse 0.5 1.5
```

The above example scales the movement by `0.5` and adds a `1.5` degree rotation, which is my configuration with a Viper V2 Pro @ 1600dpi.

## How?

In a normal setup, input events are handled roughly like:

```
kernel -> pointer device -> libinput -> X server -> X client
```

However, we can "grab" exclusive access to a pointer device, such that libinput can no longer received events from it.
If we then create a [virtual pointer device](https://www.kernel.org/doc/html/v4.12/input/uinput.html)
we can forward events to it, after applying scaling and rotation as needed.

The flow will then look more like:

```
kernel -> pointer device -> mouserot -> mouserot virtual pointer -> libinput -> X server -> X client
```

## Building

todo


## TODO

- Compilation instructions
- Installation
- systemd service configuration/setup
- Test delay/profiling
