#include <Wire.h>
#include <RTClib.h>
#include <SD.h>
#include <SoftwareSerial.h>

RTC_PCF8523 rtc;

SoftwareSerial IM920(8, 9);  // RX, TX
int busy;  // IM920 BUSY status

void setup() {
  pinMode(10, INPUT);  // IM920 BUSY pin

  Serial.begin(19200);  // to PC

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC.");
    while (1);
  }
  if (! rtc.initialized()) {
    Serial.println("RTC isn't running.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  IM920.begin(19200);  // to IM920
  delay(1000);
  IM920.println("ECIO");  // set Character Mode
}

char recv[63];
int idx = 0;

void loop() {
  DateTime now = rtc.now();

  do {
    busy = digitalRead(10);
  } while (busy != 0);
  if (IM920.available()) {
    recv[idx] = IM920.read();
    if (63 < idx || recv[idx] == '\n') {
      recv[idx] = '\n';
      Serial.print(now.year(), DEC);
      Serial.print('/');
      Serial.print(now.month(), DEC);
      Serial.print('/');
      Serial.print(now.day(), DEC);
      Serial.print(" ");
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      Serial.print(now.minute(), DEC);
      Serial.print(':');
      Serial.print(now.second(), DEC);
      Serial.print(" -> ");
      Serial.println(recv);
      idx = 0;
    } else {
      idx++;
    }
  }
  // delay(50);
}
