/**

BMP180FP.cpp, floating point version.

Copyright by J. Schwender 2018

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
#include "BMP180FP.h"
#include <Wire.h>

/**
 * Init the Chip.
 * Starts I2C communication, read chip's ID and calibration data.
 */
void BMP180FP::init() {
	Wire.begin();
	setSamplingMode(BMP180_OVERSAMPLING_STANDARD);
	_ID = readID();
	readCalibrationData();
}

/**
 * Read the ID of the chip.
 */
byte BMP180FP::readID() {
	return readByteFromRegister(BMP180_CHIP_ID_REGISTER);
}

/**
 * Get the ID of the chip.
 */
byte BMP180FP::getID() {
	return _ID;
}

/**
 * Check if the ID is valid.
 * ID must be 0x55.
 */
bool BMP180FP::hasValidID() {
	return getID() == BMP180_CHIP_ID;
}

/**
 * Read calibration data.
 */
void BMP180FP::readCalibrationData() {
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
	// Compute floating-point polynominals:

	c3 = 160.0 * pow(2,-15) * (double)Cal_AC3;         //c3 = 4.8828125e-3 * (double)Cal_AC3;
	c4 = pow(10,-3) * pow(2,-15) * (double)Cal_AC4;    //c4 = 3.051757813e-8 * (double)Cal_AC4;
	b1 = pow(160,2) * pow(2,-30) * (double)Cal_B1;     //b1 = 2.384185791E-5 * (double)Cal_B1;
	c5 = (pow(2,-15) / 160) * (double)Cal_AC5;         //c5 = 1.907348633E-7 * (double)Cal_AC5;
	c6 = (double)Cal_AC6;
	mc = (pow(2,11) / pow(160,2)) * (double)Cal_MC;    //mc = 0.08 * (double)Cal_MC;
	md = (double)Cal_MD / 160.0;
	x0 = (double)Cal_AC1;
	x1 = 160.0 * pow(2,-13) * (double)Cal_AC2;         //x1 = 0.01953125 * (double)Cal_AC2;
	x2 = pow(160,2) * pow(2,-25) * (double)Cal_B2;     //x2 = 7.629394531E-4 * (double)Cal_B2;
	y0 = c4 * pow(2,15);                               //y0 = 32768.0 * c4;
	y1 = c4 * c3;
	y2 = c4 * b1;
	p0 = (3791.0 - 8.0) / 1600.0;                      //p0 = 2.364375;
	p1 = 1.0 - (7357.0 * pow(2,-20));                  //p1 = 0.9929838181;
	p2 = 3038.0 * 100.0 * pow(2,-36);                  //p2 = 4.420871846E-6;
}

/**
 * Make a soft reset.
 */
void BMP180FP::reset() {
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.write(BMP180_SOFT_RESET_REGISTER);
	Wire.write(BMP180_SOFT_RESET);
	Wire.endTransmission();
}

/**
 * Select register for reading operation.
 */
void BMP180FP::selectRegister(unsigned char reg) {
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.write(reg);
	Wire.endTransmission();
}

/**
 * Read a byte from a register.
 */
unsigned char BMP180FP::readByteFromRegister(unsigned char reg) {
	unsigned char b;
	selectRegister(reg);
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.requestFrom(BMP180_I2C_ADDRESS, 1);
	b = (unsigned char)Wire.read();
	Wire.endTransmission();
	return b;
}

/**
 * Read an signed integer from a register.
 */
signed int BMP180FP::readIntFromRegister(unsigned char reg) {
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
 * Read an unsigned integer from a register.
 */
unsigned int BMP180FP::readUIntFromRegister(unsigned char reg) {
	unsigned int i;
	selectRegister(reg);
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.requestFrom(BMP180_I2C_ADDRESS, 2);
	i = (unsigned int)Wire.read();
	i = i << 8 | (unsigned int)Wire.read();
	Wire.endTransmission();
	return i;
}

/**
 * Read a long from a register.
 * Int true, it's not a long - it's a 19bit value within three bytes.
 */
unsigned long BMP180FP::readLongFromRegister(unsigned char reg) {
	unsigned long l;
	selectRegister(reg);
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.requestFrom(BMP180_I2C_ADDRESS, 3);
	l = (unsigned long)Wire.read();
	l = l << 8 | (unsigned long)Wire.read();
	l = l << 8 | (unsigned long)Wire.read();
	Wire.endTransmission();
	return l;
}

/**
 * Starts a measure.
 */
void BMP180FP::measure(byte measureID) {
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.write(BMP180_CONTROL_REGISTER);
	Wire.write(measureID);
	Wire.endTransmission();
}

/**
 * Starts the measure of the uncompensated temperature.
 * The measured value must be compensated by the calibration data.
 */
long BMP180FP::measureTemperature() {
	measure(BMP180_MEASURE_TEMPERATURE);
	delay(5);
	return (long)readUIntFromRegister(BMP180_MEASURE_VALUE_MSB);
}

/**
 * Starts the measure of the uncompensated pressure.
 * The measured value must be compensated by the calibration data.
 */
long BMP180FP::measurePressure(unsigned char oversampling) {
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

void BMP180FP::compensateTemperature(long UT) {     // same with floating point
	double alpha =  c5 * ((double)UT - c6);
	T = alpha + mc / (alpha + md);  // setting global variable here
}

void BMP180FP::compensatePressure(long UncompPressure, int oversampling) {
	double s,x,y,z,sq;
	s = T - 25.0;
	sq = pow(s,2);
	x = (x2 * sq) + (x1 * s) + x0;
	y = (y2 * sq) + (y1 * s) + y0;
	z = ((double)UncompPressure/(double)(1<<oversampling) - x) / y;
	P = (p2 * pow(z,2)) + (p1 * z) + p0;  // setting global variable here
}

/**
 * Set the sampling mode.
 */
void BMP180FP::setSamplingMode(unsigned char samplingMode) {
	_samplingMode = samplingMode;
}

double BMP180FP::getAltitude() {   // simple ISO standard implementation
	double altitude = 44330.0 * (1.0 - pow( (P/InitialPressure),0.1902949572) );
	return altitude;
}

void BMP180FP::getData() {   // first reads the temperature then immediately the pressure
	double ut = measureTemperature();            // it seems important to follow that sequence like
	long up = measurePressure(_samplingMode);   //  it is given in the data sheet.
	compensateTemperature(ut);          // otherwise the noise is larger.
	compensatePressure(up, _samplingMode);
}

void BMP180FP::setP0() {
    getData();
    InitialPressure = P;  // this is used as a reference pressure on power on/reset. Call it only once in setup().
}             // May be used to display a pressure/altitude change since reset.
