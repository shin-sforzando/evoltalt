evoltalt
====

<!-- TOC depthFrom:2 depthTo:6 withLinks:1 updateOnSave:1 orderedList:0 -->

- [TX](#tx)
	- [Qduino mini](#qduino-mini)
- [RX](#rx)
	- [Arduino Leonard](#arduino-leonard)
		- [IM315SHLDv2](#im315shldv2)
		- [Adafruit Data Logger Shield rev B](#adafruit-data-logger-shield-rev-b)
			- [Pin Assign](#pin-assign)

<!-- /TOC -->

## TX
### Qduino mini

## RX
### Arduino Leonard
#### IM315SHLDv2
#### Adafruit Data Logger Shield rev B
SD card works well with FAT32 format.
RTC chip is PCF8523.
Battery for RTC is CR1220.
It needs [RTClib](https://github.com/adafruit/RTClib).

##### Pin Assign
* ICSP SCK - SPI clock
* ICSP MISO - SPI MISO
* ICSP MOSI - SPI MOSI
* Digital #10 - SD Card chip select (can cut a trace to re-assign)
* SDA not connected to A4
* SCL not connected to A5
