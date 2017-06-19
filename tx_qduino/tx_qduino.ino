#include <Wire.h>
#include <SoftwareSerial.h>
#include <Qduino.h>

#define BME280_ADDRESS 0x76
#define IM920_BUSY_PIN 4
#define TX_INTERVAL 2000

unsigned long int raw_humidity, raw_temperature, raw_pressure;
signed long int fine_temperature;

uint16_t digit_temperature1;
int16_t digit_temperature2;
int16_t digit_temperature3;
uint16_t digit_pressure1;
int16_t digit_pressure2;
int16_t digit_pressure3;
int16_t digit_pressure4;
int16_t digit_pressure5;
int16_t digit_pressure6;
int16_t digit_pressure7;
int16_t digit_pressure8;
int16_t digit_pressure9;
int8_t digit_humidity1;
int16_t digit_humidity2;
int8_t digit_humidity3;
int16_t digit_humidity4;
int16_t digit_humidity5;
int8_t digit_humidity6;

SoftwareSerial IM920(15, 16);  // RX, TX
int im920_busy;  // IM920 BUSY status

qduino q;
fuelGauge battery;

void setup() {
        // for bme280's register map
        uint8_t spi3w_enable = 0;  // 3-wire SPI: disable
        uint8_t mode = 3;  // Mode: normal (00: sleep, 01 or 10: force, 11: normal)
        uint8_t time_to_standby = 5;  // Time to standby: 1000ms (000: 0.5, 001: 62.5, 010: 125, 011: 250, 100: 500, 101: 1000, 110: 10, 111: 20)
        uint8_t oversampling_temperature = 5; // temperature oversampling: x16 (000: skipped, 001: x1, 010: x2, 011: x4, 100: x8, 101 and others: x16)
        uint8_t oversampling_pressure = 4; // pressure oversampling: x8
        uint8_t oversampling_humidity = 2; // humidity oversampling: x2
        uint8_t filter = 4;  // Filter: 16 (000: off, 001: 2, 010: 4, 011: 8, 100 and others: 16)

        uint8_t control_humidity_register  = oversampling_humidity;
        uint8_t control_measure_register = (oversampling_temperature << 5) | (oversampling_pressure << 2) | mode;
        uint8_t config_register    = (time_to_standby << 5) | (filter << 2) | spi3w_enable;

        Serial.begin(19200); // to PC for debugging

        Wire.begin();

        q.setup();
        battery.setup();

        writeRegister(0xF2, control_humidity_register);
        writeRegister(0xF4, control_measure_register);
        writeRegister(0xF5, config_register);
        readTrim();

        pinMode(IM920_BUSY_PIN, INPUT);  // IM920 BUSY pin
        IM920.begin(19200);
        delay(1000);
        IM920.println("ECIO");  // set Character Mode
}

void (* resetQduino) (void) = 0;  // reset function by code

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

        // check irregular data
        if (charge <= 0 ||
            actual_pressure <= 700 || 1083.8 <= actual_pressure ||
            actual_temperature <= -40 || 58.8 <= actual_temperature ||
            actual_humidity <= 0 || 100 <= actual_humidity) {
                resetQduino();
        }

        Serial.print(charge);
        Serial.print("% ");
        Serial.print(actual_pressure);
        Serial.print("hPa ");
        Serial.print(actual_temperature);
        Serial.print("C ");
        Serial.print(actual_humidity);
        Serial.println("%");

        do {
                im920_busy = digitalRead(IM920_BUSY_PIN);
        } while (im920_busy != 0);
        IM920.print("TXDA ");
        IM920.print(charge);
        IM920.print("% ");
        IM920.print(actual_pressure);
        IM920.print("hPa ");
        IM920.print(actual_temperature);
        IM920.print("C ");
        IM920.print(actual_humidity);
        IM920.print("%\r\n");

        q.ledOff();
        delay(TX_INTERVAL);
        q.setRGB(GREEN);
}

void readTrim()
{
        uint8_t data[32], i = 0;
        Wire.beginTransmission(BME280_ADDRESS);
        Wire.write(0x88);
        Wire.endTransmission();
        Wire.requestFrom(BME280_ADDRESS, 24);
        while (Wire.available()) {
                data[i] = Wire.read();
                i++;
        }

        Wire.beginTransmission(BME280_ADDRESS);
        Wire.write(0xA1);
        Wire.endTransmission();
        Wire.requestFrom(BME280_ADDRESS, 1);
        data[i] = Wire.read();
        i++;

        Wire.beginTransmission(BME280_ADDRESS);
        Wire.write(0xE1);
        Wire.endTransmission();
        Wire.requestFrom(BME280_ADDRESS, 7);
        while (Wire.available()) {
                data[i] = Wire.read();
                i++;
        }
        digit_temperature1 = (data[1] << 8) | data[0];
        digit_temperature2 = (data[3] << 8) | data[2];
        digit_temperature3 = (data[5] << 8) | data[4];
        digit_pressure1 = (data[7] << 8) | data[6];
        digit_pressure2 = (data[9] << 8) | data[8];
        digit_pressure3 = (data[11] << 8) | data[10];
        digit_pressure4 = (data[13] << 8) | data[12];
        digit_pressure5 = (data[15] << 8) | data[14];
        digit_pressure6 = (data[17] << 8) | data[16];
        digit_pressure7 = (data[19] << 8) | data[18];
        digit_pressure8 = (data[21] << 8) | data[20];
        digit_pressure9 = (data[23] << 8) | data[22];
        digit_humidity1 = data[24];
        digit_humidity2 = (data[26] << 8) | data[25];
        digit_humidity3 = data[27];
        digit_humidity4 = (data[28] << 4) | (0x0F & data[29]);
        digit_humidity5 = (data[30] << 4) | ((data[29] >> 4) & 0x0F);
        digit_humidity6 = data[31];
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
        var1 = ((((adc_T >> 3) - ((signed long int)digit_temperature1 << 1))) * ((signed long int)digit_temperature2)) >> 11;
        var2 = (((((adc_T >> 4) - ((signed long int)digit_temperature1)) * ((adc_T >> 4) - ((signed long int)digit_temperature1))) >> 12) * ((signed long int)digit_temperature3)) >> 14;

        fine_temperature = var1 + var2;
        T = (fine_temperature * 5 + 128) >> 8;
        return T;
}

unsigned long int calibrationPressure(signed long int adc_P)
{
        signed long int var1, var2;
        unsigned long int P;
        var1 = (((signed long int)fine_temperature) >> 1) - (signed long int)64000;
        var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((signed long int)digit_pressure6);
        var2 = var2 + ((var1 * ((signed long int)digit_pressure5)) << 1);
        var2 = (var2 >> 2) + (((signed long int)digit_pressure4) << 16);
        var1 = (((digit_pressure3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((signed long int)digit_pressure2) * var1) >> 1)) >> 18;
        var1 = ((((32768 + var1)) * ((signed long int)digit_pressure1)) >> 15);
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
        var1 = (((signed long int)digit_pressure9) * ((signed long int)(((P >> 3) * (P >> 3)) >> 13))) >> 12;
        var2 = (((signed long int)(P >> 2)) * ((signed long int)digit_pressure8)) >> 13;
        P = (unsigned long int)((signed long int)P + ((var1 + var2 + digit_pressure7) >> 4));
        return P;
}

unsigned long int calibrationHumidity(signed long int adc_H)
{
        signed long int v_x1;

        v_x1 = (fine_temperature - ((signed long int)76800));
        v_x1 = (((((adc_H << 14) - (((signed long int)digit_humidity4) << 20) - (((signed long int)digit_humidity5) * v_x1)) + ((signed long int)16384)) >> 15) * (((((((v_x1 * ((signed long int)digit_humidity6)) >> 10) * (((v_x1 * ((signed long int)digit_humidity3)) >> 11) + ((signed long int) 32768))) >> 10) + (( signed long int)2097152)) * ((signed long int) digit_humidity2) + 8192) >> 14));
        v_x1 = (v_x1 - (((((v_x1 >> 15) * (v_x1 >> 15)) >> 7) * ((signed long int)digit_humidity1)) >> 4));
        v_x1 = (v_x1 < 0 ? 0 : v_x1);
        v_x1 = (v_x1 > 419430400 ? 419430400 : v_x1);
        return (unsigned long int)(v_x1 >> 12);
}
