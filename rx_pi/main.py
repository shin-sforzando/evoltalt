#!/usr/bin/env python
# -*- coding: utf-8 -*-

from datetime import datetime
import re
import serial
import sys
import time

import bme280

import ibmiotf.application
import ibmiotf.device

try:
    options = ibmiotf.device.ParseConfigFile("./config.cfg")
    client = ibmiotf.device.Client(options)
except ibmiotf.ConnectionException as e:
    print str(e)
    sys.exit()

regex = r"(?P<node>\d{2}),(?P<sender>\w{4}),(?P<rssi>\w{2}):\s(?P<charge>\d+)%\s(?P<pressure>\d+\.\d+)hPa\s(?P<temperature>\d+\.\d+)C\s(?P<humidity>\d+\.\d+)%"

pattern = re.compile(regex)

with serial.Serial("/dev/ttyUSB0", 19200) as ser:
    client.connect()
    while ser.isOpen():
        rx_msg = ser.readline()
        m = pattern.match(rx_msg)
        if m:
            now = datetime.now().strftime("%Y/%m/%d %H:%M:%S")
            base_temperature, base_pressure, base_humidity = bme280.readBME280All()
            print now, rx_msg,
            data = {"time": now, "sender": m.group("sender"), "rssi": int(m.group("rssi"), 16), "charge": int(m.group("charge")),
                    "base_temperature": base_temperature, "base_pressure": base_pressure, "base_humidity": base_humidity,
                    "current_temperature": float(m.group("temperature")), "current_pressure": float(m.group("pressure")), "current_humidity": float(m.group("humidity")), "altitude": 0.0}

            def on_publish_callback():
                print data["time"], "Confirmed."

            success = client.publishEvent("evoltalt", "json", data, qos=1, on_publish=on_publish_callback)
            if not success:
                print "Not connected IoTF."

    client.disconnect()
