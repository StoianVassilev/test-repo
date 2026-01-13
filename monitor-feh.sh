#!/bin/bash

while true; do
    if pgrep -x "feh" > /dev/null; then
        echo "$(date): FEH is running"
    else
        echo "$(date): FEH is not running. Starting it..."
        DISPLAY=:0.0 XAUTHORITY=/home/stoian/.Xauthority /usr/bin/feh --draw-filename --quiet --randomize --info "echo IP:$(hostname -I)" --recursive --auto-zoom --fullscreen --hide-pointer --slideshow-delay 60.0 /home/stoian/Pictures /media/pi /boot/Pictures &
        echo "$(date): FEH started"
        /home/stoian/FEH_Restart_email.py
    fi
    sleep 600
done
