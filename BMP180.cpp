/**

BMP180.cpp, this implements the origial integer arithetic from the data sheet
   floating point version is preferred. It shows less rounding artefacts and noise.

Copyright by J. Schwender, 2018

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

 */
#include "Arduino.h"
#include "BMP180.h"
#include <Wire.h>

// nice
typedef uint8_t byte;

/**
 * Init the Chip.
 * Starts I2C communication, read chip's ID and calibration data.
 */
void BMP180::init() {
	Wire.begin();
	setSamplingMode(BMP180_OVERSAMPLING_STANDARD);
	_ID = readID();
	readCalibrationData();
}

/**
 * Read the ID of the chip.
 */
byte BMP180::readID() {
	return readByteFromRegister(BMP180_CHIP_ID_REGISTER);
}

/**
 * Get the ID of the chip.
 */
byte BMP180::getID() {
	return _ID;
}

/**
 * Check if the ID is valid.
 * ID must be 0x55.
 */
bool BMP180::hasValidID() {
	return getID() == BMP180_CHIP_ID;
}

/**
 * Read calibration data.
 */
void BMP180::readCalibrationData() {
	Cal_AC1 = readIntFromRegister(BMP180_CALIBRATION_DATA_AC1);
	Cal_AC2 = readIntFromRegister(BMP180_CALIBRATION_DATA_AC2);
	Cal_AC3 = readIntFromRegister(BMP180_CALIBRATION_DATA_AC3);
	Cal_AC4 = readUIntFromRegister(BMP180_CALIBRATION_DATA_AC4);
	Cal_AC5 = readUIntFromRegister(BMP180_CALIBRATION_DATA_AC5);
	Cal_AC6 = readUIntFromRegister(BMP180_CALIBRATION_DATA_AC6);
	Cal_B1  = readIntFromRegister(BMP180_CALIBRATION_DATA_B1);
	Cal_B2  = readIntFromRegister(BMP180_CALIBRATION_DATA_B2);
	Cal_MB  = readIntFromRegister(BMP180_CALIBRATION_DATA_MB);
	Cal_MC  = readIntFromRegister(BMP180_CALIBRATION_DATA_MC);
	Cal_MD  = readIntFromRegister(BMP180_CALIBRATION_DATA_MD);
}

/**
 * Make a soft reset.
 */
void BMP180::reset() {
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.write(BMP180_SOFT_RESET_REGISTER);
	Wire.write(BMP180_SOFT_RESET);
	Wire.endTransmission();
}

/**
 * Select register for reading operation.
 */
void BMP180::selectRegister(byte reg) {
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.write(reg);
	Wire.endTransmission();
}

/**
 * Read a byte from a register.
 */
byte BMP180::readByteFromRegister(byte reg) {
	byte b;
	selectRegister(reg);
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.requestFrom(BMP180_I2C_ADDRESS, 1);
	b = Wire.read();
	Wire.endTransmission();
	return b;
}

/**
 * Read an signed integer from a register.
 */
signed int BMP180::readIntFromRegister(byte reg) {
	signed int i;
	selectRegister(reg);
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.requestFrom(BMP180_I2C_ADDRESS, 2);
	i = (signed int)Wire.read();
	i = i << 8 | (signed int)Wire.read();
	Wire.endTransmission();
	return i;
}

/**
 * Read an integer from a register.
 */
unsigned int BMP180::readUIntFromRegister(byte reg) {
	unsigned int i;
	selectRegister(reg);
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.requestFrom(BMP180_I2C_ADDRESS, 2);
	i = (signed int)Wire.read();
	i = i << 8 | (signed int)Wire.read();
	Wire.endTransmission();
	return i;
}

/**
 * Read a long from a register.
 * Int true, it's not a long - it's a 19bit value within three bytes.
 */
unsigned long BMP180::readLongFromRegister(byte reg) {
	unsigned long l;
	selectRegister(reg);
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.requestFrom(BMP180_I2C_ADDRESS, 3);
	l = Wire.read();
	l = l << 8 | Wire.read();
	l = l << 8 | Wire.read();
	Wire.endTransmission();
	return l;
}

/**
 * Starts a measure.
 */
void BMP180::measure(byte measureID) {
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.write(BMP180_CONTROL_REGISTER);
	Wire.write(measureID);
	Wire.endTransmission();
}

/**
 * Starts the measure of the uncompensated temperature.
 * The measured value must be compensated by the calibration data.
 */
long BMP180::measureTemperature() {
	measure(BMP180_MEASURE_TEMPERATURE);
	delay(5);
	return (long)readUIntFromRegister(BMP180_MEASURE_VALUE_MSB);
}

/**
 * Starts the measure of the uncompensated pressure.
 * The measured value must be compensated by the calibration data.
 */
long BMP180::measurePressure(byte oversampling) {
	measure(BMP180_MEASURE_PRESSURE | (oversampling << 6));
	switch (oversampling) {
		case 0: delay(5); break;
		case 1: delay(8); break;
		case 2: delay(14); break;
		case 3: delay(26); break;
	}
	long p = (long)readLongFromRegister(BMP180_MEASURE_VALUE_MSB);
	p = p >> (8 - oversampling);
	return p;
}

/**
 * Compensate the measured temperature with the calibration data.
 */
long BMP180::compensateTemperature(long UT) {
	long X1 = ((UT - (long)Cal_AC6) * (long)Cal_AC5) >> 15;
	long X2 = ((long)Cal_MC << 11) / (X1 + (long)Cal_MD);
	CalTemp_B5 = X1 + X2;   /* wird zur Druckkorrektur verwendet */
	return (CalTemp_B5 + 8) >> 4;  /* ÷2⁴ */
}

/**
 * Compensate the measured pressure with the calibration data.
 * The temperature must be measured and compensated before this operation - need valid CalTemp_B5.
*/
long BMP180::compensatePressure(long UP, int oversampling) {
	long B6, X1, X2, X3, B3, p;
	unsigned long B4, B7;
	B6 = CalTemp_B5 - 4000;
	X1 = ((long)Cal_B2 * ((B6 * B6) >> 12)) >> 11;
	X2 = ((long)Cal_AC2 * B6) >> 11;
	X3 = X1 + X2;
	B3 = ((((long)Cal_AC1 * 4 + X3) << oversampling) + 2) >> 2;
	X1 = ((long)Cal_AC3 * B6) >> 13;
	X2 = ((long)Cal_B1 * ((B6 * B6) >> 12)) >> 16;
	X3 = ((X1 + X2) + 2) >> 2;
	B4 = (Cal_AC4 * ((unsigned long)(X3 + 32768))) >> 15;
	B7 = ((unsigned long)UP - B3) * (50000 >> oversampling);
	if (B7 < 0x80000000)
		p = (B7 * 2) / B4;
	else
		p = (B7 / B4) * 2;
	X1 = (p >> 8) * (p >> 8);
	X1 = (X1 * 3038) >> 16;
	X2 = (-7357 * p) >> 16;
	p = p + ((X1 + X2 + 3791) >> 4);
	return p;
}

/**
 * Set the sampling mode.
 */
void BMP180::setSamplingMode(byte samplingMode) {
	_samplingMode = samplingMode;
}

float BMP180::getAltitude() {
	float altitude = 44330.0 * (1.0 - pow( (P/InitialPressure),0.1902949572) );
	return altitude;
}

void BMP180::getData() {   // liest zuerst die Temperatur und dann den Druck
	long ut = measureTemperature();            // Temperaturmessung unmittelbar vor der Druckmessung
	long up = measurePressure(_samplingMode);   // scheint wichtig zu sein genau diese Reihenfolge einzuhalten
	long t = compensateTemperature(ut);          // sonst bekommt man wesendlich stärkere Schwankungen!
	long p = compensatePressure(up, _samplingMode);
	T = (float)t/10.0;  // belegt die globalen Variablen mit den kompensierten Werten
	P = (float)p/100.0;
}

void BMP180::setP0() {
    getData();
    InitialPressure = P;  // Referenzwert der Höhenmessung, beim Einschalten einmal aufrufen oder bei Bedarf.
}
