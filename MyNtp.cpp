/*
 * MyNtp.cpp
 *
 *  Created on: 2. 7. 2018
 *      Author: Vladimir
 */

#include "MyNtp.h"

#define DEBUG 50


MyNtp::MyNtp() {
}

MyNtp::~MyNtp() {
}

bool MyNtp::update(WiFiUDP &udp) {
	if (WiFi.status() != WL_CONNECTED) {
#if DEBUG > 1
		Serial.println("MyNTP: WiFi Not Connected");
#endif
		return false; /* not connected */
	}

	if ((millis() - this->LastUpdateMs < this->UpdateInterval)
			&& this->LastUpdateMs != 0) {
#if DEBUG > 50
		Serial.println("MyNTP: No need to update");
#endif
		return false; /* no need to update */
	}

	udp.begin(NTP_DEFAULT_LOCAL_PORT);

#if DEBUG > 1
		Serial.println("MyNTP: Request NTP server");
#endif

	this->requestTime(udp);

	// Wait till data is there or timeout...
	byte timeout = 0;
	int cb = 0;
	do {
		delay(10);
		cb = udp.parsePacket();
		if (timeout > 100)
			return false; // timeout after 1000 ms
		timeout++;
	} while (cb == 0);

	this->LastUpdateMs = millis() - (10 * (timeout + 1)); // Account for delay in reading the time

	udp.read(this->NtpBuffer, NTP_PACKET_SIZE);

#if DEBUG > 1
		Serial.println("MyNTP: Response received");
#endif

	unsigned long highWord = word(this->NtpBuffer[40], this->NtpBuffer[41]);
	unsigned long lowWord = word(this->NtpBuffer[42], this->NtpBuffer[43]);
	// combine the four bytes (two words) into a long integer
	// this is NTP time (seconds since Jan 1 1900):
	unsigned long secsSince1900 = highWord << 16 | lowWord;

	this->CurrentEpoch = secsSince1900 - DIFF1900TO1970;

#if DEBUG > 1
		Serial.println("MyNTP: updated: " + this->getFormattedTime());
#endif
	return true;
}

unsigned long MyNtp::getEpochTime() {
	return (this->CurrentEpoch + ((millis() - this->LastUpdateMs) / 1000));
}

int MyNtp::getDay() {
	return (((this->getEpochTime() / 86400L) + 4) % 7); //0 is Sunday
}
int MyNtp::getHour() {
	return ((this->getEpochTime() % 86400L) / 3600);
}
int MyNtp::getMinute() {
	return ((this->getEpochTime() % 3600) / 60);
}
int MyNtp::getSecond() {
	return (this->getEpochTime() % 60);
}

String MyNtp::getFormattedTime() {
	unsigned long rawTime = this->getEpochTime();
	unsigned long hours = (rawTime % 86400L) / 3600;
	String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

	unsigned long minutes = (rawTime % 3600) / 60;
	String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

	unsigned long seconds = rawTime % 60;
	String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

	return hoursStr + ":" + minuteStr + ":" + secondStr;
}

void MyNtp::requestTime(WiFiUDP &udp) {
	// set all bytes in the buffer to 0
	memset(this->NtpBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	this->NtpBuffer[0] = 0b11100011;   // LI, Version, Mode
	this->NtpBuffer[1] = 0;     // Stratum, or type of clock
	this->NtpBuffer[2] = 6;     // Polling Interval
	this->NtpBuffer[3] = 0xEC;  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	this->NtpBuffer[12] = 49;
	this->NtpBuffer[13] = 0x4E;
	this->NtpBuffer[14] = 49;
	this->NtpBuffer[15] = 52;

	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:
	udp.beginPacket(NTP_DEFAULT_SERVER, NTP_DEFAULT_SERVER_PORT); //NTP requests are to port 123
	udp.write(this->NtpBuffer, NTP_PACKET_SIZE);
	udp.endPacket();
}

