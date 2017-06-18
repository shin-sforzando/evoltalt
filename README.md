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
SD card works well with FAT32 format using [SD Formatter](https://www.sdcard.org/downloads/formatter_4/).
Original SD chip select pin is 10, but it's conflicting with IM920 busy pin.
New soldered chip select pin is **9**.

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

## TODO
### HIGH
- [ ] TX: miniaturize
- [ ] TX: measure consumption power
- [ ] RX: sanitize to serial
- [ ] RX: capture to txt
- [ ] RX: implement BME280

### MID
- [ ] RX: capture to SD card
- [ ] TX/RX: make repeater

### LOW
- [ ] TX: casing
- [ ] RX: prepare graph & server
