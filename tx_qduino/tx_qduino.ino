#include <Wire.h>
#include <SoftwareSerial.h>
#include <Qduino.h>

#define BME280_ADDRESS 0x76
#define IM920_BUSY_PIN 4
#define TX_INTERVAL 2000

unsigned long int raw_humidity, raw_temperature, raw_pressure;
signed long int fine_temperature;

uint16_t dig_T1;
int16_t dig_T2;
int16_t dig_T3;
uint16_t dig_P1;
int16_t dig_P2;
int16_t dig_P3;
int16_t dig_P4;
int16_t dig_P5;
int16_t dig_P6;
int16_t dig_P7;
int16_t dig_P8;
int16_t dig_P9;
int8_t dig_H1;
int16_t dig_H2;
int8_t dig_H3;
int16_t dig_H4;
int16_t dig_H5;
int8_t dig_H6;

SoftwareSerial IM920(15, 16);  // RX, TX
int busy;  // IM920 BUSY status

qduino q;
fuelGauge battery;

void setup() {
        uint8_t oversampling_temperature = 1;    //Temperature oversampling x 1
        uint8_t oversampling_pressure = 1;       //Pressure oversampling x 1
        uint8_t oversampling_humidity = 1;       //Humidity oversampling x 1
        uint8_t mode = 3;         //Normal mode
        uint8_t time_to_standby = 5;         //Tstandby 1000ms
        uint8_t filter = 0;       //Filter off
        uint8_t spi3w_enable = 0;     //3-wire SPI Disable

        uint8_t control_measure_register = (oversampling_temperature << 5) | (oversampling_pressure << 2) | mode;
        uint8_t config_register    = (time_to_standby << 5) | (filter << 2) | spi3w_enable;
        uint8_t control_humidity_register  = oversampling_humidity;

        Serial.begin(19200); // to PC for debugging
        Wire.begin();

        q.setup();
        battery.setup();

        writeRegister(0xF2, control_humidity_register);
        writeRegister(0xF4, control_measure_register);
        writeRegister(0xF5, config_register);
        readTrim();

        pinMode(IM920_BUSY_PIN, INPUT); // IM920 BUSY pin
        IM920.begin(19200);
        delay(1000);
        IM920.println("ECIO"); // set Character Mode
}

void (* resetFunction) (void) = 0;  // reset function by code

void loop() {
        double actual_temperature = 0.0, actual_pressure = 0.0, actual_humidity = 0.0;
        signed long int calibrated_temperature;
        unsigned long int calibrated_pressure, calibrated_humidity;

        readData();

        calibrated_temperature = calibrationTemperature(raw_temperature);
        calibrated_pressure = calibrationPressure(raw_pressure);
        calibrated_humidity = calibrationHumidity(raw_humidity);
        actual_temperature = (double)calibrated_temperature / 100.0;
        actual_pressure = (double)calibrated_pressure / 100.0;
        actual_humidity = (double)calibrated_humidity / 1024.0;

        if (battery.inSleep()) {
                battery.wakeUp();
        }
        int charge = battery.chargePercentage();
        battery.goToSleep();

        Serial.print(charge);
        Serial.print("[%] ");
        Serial.print(actual_pressure);
        Serial.print("[hPa] ");
        Serial.print(actual_temperature);
        Serial.print("[C] ");
        Serial.print(actual_humidity);
        Serial.println("[%]");

        do {
                busy = digitalRead(IM920_BUSY_PIN);
        } while (busy != 0);
        IM920.print("TXDA ");
        IM920.print(charge);
        IM920.print("[%] ");
        IM920.print(actual_pressure);
        IM920.print("[hPa] ");
        IM920.print(actual_temperature);
        IM920.print("[C] ");
        IM920.print(actual_humidity);
        IM920.print("[%]\r\n");

        q.ledOff();
        delay(TX_INTERVAL);
        q.setRGB(GREEN);
}

void readTrim()
{
        uint8_t data[32], i = 0;             // Fix 2014/04/06
        Wire.beginTransmission(BME280_ADDRESS);
        Wire.write(0x88);
        Wire.endTransmission();
        Wire.requestFrom(BME280_ADDRESS, 24); // Fix 2014/04/06
        while (Wire.available()) {
                data[i] = Wire.read();
                i++;
        }

        Wire.beginTransmission(BME280_ADDRESS); // Add 2014/04/06
        Wire.write(0xA1);                    // Add 2014/04/06
        Wire.endTransmission();              // Add 2014/04/06
        Wire.requestFrom(BME280_ADDRESS, 1); // Add 2014/04/06
        data[i] = Wire.read();               // Add 2014/04/06
        i++;                                 // Add 2014/04/06

        Wire.beginTransmission(BME280_ADDRESS);
        Wire.write(0xE1);
        Wire.endTransmission();
        Wire.requestFrom(BME280_ADDRESS, 7); // Fix 2014/04/06
        while (Wire.available()) {
                data[i] = Wire.read();
                i++;
        }
        dig_T1 = (data[1] << 8) | data[0];
        dig_T2 = (data[3] << 8) | data[2];
        dig_T3 = (data[5] << 8) | data[4];
        dig_P1 = (data[7] << 8) | data[6];
        dig_P2 = (data[9] << 8) | data[8];
        dig_P3 = (data[11] << 8) | data[10];
        dig_P4 = (data[13] << 8) | data[12];
        dig_P5 = (data[15] << 8) | data[14];
        dig_P6 = (data[17] << 8) | data[16];
        dig_P7 = (data[19] << 8) | data[18];
        dig_P8 = (data[21] << 8) | data[20];
        dig_P9 = (data[23] << 8) | data[22];
        dig_H1 = data[24];
        dig_H2 = (data[26] << 8) | data[25];
        dig_H3 = data[27];
        dig_H4 = (data[28] << 4) | (0x0F & data[29]);
        dig_H5 = (data[30] << 4) | ((data[29] >> 4) & 0x0F); // Fix 2014/04/06
        dig_H6 = data[31];                             // Fix 2014/04/06
}
void writeRegister(uint8_t reg_address, uint8_t data)
{
        Wire.beginTransmission(BME280_ADDRESS);
        Wire.write(reg_address);
        Wire.write(data);
        Wire.endTransmission();
}


void readData()
{
        int i = 0;
        uint32_t data[8];
        Wire.beginTransmission(BME280_ADDRESS);
        Wire.write(0xF7);
        Wire.endTransmission();
        Wire.requestFrom(BME280_ADDRESS, 8);
        while (Wire.available()) {
                data[i] = Wire.read();
                i++;
        }
        raw_pressure = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
        raw_temperature = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
        raw_humidity  = (data[6] << 8) | data[7];
}


signed long int calibrationTemperature(signed long int adc_T)
{
        signed long int var1, var2, T;
        var1 = ((((adc_T >> 3) - ((signed long int)dig_T1 << 1))) * ((signed long int)dig_T2)) >> 11;
        var2 = (((((adc_T >> 4) - ((signed long int)dig_T1)) * ((adc_T >> 4) - ((signed long int)dig_T1))) >> 12) * ((signed long int)dig_T3)) >> 14;

        fine_temperature = var1 + var2;
        T = (fine_temperature * 5 + 128) >> 8;
        return T;
}

unsigned long int calibrationPressure(signed long int adc_P)
{
        signed long int var1, var2;
        unsigned long int P;
        var1 = (((signed long int)fine_temperature) >> 1) - (signed long int)64000;
        var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((signed long int)dig_P6);
        var2 = var2 + ((var1 * ((signed long int)dig_P5)) << 1);
        var2 = (var2 >> 2) + (((signed long int)dig_P4) << 16);
        var1 = (((dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((signed long int)dig_P2) * var1) >> 1)) >> 18;
        var1 = ((((32768 + var1)) * ((signed long int)dig_P1)) >> 15);
        if (var1 == 0)
        {
                return 0;
        }
        P = (((unsigned long int)(((signed long int)1048576) - adc_P) - (var2 >> 12))) * 3125;
        if (P < 0x80000000)
        {
                P = (P << 1) / ((unsigned long int) var1);
        }
        else
        {
                P = (P / (unsigned long int)var1) * 2;
        }
        var1 = (((signed long int)dig_P9) * ((signed long int)(((P >> 3) * (P >> 3)) >> 13))) >> 12;
        var2 = (((signed long int)(P >> 2)) * ((signed long int)dig_P8)) >> 13;
        P = (unsigned long int)((signed long int)P + ((var1 + var2 + dig_P7) >> 4));
        return P;
}

unsigned long int calibrationHumidity(signed long int adc_H)
{
        signed long int v_x1;

        v_x1 = (fine_temperature - ((signed long int)76800));
        v_x1 = (((((adc_H << 14) - (((signed long int)dig_H4) << 20) - (((signed long int)dig_H5) * v_x1)) +
                  ((signed long int)16384)) >> 15) * (((((((v_x1 * ((signed long int)dig_H6)) >> 10) *
                                                          (((v_x1 * ((signed long int)dig_H3)) >> 11) + ((signed long int) 32768))) >> 10) + (( signed long int)2097152)) *
                                                       ((signed long int) dig_H2) + 8192) >> 14));
        v_x1 = (v_x1 - (((((v_x1 >> 15) * (v_x1 >> 15)) >> 7) * ((signed long int)dig_H1)) >> 4));
        v_x1 = (v_x1 < 0 ? 0 : v_x1);
        v_x1 = (v_x1 > 419430400 ? 419430400 : v_x1);
        return (unsigned long int)(v_x1 >> 12);
}
