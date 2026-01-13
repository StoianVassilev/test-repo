#!/usr/bin/env python3
"""
Raspberry Pi startup Email Notification Script
Sends an email with the Pi's serial number and IP address, when FEH starts.
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

def send_email():
    """Send an email with Pi information."""
    serial = get_serial_number()
    ip_address = get_ip_address()
    hostname = get_hostname()
    current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    subject = f"RPi Startup Report: {hostname} : {ip_address} : {serial} : {current_time}"
    body = f"""
Raspberry Pi Startup Report: FEH just started on your Raspberry Pi.
==========================
Hostname:      {hostname}
IP Address:    {ip_address}
Date/Time:     {current_time}
Serial Number: {serial}

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
