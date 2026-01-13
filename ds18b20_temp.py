#!/usr/bin/env python3
"""
DS18B20 Temperature Sensor Reader
Reads temperature from DS18B20 sensor via 1-Wire interface.

Setup:
1. Enable 1-Wire: sudo raspi-config -> Interface Options -> 1-Wire -> Enable
2. Or add to /boot/config.txt: dtoverlay=w1-gpio
3. Connect DS18B20: VCC->3.3V, GND->GND, DATA->GPIO4 (with 4.7k pull-up resistor)
4. Reboot the Pi
"""

import glob
import time

# 1-Wire device path
BASE_DIR = '/sys/bus/w1/devices/'
DEVICE_FOLDER = glob.glob(BASE_DIR + '28*')

def get_sensor_path():
    """Find the DS18B20 sensor device file."""
    if not DEVICE_FOLDER:
        raise FileNotFoundError("No DS18B20 sensor found. Check wiring and 1-Wire is enabled.")
    return DEVICE_FOLDER[0] + '/w1_slave'

def read_temp_raw(sensor_path):
    """Read raw data from the sensor."""
    with open(sensor_path, 'r') as f:
        return f.readlines()

def read_temperature(sensor_path):
    """Parse temperature from sensor data."""
    lines = read_temp_raw(sensor_path)
    
    # Wait for valid reading (CRC check)
    while lines[0].strip()[-3:] != 'YES':
        time.sleep(0.2)
        lines = read_temp_raw(sensor_path)
    
    # Find temperature value
    equals_pos = lines[1].find('t=')
    if equals_pos != -1:
        temp_string = lines[1][equals_pos + 2:]
        temp_c = float(temp_string) / 1000.0
        temp_f = temp_c * 9.0 / 5.0 + 32.0
        return temp_c, temp_f
    return None, None

def main():
    try:
        sensor_path = get_sensor_path()
        print(f"Found sensor: {DEVICE_FOLDER[0]}")
        print("Reading temperature (Ctrl+C to stop)...\n")
        
        while True:
            temp_c, temp_f = read_temperature(sensor_path)
            if temp_c is not None:
                print(f"Temperature: {temp_c:.3f}°C / {temp_f:.2f}°F")
            else:
                print("Error reading temperature")
            #time.sleep(1)
            
    except FileNotFoundError as e:
        print(f"Error: {e}")
    except KeyboardInterrupt:
        print("\nStopped.")

if __name__ == "__main__":
    main()
