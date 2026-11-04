#pragma once

///////////////////////////////////////////////////////////////////////////////
// DFRobot B_LUX_V30B Ambient Light Sensor Module (0..200'000 LUX)
// SKU: SEN0390
// Copyright (C) https://muman.ch + https://github.com/mumanchu, 2026.11.04
// Copyright (c) 2010 DFRobot Co.Ltd, MIT License
// If you re-use this software, please include the above copyright notices
/*
For details see the github repository
https://github.com/V30BAmbientLightSensor

DFRobot Wiki Page
https://wiki.dfrobot.com/sen0390/

Data Sheet
https://dfimg.dfrobot.com/wiki/18648/SEN0390_b-lux-v30b-ambient-light-sensor_datasheet_V1.0.pdf

Official DFRobot Library
https://github.com/DFRobot/DFRobot_B_LUX_V30B
*/

typedef unsigned int uint;
typedef unsigned long ulong;

#ifndef LOGERROR
#ifdef DEBUG
#define LOGERROR(s) { Serial.println(s); Serial.flush(); }
#else
#define LOGERROR(s)
#endif
#endif


class V30BAmbientLightSensor
{
public:
	#ifdef V30B_USE_HARDWARE_I2C
	// WARNING! HARDWARE I2C IS NOT RELIABLE!
	bool begin(TwoWire* twoWire);
	#else
	bool begin(uint sdaPin, uint sclPin);
	#endif

	bool setAutoMode();
	bool setManualMode(uint integrationTime, bool highBrightness);
	bool readLux(long* lux);
	bool readLux(float* lux);
	bool readEEPROM(uint adds, byte* data, uint length = 1);
	bool writeEEPROM(uint adds, const byte* data, uint length);
	bool writeEEPROM(uint adds, byte data) { return writeEEPROM(adds, &data, 1); }

protected:
	const byte i2cAdds = 0x4a;	// fixed I2C address

	#ifdef V30B_USE_HARDWARE_I2C
	TwoWire* wire;
	#else
	// Software I2C
	const uint usDelay = 5;		// the DFRobot code use 5uS, but works ok with 0
	uint sda, scl;
	void i2cSendStartStop(bool start);
	void i2cSendAck(bool ack);
	bool i2cRecvAck();
	bool i2cSendByte(byte data);
	byte i2cRecvByte();
	#endif

	bool i2cReadReg(byte reg, byte* data) { return i2cReadRegs(reg, data, 1); }
	bool i2cWriteReg(uint reg, byte data) { return i2cWriteRegs(reg, &data, 1); }
	bool i2cReadRegs(byte reg, byte* data, uint length);
	bool i2cWriteRegs(uint reg, const byte* data, uint length);
};


// Initialize
#ifdef V30B_USE_HARDWARE_I2C

// Hardware I2C, max. 100000Hz
bool V30BAmbientLightSensor::begin(TwoWire* twoWire)
{
	wire = twoWire;
	return setAutoMode();
}
#else 

// Software I2C
// do not use the hardware I2C pins, else hardware I2C will not work
bool V30BAmbientLightSensor::begin(uint sdaPin, uint sclPin)
{
	sda = sdaPin;
	scl = sclPin;
	pinMode(sda, OUTPUT);
	digitalWrite(sda, 1);
	pinMode(scl, OUTPUT);
	digitalWrite(scl, 1);
	return setAutoMode();
}
#endif

// Auto Mode
bool V30BAmbientLightSensor::setAutoMode()
{
	if (!i2cWriteReg(0x04, 0x00))
		return false;
	delay(500);
	return true;
}

// Manual Mode
// integrationTime (0..7) : 0=800mS (low light), 1=400mS, 2=200mS, 3=100mS (high brightness), 
//                          4=50mS, 5=25mS, 6=12.5mS, 7=6.25mS
// highBrightness         : 0=normal, 1=high brightness divide-by-8
bool V30BAmbientLightSensor::setManualMode(uint integrationTime, bool highBrightness)
{
	if (integrationTime > 7) {
		LOGERROR("bad param");
		return false;
	}
	byte b = 0x40 | integrationTime;
	if (highBrightness)
		b |= 0x08;
	if (!i2cWriteReg(0x04, b))
		return false;
	delay(500);
	return true;
}

// Integer version, min. 0 LUX
bool V30BAmbientLightSensor::readLux(long* lux)
{
	byte data[4];
	if (!i2cReadRegs(0x00, data, 4)) {
		*lux = -1;
		return false;
	}
	long v = *(long*)data;
	if (v < 0)
		v = 0;
	// (v * 1.4f) / 1000.0f;
	// + 5000 is for rounding to nearest integer
	*lux = ((v * 14) + 5000) / 10000;
	return true;
}

// Float version, if you need < 1.0 LUX
bool V30BAmbientLightSensor::readLux(float* lux)
{
	byte data[4];
	if (!i2cReadRegs(0x00, data, 4)) {
		*lux = -1.0f;
		return false;
	}
	long v = *(long*)data;
	if (v < 0)
		v = 0;
	*lux = (v * 1.4f) / 1000.0f;
	return true;
}

// 112 bytes of EEPROM are available for user data (I think)
bool V30BAmbientLightSensor::readEEPROM(uint adds, byte* data, uint length /*=1*/)
{
	if ((adds + length) > 112) {
		LOGERROR("bad param");
		return false;
	}
	return i2cReadRegs(0x10 + adds, data, length);
}

// NOTE: EEPROMs have a 'limited' number of write cycles, typically 100'000 to 
// 1'000'000, so don't write code that sits in a loop writing to EEPROM
bool V30BAmbientLightSensor::writeEEPROM(uint adds, const byte* data, uint length)
{
	if ((adds + length) > 112) {
		LOGERROR("bad param");
		return false;
	}
	bool ok = i2cWriteRegs(0x10 + adds, data, length);
	delay(100);
	return ok;
}


// HARDWARE I2C DID NOT WORK FOR ME
#ifdef V30B_USE_HARDWARE_I2C

///////////////////////////////////////////////////////////////////////////////
// Hardware I2C, runs at 100KHz, not reliable - DO NOT USE
// there is some obscure timing problem

// Read one or more registers
// returns : true = success, false = failed
bool V30BAmbientLightSensor::i2cReadRegs(byte reg, byte* data, uint length)
{
	// all zeros if it fails
	memset(data, 0, length);

	wire->beginTransmission(i2cAdds);
	wire->write((byte)reg);
	if (wire->endTransmission(false) != 0) {
		LOGERROR("endTransmission() failed 1");
		return false;
	}
	uint n = wire->requestFrom(i2cAdds, length);
	#if 1
	for (uint i = 0; i < length; ++i) {
		int b = wire->read();
		if (b < 0) {
			LOGERROR("readBytes() failed");
			return false;
		}
		*data++ = (byte)b;
	}
	#else
	if (wire->readBytes(data, length) != length) {
		LOGERROR("readBytes() failed");
		return false;
	}
	#endif
	return true;
}

// Write one or more registers
// returns : true = success, false = failed
bool V30BAmbientLightSensor::i2cWriteRegs(uint reg, const byte* data, uint length)
{
	wire->beginTransmission(i2cAdds);
	wire->write((byte)reg);
	#if 1
	for (uint i = 0; i < length; ++i) {
		wire->write(*data++);
	}
	#else
		wire->write(data, length);
	#endif
	if (wire->endTransmission() != 0) {
		LOGERROR("endTransmission() failed 1");
		return false;
	}
	return true;
}

#else

// Read one or more registers
// returns : true = success, false = failed
bool V30BAmbientLightSensor::i2cReadRegs(byte reg, byte* data, uint length)
{
	// all zeros if it fails
	memset(data, 0, length);

	i2cSendStartStop(true);
	bool ok = i2cSendByte(i2cAdds << 1);
	if (ok)
		ok = i2cSendByte(reg);
	if (ok) {
		// repeated start condition
		i2cSendStartStop(true);
		ok = i2cSendByte((i2cAdds << 1) | 1);
		if (ok) {
			delay(10);
			for (uint i = 0; i < length; i++) {
				data[i] = i2cRecvByte();
				i2cSendAck(i != length - 1);
			}
		}
	}
	digitalWrite(sda, 0);
	i2cSendStartStop(false);
	return ok;
}

// Write one or more registers
// returns : true = success, false = failed
bool V30BAmbientLightSensor::i2cWriteRegs(uint reg, const byte* data, uint length)
{
	i2cSendStartStop(true);
	bool ok = i2cSendByte(i2cAdds << 1);
	if (ok)
		ok = i2cSendByte(reg);
	if (ok) {
		for (int i = 0; i < length; ++i) {
			ok = i2cSendByte(*data++);
			if (!ok)
				break;
		}
	}
	digitalWrite(sda, 0);
	i2cSendStartStop(false);
	return ok;
}


///////////////////////////////////////////////////////////////////////////////
// Bit-banged Software I2C, this works well
// modified from this DFRobot code
// https://github.com/DFRobot/DFRobot_B_LUX_V30B
// Copyright (c) 2010 DFRobot Co.Ltd, MIT License

// start bit : scl=H, sda=H->L
// stop bit  : scl=H, sda=L->H
void V30BAmbientLightSensor::i2cSendStartStop(bool start)
{
	digitalWrite(scl, 1);
	digitalWrite(sda, start);
	delayMicroseconds(usDelay);
	digitalWrite(sda, !start);
	delayMicroseconds(usDelay);
	digitalWrite(scl, 0);
	delayMicroseconds(usDelay);
}

void V30BAmbientLightSensor::i2cSendAck(bool ack)
{
	pinMode(sda, OUTPUT);
	// sda : 0 = ACK, 1 = NAK
	digitalWrite(sda, !ack);
	digitalWrite(scl, 1);
	delayMicroseconds(usDelay);
	digitalWrite(scl, 0);
	delayMicroseconds(usDelay);
}

// returns : true = ACK, false = NAK
bool V30BAmbientLightSensor::i2cRecvAck(void)
{
	pinMode(sda, INPUT);
	digitalWrite(scl, 1);
	delayMicroseconds(usDelay);
	// sda : 0 = ACK, 1 = NAK
	bool ack = !digitalRead(sda);
	digitalWrite(scl, 0);
	digitalWrite(sda, 1);
	pinMode(sda, OUTPUT);
	delayMicroseconds(usDelay);
	return ack;
}

// returns the byte
byte V30BAmbientLightSensor::i2cRecvByte()
{
	pinMode(sda, INPUT_PULLUP);
	// clock in 8 bits
	byte b = 0;
	for (uint i = 0; i < 8; i++) {
		b <<= 1;
		digitalWrite(scl, 1);
		delayMicroseconds(usDelay);
		b |= digitalRead(sda);
		digitalWrite(scl, 0);
		delayMicroseconds(usDelay);
	}
	pinMode(sda, OUTPUT);
	return b;
}

// returns : true = ACK, false = NAK
bool V30BAmbientLightSensor::i2cSendByte(byte data)
{
	// clock out 8 bits
	for (uint i = 0; i < 8; i++) {
		digitalWrite(sda, data & 0x80 ? 1 : 0);
		delayMicroseconds(usDelay);
		digitalWrite(scl, 1);
		delayMicroseconds(usDelay);
		digitalWrite(scl, 0);
		delayMicroseconds(usDelay);
		data <<= 1;
	}
	return i2cRecvAck();
}

#endif

