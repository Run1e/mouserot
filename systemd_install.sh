#!/bin/bash

if [ "$EUID" -ne 0 ]; then
    echo "Please run this script as root"
    exit 1
fi

echo -e

mkdir /etc/mouserot
cp config.example.yaml /etc/mouserot/config.yaml

cp mouserot.service /usr/lib/systemd/system

systemctl daemon-reload

echo
echo "========================================================================"
echo
echo "README"
echo
echo "Edit /etc/mouserot/config.yaml to configure the daemon."
echo "The available device values can be found with 'sudo mouserot list --by-id'"
echo
echo "Then you can enable and start the daemon with:"
echo "systemctl enable mouserot"
echo "systemctl start mouserot"