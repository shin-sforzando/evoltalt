#include <Wire.h>
#include <RTClib.h>
#include <SoftwareSerial.h>

RTC_PCF8523 rtc;

SoftwareSerial IM920(8, 9);  // RX, TX
int busy;  // IM920 BUSY status

void setup() {
  pinMode(10, INPUT);  // IM920 BUSY pin

  Serial.begin(19200);  // to PC
  while (!Serial);

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
  char dateBuffer[12];

  do {
    busy = digitalRead(10);
  } while (busy != 0);
  if (IM920.available()) {
    recv[idx] = IM920.read();
    if (63 < idx || recv[idx] == '\n') {
      recv[idx] = '\n';

      sprintf(dateBuffer, "%04u/%02u/%02u ", now.year(), now.month(), now.day());
      Serial.print(dateBuffer);
      sprintf(dateBuffer, "%02u:%02u:%02u ", now.hour(), now.minute(), now.second());
      Serial.print(dateBuffer);
      Serial.print(" -> ");
      Serial.println(recv);
      idx = 0;
    } else {
      idx++;
    }
  }
  // delay(50);
}
