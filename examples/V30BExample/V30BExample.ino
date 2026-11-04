///////////////////////////////////////////////////////////////////////////////
// Example sketch for DFRobot B_LUX_V30B Ambient Light Sensor Library
// SKU: SEN0390
// Copyright (C) https://muman.ch + https://github.com/mumanchu, 2026.11.04
// Copyright (c) 2010 DFRobot Co.Ltd, MIT License
// If you re-use this software, please include the above copyright notices
// 
// For details see the github repository
// https://github.com/V30BAmbientLightSensor

#define DEBUG

// Uncomment this if you want to try the hardware I2C, I couldn't make it work
//#define V30B_USE_HARDWARE_I2C

#ifdef V30B_USE_HARDWARE_I2C
#include <Wire.h>
#endif

#include "V30BAmbientLightSensor.h"
V30BAmbientLightSensor als;


void setup() 
{
	// use different pins for Serial logging
	// this is only for the (really nice) STM32 Nucleo-64 boards
	#ifdef ARDUINO_NUCLEO_64
	Serial.setTx(PC_10);
	Serial.setRx(PC_11);
	#endif

	Serial.begin(115200);
	delay(3000);

	// PuTTY clear screen and scrollback
	// comment out if not using PuTTY
	Serial.print("\033[2J\033[H\033[3J");

	Serial.println("\n\rV30B Ambient Light Sensor Started\n\r");
	Serial.flush();


	#ifdef V30B_USE_HARDWARE_I2C

	// hardware I2C did not work for me
	Wire.begin();
	Wire.setClock(100000);
	Wire.setTimeout(100);

	if (!als.begin(&Wire)) {
		Serial.println("als.begin() failed");
		Serial.flush();
		while (1) yield();
	}
	#else

	// software I2C works very well
	if (!als.begin(SDA, SCL)) {
		Serial.println("als.begin() failed");
		Serial.flush();
		while (1) yield();
	}
	#endif

	// Set to 1 to enable the EEPROM tests
	#if 0

	///////////////////////////////////////////////////////////////////////////
	// 112 bytes of EEPROM can be used by the application (I think)
	const uint nbytes = 112;
	byte eeprom[nbytes];

	// all values are zero by default
	bool ok = als.readEEPROM(0, eeprom, nbytes);
	if (!ok) {
		Serial.println("als.readEEPROM() failed");
		Serial.flush();
		while (1) yield();
	}

	// write random values
	srand(1234);
	for (int i = 0; i < nbytes; ++i)
		eeprom[i] = (byte)rand();
	ok = als.writeEEPROM(0, eeprom, nbytes);
	if (!ok) {
		Serial.println("als.writeEEPROM() failed");
		Serial.flush();
		while (1) yield();
	}

	// verify
	ok = als.readEEPROM(0, eeprom, nbytes);
	if (!ok) {
		Serial.println("als.readEEPROM() failed");
		Serial.flush();
		while (1) yield();
	}
	srand(1234);
	for (int i = 0; i < nbytes; ++i) {
		if (eeprom[i] != (byte)rand()) {
			Serial.println("EEPROM verify error");
			Serial.flush();
			break;
		}
	}

	// reset to zeros
	memset(eeprom, 0x00, nbytes);
	ok = als.writeEEPROM(0, eeprom, nbytes);
	if (!ok) {
		Serial.println("als.writeEEPROM() failed");
		Serial.flush();
		while (1) yield();
	}

	Serial.println("EEPROM test passed\n\r");
	Serial.flush();
	#endif
}


void loop() 
{
	char buf[32];

	long lux;
	if (als.readLux(&lux)) {
		sprintf(buf, "%-8ld  LUX  %-6ld ", millis() / 1000, lux);
		Serial.print(buf);
	}
	float flux;
	if (als.readLux(&flux)) {
		sprintf(buf, "%.03f", flux);
		Serial.print(buf);
	}
	Serial.println();
	Serial.flush();

	delay(1000);
}
