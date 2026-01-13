#!/bin/bash
# If FEH is NOT working, then send an email
#ps --no-headers -C feh || echo "The FEH has been restarted!" | mail -s "RasPi#x: FEH restart" stoian.vassilev@gmail.com
# then kick FEH again
#ps --no-headers -C feh || DISPLAY=:0.0 XAUTHORITY=/home/pi/.Xauthority /usr/bin/feh --draw-filename --quiet --randomize --recursive --reload 600 --auto-zoom --fullscreen --hide-pointer --slideshow-delay 60.0 /home/pi/Pictures /media/pi /boot/Pictures &
#ps --no-headers -C feh || DISPLAY=:0.0 XAUTHORITY=/home/stoian/.Xauthority /usr/bin/feh --draw-filename --quiet --info "echo 'IP:$(hostname -I)'" --randomize --recursive --auto-zoom --fullscreen --hide-pointer --slideshow-delay 60.0 /home/stoian/Pictures /media/pi /boot/Pictures &

if pgrep -x "feh" > /dev/null; then
    : # FEH is already running
else
    echo "FEH is not running. Starting it..."
    DISPLAY=:0.0 XAUTHORITY=/home/stoian/.Xauthority /usr/bin/feh --draw-filename --quiet --randomize --info echo IP:$(hostname -I) --recursive --auto-zoom --fullscreen --hide-pointer --slideshow-delay 60.0 /home/stoian/Pictures /media/pi /boot/Pictures &
    echo "FEH started"
    echo "Sending an email"
    ./FEH_Restart_email.py
fi