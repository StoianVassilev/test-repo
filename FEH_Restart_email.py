#!/usr/bin/env python3
"""
Raspberry Pi FEH Restart Email Notification Script
Sends an email with the Pi's serial number and IP address, when FEH RE-starts.
"""

import smtplib
import socket
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from datetime import datetime

# Gmail configuration - UPDATE THESE VALUES
GMAIL_USER = "dbk32rpi@gmail.com"
GMAIL_APP_PASSWORD = "iexa pacw fvgc lvib"  # Use App Password, not regular password
RECIPIENT_EMAIL = "stoian.vassilev@gmail.com"

def get_serial_number():
    """Get the Raspberry Pi serial number from /proc/cpuinfo."""
    try:
        with open('/proc/cpuinfo', 'r') as f:
            for line in f:
                if line.startswith('Serial'):
                    return line.strip().split(':')[1].strip()
    except Exception as e:
        return f"Unable to get serial: {e}"
    return "Serial not found"

def get_ip_address():
    """Get the primary IP address of the Raspberry Pi."""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception as e:
        return f"Unable to get IP: {e}"

def get_hostname():
    """Get the hostname of the Raspberry Pi."""
    return socket.gethostname()

def get_cpu_temp():
    """Get the CPU temperature."""
    try:
        with open('/sys/class/thermal/thermal_zone0/temp', 'r') as f:
            temp = int(f.read()) / 1000
            return f"{temp:.1f}°C"
    except Exception as e:
        return f"Unable to get temp: {e}"

def get_pi_model():
    """Get the Raspberry Pi model."""
    try:
        with open('/proc/device-tree/model', 'r') as f:
            return f.read().strip().replace('\x00', '')
    except Exception as e:
        return f"Unable to get model: {e}"

def get_memory_info():
    """Get memory usage information."""
    try:
        with open('/proc/meminfo', 'r') as f:
            lines = f.readlines()
            mem_total = int(lines[0].split()[1]) // 1024
            mem_free = int(lines[1].split()[1]) // 1024
            mem_used = mem_total - mem_free
            return f"{mem_used}MB / {mem_total}MB"
    except Exception as e:
        return f"Unable to get memory: {e}"

def get_uptime():
    """Get system uptime."""
    try:
        with open('/proc/uptime', 'r') as f:
            uptime_seconds = float(f.read().split()[0])
            days = int(uptime_seconds // 86400)
            hours = int((uptime_seconds % 86400) // 3600)
            minutes = int((uptime_seconds % 3600) // 60)
            return f"{days}d {hours}h {minutes}m"
    except Exception as e:
        return f"Unable to get uptime: {e}"

def send_email():
    """Send an email with Pi information."""
    serial = get_serial_number()
    ip_address = get_ip_address()
    hostname = get_hostname()
    current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    cpu_temp = get_cpu_temp()
    pi_model = get_pi_model()
    memory = get_memory_info()
    uptime = get_uptime()

    subject = f"RPi FEH RESTART Report: {hostname} : {ip_address} : {serial} : {current_time}"
    body = f"""
FEH just re-started on your Raspberry Pi:

System Information:
-------------------
Hostname:      {hostname}
IP Address:    {ip_address}
Date/Time:     {current_time}
Serial Number: {serial}

Hardware Information:
---------------------
Model:         {pi_model}
CPU Temp:      {cpu_temp}
Memory Usage:  {memory}
Uptime:        {uptime}

This message was sent automatically from your Raspberry Pi.
"""

    msg = MIMEMultipart()
    msg['From'] = GMAIL_USER
    msg['To'] = RECIPIENT_EMAIL
    msg['Subject'] = subject
    msg.attach(MIMEText(body, 'plain'))

    try:
        server = smtplib.SMTP('smtp.gmail.com', 587)
        server.starttls()
        server.login(GMAIL_USER, GMAIL_APP_PASSWORD)
        server.sendmail(GMAIL_USER, RECIPIENT_EMAIL, msg.as_string())
        server.quit()
        print(f"Email sent successfully to {RECIPIENT_EMAIL}")
    except Exception as e:
        print(f"Failed to send email: {e}")

if __name__ == "__main__":
    send_email()
