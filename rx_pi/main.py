#!/usr/bin/env python
# -*- coding: utf-8 -*-

from datetime import datetime
from logging import getLogger, FileHandler, Formatter, DEBUG
import os
import re
import serial
import sys
import time
import csv

import bme280
import coloredlogs
import dweepy
import requests


BASE_NDIGITS = 6
CURRENT_NDIGITS = 4
ALT_NDIGITS = 6
ALT_MA_SIZE = 5

os.environ["COLOREDLOGS_LOG_FORMAT"] = "[%(levelname)s]\t%(asctime)s - %(message)s"
os.environ["COLOREDLOGS_DATE_FORMAT"] = "%H:%M:%S"

working_path = os.path.dirname(__file__)
if os.path.exists("/media/usb0"):
    log_path = os.path.join("/media/usb0", datetime.now().strftime("%Y%m%d") + ".log")
else:
    log_path = os.path.join(working_path, datetime.now().strftime("%Y%m%d") + ".log")

csv_path = os.path.join(working_path, datetime.now().strftime("%Y%m%d") + ".csv")

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


queue_alt_ma = []


def estimate_altitude(base_pressure, current_pressure, current_temperature):
    a = (((base_pressure / current_pressure) ** (1.0 / 5.275) - 1.0) * (current_temperature + 273.15)) / 0.0065
    queue_alt_ma.append(a)
    if ALT_MA_SIZE < len(queue_alt_ma):
        queue_alt_ma.pop(0)
    return sum(queue_alt_ma) / len(queue_alt_ma)


def get_location():
    try:
        response = requests.get("http://ipinfo.io/", timeout=1.0)
    except requests.exceptions.RequestException as e:
        logger.error(e)
        return (58.9667, 5.7500)  # -> Stavanger
    return response.json()["loc"].split(",")


last_update = datetime.now()
last_altitude = 0
idx = 0

fieldnames = ["timestamp", "sender", "rssi", "charge", "base_temperature", "base_pressure", "base_humidity", "current_temperature", "current_pressure", "current_humidity", "altitude", "speed", "latitude", "longitude"]

with serial.Serial("/dev/ttyUSB0", 19200) as ser:
    if os.path.isfile(csv_path) and os.stat(csv_path).st_size == 0:
        with open(csv_path, "w") as csv_file:
            writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
            writer.writeheader()

    lat, lng = get_location()
    logger.info("Lat / Lng: %s / %s", lat, lng)

    time.sleep(1)

    while ser.isOpen():
        with requests.Session() as session:
            rx_msg = ser.readline()
            m = pattern.match(rx_msg.decode('utf-8'))
            if m:
                now = datetime.now()
                delta_t = now - last_update
                logger.info("# %s (Δ: %s [s])", "{0:06d}".format(idx), delta_t.total_seconds())

                base_temperature, base_pressure, base_humidity = bme280.readBME280All()
                base_pressure = round(base_pressure, BASE_NDIGITS)
                base_temperature = round(base_temperature, BASE_NDIGITS)
                base_humidity = round(base_humidity, BASE_NDIGITS)
                logger.info("Base     : %shPa %sC %s%%", base_pressure, base_temperature, base_humidity)

                logger.info("Current  : %s", m.group())
                current_sender = m.group("sender")
                current_rssi = int(m.group("rssi"), 16)
                current_charge = int(m.group("charge"))
                current_pressure = round(float(m.group("pressure")), CURRENT_NDIGITS)
                current_temperature = round(float(m.group("temperature")), CURRENT_NDIGITS)
                current_humidity = round(float(m.group("humidity")), CURRENT_NDIGITS)
                current_altitude = round(estimate_altitude(base_pressure, current_pressure, current_temperature), ALT_NDIGITS)
                speed = round((current_altitude - last_altitude) / delta_t.total_seconds(), ALT_NDIGITS)
                logger.info("Altitude : %s[m] (%s [m/s])", current_altitude, speed)

                data = {"timestamp": now.isoformat(), "sender": current_sender, "rssi": current_rssi, "charge": current_charge,
                        "base_temperature": base_temperature, "base_pressure": base_pressure, "base_humidity": base_humidity,
                        "current_temperature": current_temperature, "current_pressure": current_pressure, "current_humidity": current_humidity,
                        "altitude": current_altitude, "speed": speed,
                        "latitude": lat, "longitude": lng}

                with open(csv_path, "a") as csv_file:
                    writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
                    writer.writerow(data)

                try:
                    dweepy.dweet_for("evoltalt", data, session=session)
                except dweepy.api.DweepyError as de:
                    logger.error(de)
                    time.sleep(1)

                last_update = now
                last_altitude = current_altitude
                idx += 1
