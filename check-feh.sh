#!/bin/bash

if pgrep -x "feh" > /dev/null; then
    echo "FEH is already running"
else
    echo "FEH is not running. Starting it..."
    DISPLAY=:0.0 XAUTHORITY=/home/stoian/.Xauthority /usr/bin/feh --draw-filename --quiet --randomize --info "echo IP:$(hostname -I)" --recursive --auto-zoom --fullscreen --hide-pointer --slideshow-delay 60.0 /home/stoian/Pictures /media/pi /boot/Pictures &
    echo "FEH started"
fi

#test change#1
