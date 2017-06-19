#!/usr/bin/env python
# -*- coding: utf-8 -*-

from datetime import datetime
from logging import getLogger, StreamHandler, FileHandler, Formatter, DEBUG
import os
import re
import serial
import sys
import time

import bme280

import ibmiotf.application
import ibmiotf.device

logger = getLogger(__name__)
formatter = Formatter("[%(levelname)s] %(asctime)s - %(message)s", datefmt="%H:%M:%S %Z")
stream_handler = StreamHandler()
stream_handler.setLevel(DEBUG)
stream_handler.setFormatter(formatter)
file_handler = FileHandler(filename=datetime.now().strftime("%Y%m%d") + ".log")
file_handler.setLevel(DEBUG)
file_handler.setFormatter(formatter)
logger.addHandler(stream_handler)
logger.addHandler(file_handler)
logger.setLevel(DEBUG)

try:
    config_path = os.path.join(os.path.dirname(__file__), 'config.cfg')
    options = ibmiotf.device.ParseConfigFile(config_path)
    client = ibmiotf.device.Client(options)
except ibmiotf.ConnectionException as e:
    logger.error(str(e))
    sys.exit()

regex = r"(?P<node>\d{2}),(?P<sender>\w{4}),(?P<rssi>\w{2}):\s(?P<charge>\d+)%\s(?P<pressure>\d+\.\d+)hPa\s(?P<temperature>\d+\.\d+)C\s(?P<humidity>\d+\.\d+)%"
pattern = re.compile(regex)


def estimate_altitude(base_pressure, current_pressure, current_temperature):
    return (((current_pressure / base_pressure) ** (1.0 / 5.275) - 1.0) * (current_temperature + 273.15)) / 0.0065


with serial.Serial("/dev/ttyUSB0", 19200) as ser:
    client.connect()
    while ser.isOpen():
        rx_msg = ser.readline()
        m = pattern.match(rx_msg)
        if m:
            now = datetime.now().strftime("%Y/%m/%d %H:%M:%S")

            current_sender = m.group("sender")
            current_rssi = int(m.group("rssi"), 16)
            current_charge = int(m.group("charge"))
            current_pressure = float(m.group("pressure"))
            current_temperature = float(m.group("temperature"))
            current_humidity = float(m.group("humidity"))
            logger.info("current: %s", m.group())

            base_temperature, base_pressure, base_humidity = bme280.readBME280All()
            logger.info("base: %shPa %sC %s%%", base_pressure, base_temperature, base_humidity)

            data = {"time": now, "sender": current_sender, "rssi": current_rssi, "charge": current_charge,
                    "base_temperature": base_temperature, "base_pressure": base_pressure, "base_humidity": base_humidity,
                    "current_temperature": current_temperature, "current_pressure": current_pressure, "current_humidity": current_humidity, "altitude": estimate_altitude(base_pressure, current_pressure, current_temperature)}

            def on_publish_callback():
                logger.info("Published the data received at %s.", data["time"])

            success = client.publishEvent("evoltalt", "json", data, qos=1, on_publish=on_publish_callback)
            if not success:
                logger.error("Not connected IoTF.")

    client.disconnect()
