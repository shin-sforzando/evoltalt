#!/usr/bin/env python
# -*- coding: utf-8 -*-

from datetime import datetime
from logging import getLogger, FileHandler, Formatter, DEBUG
import os
import re
import serial
import sys
import time

import bme280
import coloredlogs
import dweepy
import requests


os.environ["COLOREDLOGS_LOG_FORMAT"] = "[%(levelname)s]\t%(asctime)s - %(message)s"
os.environ["COLOREDLOGS_DATE_FORMAT"] = "%H:%M:%S"

working_path = os.path.dirname(__file__)
log_path = os.path.join(working_path, datetime.now().strftime("%Y%m%d") + ".log")

logger = getLogger(__name__)
coloredlogs.install(level="DEBUG", logger=logger)
formatter = Formatter("[%(levelname)s]\t%(asctime)s - %(message)s", datefmt="%H:%M:%S %Z")
file_handler = FileHandler(filename=log_path)
file_handler.setLevel(DEBUG)
file_handler.setFormatter(formatter)
logger.addHandler(file_handler)
logger.setLevel(DEBUG)

regex = r"(?P<node>\d{2}),(?P<sender>\w{4}),(?P<rssi>\w{2}):\s(?P<charge>\d+)%\s(?P<pressure>\d+\.\d+)hPa\s(?P<temperature>\d+\.\d+)C\s(?P<humidity>\d+\.\d+)%"
pattern = re.compile(regex)


def estimate_altitude(base_pressure, current_pressure, current_temperature):
    return (((current_pressure / base_pressure) ** (1.0 / 5.275) - 1.0) * (current_temperature + 273.15)) / 0.0065


def get_location():
    r = requests.get("http://ipinfo.io/")
    return r.json()["loc"].split(",")


lat, lng = get_location()
logger.info("Latitude: %s Longitude: %s", lat, lng)


with serial.Serial("/dev/ttyUSB0", 19200) as ser:
    with requests.Session() as session:
        while ser.isOpen():
            rx_msg = ser.readline()
            m = pattern.match(rx_msg.decode('utf-8'))
            if m:
                now = datetime.now()

                base_temperature, base_pressure, base_humidity = bme280.readBME280All()
                logger.info("Base    : %shPa %sC %s%%", base_pressure, base_temperature, base_humidity)

                logger.info("Current : %s", m.group())
                current_sender = m.group("sender")
                current_rssi = int(m.group("rssi"), 16)
                current_charge = int(m.group("charge"))
                current_pressure = float(m.group("pressure"))
                current_temperature = float(m.group("temperature"))
                current_humidity = float(m.group("humidity"))
                current_altitude = estimate_altitude(base_pressure, current_pressure, current_temperature)
                logger.info("Altitude: %s[m]", current_altitude)

                data = {"timestamp": now.isoformat(), "sender": current_sender, "rssi": current_rssi, "charge": current_charge,
                        "base_temperature": base_temperature, "base_pressure": base_pressure, "base_humidity": base_humidity,
                        "current_temperature": current_temperature, "current_pressure": current_pressure, "current_humidity": current_humidity, "altitude": current_altitude,
                        "latitude": lat, "longitude": lng}

                try:
                    dweepy.dweet_for("evoltalt", data, session=session)
                except dweepy.api.DweepyError as de:
                    logger.error(de)
                    time.sleep(1)
