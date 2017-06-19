#!/usr/bin/env python
# -*- coding: utf-8 -*-

from datetime import datetime
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

with serial.Serial("/dev/ttyUSB0", 19200) as ser:
    client.connect()
    while ser.isOpen():
        rx_msg = ser.readline()
        if rx_msg != "":
            now = datetime.now().strftime("%Y/%m/%d %H:%M:%S")
            base_temperature, base_pressure, base_humidity = bme280.readBME280All()
            print now, rx_msg
            data = {"time": now, "sender": "", "rssi": "", "charge": 0.0,
                    "base_temperature": base_temperature, "base_pressure": base_pressure, "base_humidity": base_humidity,
                    "current_temperature": 0.0, "current_pressure": 0.0, "current_humidity": 0.0, "altitude": 0.0}

            def on_publish_callback():
                print "Confirmed."

            success = client.publishEvent("evoltalt", "json", data, qos=1, on_publish=on_publish_callback)
            if not success:
                print "Not connected IoTF."

    client.disconnect()
