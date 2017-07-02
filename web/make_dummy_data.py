#!/usr/bin/env python
# -*- coding: utf-8 -*-

import csv
import datetime
import random

fieldnames = ["timestamp", "sender", "rssi", "charge",
              "base_temperature", "base_pressure", "base_humidity",
              "current_temperature", "current_pressure", "current_humidity",
              "altitude", "speed",
              "latitude", "longitude"]


def get_dummy_date():
    d = datetime.timedelta(seconds=2.5)
    t = datetime.datetime.now()
    while True:
        yield t
        t += d


def main():
    with open("dummy.csv", "w") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()

        for i in range(100):
            data = {"timestamp": get_dummy_date().__next__(), "sender": "DUMM", "rssi": random.randrange(128, 255), "charge": random.randrange(1, 101),
                    "base_temperature": base_temperature, "base_pressure": base_pressure, "base_humidity": base_humidity,
                    "current_temperature": current_temperature, "current_pressure": current_pressure, "current_humidity": current_humidity,
                    "altitude": current_altitude, "speed": speed,
                    "latitude": lat, "longitude": lng}


if __name__ == '__main__':
    main()
