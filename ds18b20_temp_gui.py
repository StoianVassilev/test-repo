#!/usr/bin/env python3
"""
DS18B20 Temperature Sensor Reader with GUI
Reads temperature from DS18B20 sensor via 1-Wire interface.

Setup:
1. Enable 1-Wire: sudo raspi-config -> Interface Options -> 1-Wire -> Enable
2. Or add to /boot/config.txt: dtoverlay=w1-gpio
3. Connect DS18B20: VCC->3.3V, GND->GND, DATA->GPIO4 (with 4.7k pull-up resistor)
4. Reboot the Pi
"""

import glob
import time
import tkinter as tk
from tkinter import font

# 1-Wire device path
BASE_DIR = '/sys/bus/w1/devices/'
DEVICE_FOLDER = glob.glob(BASE_DIR + '28*')

def get_sensor_path():
    """Find the DS18B20 sensor device file."""
    if not DEVICE_FOLDER:
        return None
    return DEVICE_FOLDER[0] + '/w1_slave'

def read_temp_raw(sensor_path):
    """Read raw data from the sensor."""
    with open(sensor_path, 'r') as f:
        return f.readlines()

def read_temperature(sensor_path):
    """Parse temperature from sensor data."""
    try:
        lines = read_temp_raw(sensor_path)
        
        # Wait for valid reading (CRC check)
        if lines[0].strip()[-3:] != 'YES':
            return None, None
        
        # Find temperature value
        equals_pos = lines[1].find('t=')
        if equals_pos != -1:
            temp_string = lines[1][equals_pos + 2:]
            temp_c = float(temp_string) / 1000.0
            temp_f = temp_c * 9.0 / 5.0 + 32.0
            return temp_c, temp_f
    except Exception:
        pass
    return None, None

class TemperatureGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("DS18B20 Temperature Monitor")
        self.root.geometry("400x200")
        self.root.configure(bg='#2c3e50')
        
        self.sensor_path = get_sensor_path()
        
        # Title label
        title_font = font.Font(family='Helvetica', size=14, weight='bold')
        self.title_label = tk.Label(
            root, 
            text="DS18B20 Temperature", 
            font=title_font, 
            fg='white', 
            bg='#2c3e50'
        )
        self.title_label.pack(pady=10)
        
        # Temperature display
        temp_font = font.Font(family='Helvetica', size=48, weight='bold')
        self.temp_label = tk.Label(
            root, 
            text="--.-°C", 
            font=temp_font, 
            fg='#3498db', 
            bg='#2c3e50'
        )
        self.temp_label.pack(pady=10)
        
        # Fahrenheit display
        f_font = font.Font(family='Helvetica', size=18)
        self.temp_f_label = tk.Label(
            root, 
            text="--.-°F", 
            font=f_font, 
            fg='#95a5a6', 
            bg='#2c3e50'
        )
        self.temp_f_label.pack()
        
        # Status label
        self.status_label = tk.Label(
            root, 
            text="", 
            font=('Helvetica', 10), 
            fg='#e74c3c', 
            bg='#2c3e50'
        )
        self.status_label.pack(pady=5)
        
        if not self.sensor_path:
            self.status_label.config(text="No sensor found! Check wiring.")
        
        # Start updating
        self.update_temperature()
    
    def update_temperature(self):
        if self.sensor_path:
            temp_c, temp_f = read_temperature(self.sensor_path)
            if temp_c is not None:
                self.temp_label.config(text=f"{temp_c:.1f}°C")
                self.temp_f_label.config(text=f"{temp_f:.1f}°F")
                self.status_label.config(text="")
                
                # Change color based on temperature
                if temp_c < 20:
                    self.temp_label.config(fg='#3498db')  # Blue - cold
                elif temp_c < 30:
                    self.temp_label.config(fg='#2ecc71')  # Green - normal
                else:
                    self.temp_label.config(fg='#e74c3c')  # Red - hot
            else:
                self.status_label.config(text="Read error")
        
        # Update every 1 second
        #self.root.after(1000, self.update_temperature)

def main():
    root = tk.Tk()
    app = TemperatureGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main()
