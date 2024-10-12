#!/bin/bash

if [ "$#" -eq 0 ]; then
    build_type=Release
    echo "No build type specified, defaulting to 'Release'"
else
    build_type="$1"
fi;

if [[ "$build_type" != "Debug" && "$build_type" != "Release" ]]; then
    echo "Only 'Debug' and 'Release' build types are allowed"
    exit;
fi;

folder=build/${build_type}
mkdir -p ${folder} && cd ${folder}
cmake -DCMAKE_BUILD_TYPE=${build_type} ../..
make

if [[ "$build_type" == "Debug" ]]; then
    exit;
fi;

echo
read -p "Do you want to install the mouserot binary to /usr/local/bin? [yes/no] " answer

answer=${answer,,}

if [[ "$answer" == "yes" || "$answer" == "y" ]]; then
    sudo make install
    echo
    echo
    echo "To install the systemd service, run 'sudo ./systemd-install.sh'"
else
    echo "The systemd_install.sh script will not work without installing mouserot to /usr/local/bin. Run this script again if you change your mind."
fi