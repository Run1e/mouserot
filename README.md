# mouserot

A simple Linux tool to change mouse scaling and rotation

## Usage

```bash
runie@dev> mouserot -h
SYNOPSIS
        mouserot apply <device> <scaling> <rotation>
        mouserot list [--by-id]
        mouserot daemon <config>
        mouserot help

OPTIONS
        --by-id     list by id
```

### List mouse devices

```bash
runie@dev> sudo mouserot list
Keychron Keychron Q3 Mouse       -> /dev/input/event3
Logitech G403                    -> /dev/input/event20
Logitech USB Receiver            -> /dev/input/event17
Razer Razer Viper V2 Pro         -> /dev/input/event7
Razer Razer Viper V2 Pro Mouse   -> /dev/input/event9
Razer Razer Viper V3 Pro         -> /dev/input/event12
Razer Razer Viper V3 Pro Mouse   -> /dev/input/event14
```

### Apply scaling and rotation

After getting the path to a pointer device, you can apply scaling and rotation:

```bash
runie@dev> sudo mouserot apply /dev/input/event12 0.5 1.4
```

The above example scales the movement by `0.5` and adds a `1.4` degree rotation, which is my configuration with a Viper V3 Pro @ 1600dpi.

## How?

In a normal setup, input events are handled roughly like:

```
kernel -> pointer device -> libinput -> X server -> X client
```

However, we can "grab" exclusive access to a pointer device, such that libinput can no longer receive events from it.
Then we can create a [virtual pointer device](https://www.kernel.org/doc/html/v4.12/input/uinput.html)
which we can forward events to, after they have been scaled and rotated as needed.

The flow will then look more like:

```
kernel -> pointer device -> mouserot -> virtual pointer device -> libinput -> X server -> X client
```

## Installation

### Dependencies

```bash
sudo apt install cmake build-essential libevdev-dev pkg-config
```

### Building and installing

First clone the repo with the `--recursive` flag:

```bash
git clone --recursive https://github.com/Run1e/mouserot.git
```

Alternatively clone normally and then run `git submodule update --init`.

Then run the build script:

```bash
./build.sh
```

You will be prompted if you want to copy the mouserot binary to `/usr/local/bin/mouserot`.

The build is emitted to `build/Release`.

### Daemon

If you want to install mouserot as a systemd service:

```bash
sudo ./systemd-install.sh

# configure the daemon
sudo vim/nano /etc/mouserot/config.yaml

# enable and start
sudo systemctl enable mouserot
sudo systemctl start mouserot

# check to see if everything went well
sudo systemctl status mouserot
```
