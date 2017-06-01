#include <SoftwareSerial.h>

SoftwareSerial IM920(8, 9);  // RX, TX
int busy;  // IM920 BUSY status

void setup() {
  pinMode(10, INPUT);  // IM920 BUSY pin
  Serial.begin(19200);  // to PC
  IM920.begin(19200);  // to IM920
  delay(1000);
  IM920.println("ECIO");  // set Character Mode
}

char input[63];
int idx = 0;

void loop() {
  do {
    busy = digitalRead(10);
  } while (busy != 0);
  if (IM920.available()) {
    input[idx] = IM920.read();
    if (63 < idx || input[idx] == '\n') {
      input[idx] = '\n';
      Serial.println(input);
      idx = 0;
    } else {
      idx++;
    }
  }
  // delay(50);
}

