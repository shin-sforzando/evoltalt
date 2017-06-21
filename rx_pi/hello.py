#!/usr/bin/env python
# -*- coding: utf-8 -*-

import logging
import coloredlogs
import sys
import time
import pprint
import uuid

# import ibmiotf.application
# import ibmiotf.device

logger = logging.getLogger(__name__)
coloredlogs.install(level="DEBUG")
logger.info("test")

# try:
#     options = ibmiotf.device.ParseConfigFile("./config.cfg")
#     client = ibmiotf.device.Client(options)
# except ibmiotf.ConnectionException as e:
#     print(str(e))
#     sys.exit()
#
# client.connect()
#
# client.publishEvent("hello", "json", "hello")
#
# client.disconnect()
