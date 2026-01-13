sleep 60
DISPLAY=:0.0 XAUTHORITY=/home/stoian/.Xauthority /usr/bin/feh --draw-filename --quiet --randomize --info echo "IP:$(hostname -I)" --recursive --auto-zoom --fullscreen --hide-pointer --slideshow-delay 60.0 /home/stoian/Pictures /media/pi /boot/Pictures &
#sleep 10
./Startup_email.py
