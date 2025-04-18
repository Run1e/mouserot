#include "clipp.h"
#include "mouserot.h"
#include "spdlog/spdlog.h"
#include "utils.h"
#include "yaml-cpp/yaml.h"

#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <libevdev/libevdev.h>
#include <linux/uinput.h>
#include <map>
#include <system_error>
#include <unistd.h>

using namespace clipp;
namespace fs = std::filesystem;

void list(bool by_id)
{
    std::string device_name;
    long unsigned int name_width = 0;
    std::map<std::string, fs::path> map{};

    std::string device_path = by_id ? "/dev/input/by-id" : "/dev/input";

    for (const auto& entry : fs::directory_iterator(device_path)) {
        if (!entry.is_character_file())
            continue;

        const auto perms = fs::status(entry.path()).permissions();

        if ((perms & fs::perms::owner_read) == fs::perms::none)
            continue;

        if ((perms & fs::perms::owner_write) == fs::perms::none)
            continue;

        struct libevdev* dev;

        int fd = open(entry.path().c_str(), O_RDONLY | O_EXCL);
        int ret = libevdev_new_from_fd(fd, &dev);

        if (ret >= 0) {
            if (looks_like_mouse(dev)) {
                device_name = libevdev_get_name(dev);

                if (device_name.length() > name_width)
                    name_width = device_name.length();

                map[device_name] = entry.path();
            }

            libevdev_free(dev);
        }

        close(fd);
    };

    if (map.size()) {
        for (const auto& [name, path] : map) {
            std::cout << std::left << std::setw(static_cast<int>(name_width)) << std::setfill(' ') << name;
            std::cout << "   -> " << path.c_str() << std::endl;
        }
    } else {
        std::cout << "No output. Are you running as root?" << std::endl;
    }
};

void apply(std::string device, float scaling, float rotation)
{
    spdlog::info("Applying with:\n\tDevice: {}\n\tScaling: {}\n\tRotation: {}", device, scaling, rotation);

    std::unique_ptr<MouseRot> mr = std::make_unique<MouseRot>(device);

    mr->open_mouse();
    mr->create_virtual_mouse();
    mr->grab_mouse();

    mr->set_rotation_deg(rotation);
    mr->set_scale(scaling);

    while (true) {
        try {
            spdlog::info("Starting!");
            mr->loop();
        } catch (const std::system_error& exc) {
            spdlog::error(exc.what());

            if (exc.code().value() == ENODEV) {
                spdlog::info("Device not found, attempting to reopen...");

                while (true) {
                    sleep(2);

                    try {
                        mr->open_mouse();
                        mr->grab_mouse();
                    } catch (const std::runtime_error& open_exc) {
                        spdlog::error(open_exc.what());
                        continue;
                    }

                    break;
                }
            }
        }
    }
}

void daemon(const std::string& config)
{
    spdlog::info("Starting in daemon mode (config: {})", config);

    fs::path path(config);
    if (!fs::exists(path))
        throw std::runtime_error("Config file doesn't exist");

    YAML::Node node = YAML::LoadFile(config);

    const std::string device = node["device"].as<std::string>();
    const float scaling = node["scaling"].as<float>();
    const float rotation = node["rotation"].as<float>();

    apply(device, scaling, rotation);
}

enum class mode { list, apply, daemon, help };

int main(int argc, char* argv[])
{
#ifdef DEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    std::string device;
    std::string config;
    bool by_id = false;
    float rotation = 0.0;
    float scaling = 1.0;
    mode selected = mode::help;

    auto apply_cmd =
        (command("apply").set(selected, mode::apply),
         value("device", device),
         value("scaling", scaling),
         value("rotation", rotation));

    auto list_cmd = (command("list").set(selected, mode::list), option("--by-id").set(by_id) % "list by id");

    auto daemon_cmd = (command("daemon").set(selected, mode::daemon), value("config", config));

    auto cli = (apply_cmd | list_cmd | daemon_cmd | command("help").set(selected, mode::help));

    std::function<void(void)> show_help = [&cli, &argv] { std::cout << make_man_page(cli, argv[0]) << std::endl; };

    if (parse(argc, argv, cli)) {
        try {
            switch (selected) {
                case mode::apply:
                    apply(device, scaling, rotation);
                    break;
                case mode::list:
                    list(by_id);
                    break;
                case mode::daemon:
                    daemon(config);
                    break;
                case mode::help:
                    show_help();
                    break;
            }
        } catch (const std::runtime_error& exc) {
            spdlog::critical(exc.what());
            exit(-1);
        }

        return 0;
    }

    show_help();
};
