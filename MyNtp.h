/*
 * MyNtp.h
 *
 *  Created on: 2. 7. 2018
 *      Author: Vladimir
 */

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#ifndef MYNTP_H_
#define MYNTP_H_

#define NTP_PACKET_SIZE 48
#define NTP_DEFAULT_LOCAL_PORT 1337
#define NTP_DEFAULT_SERVER "europe.pool.ntp.org"
#define NTP_DEFAULT_SERVER_PORT 123

// time gap in seconds from 01.01.1900 (NTP time) to 01.01.1970 (UNIX time)
#ifndef DIFF1900TO1970
#define DIFF1900TO1970 2208988800UL
#endif

class MyNtp {
private:

	unsigned int UpdateInterval = 12 * 60 * 60 * 1000;  // In ms 12H

	unsigned long CurrentEpoch = 0;      // In s
	unsigned long LastUpdateMs = 0;        // In ms

	byte NtpBuffer[NTP_PACKET_SIZE];

	void requestTime(WiFiUDP &udp);

public:
	MyNtp();
	virtual ~MyNtp();

	bool update(WiFiUDP &udp);

	int getDay();
	int getHour();
	int getMinute();
	int getSecond();

	String getFormattedTime();
	String getFormattedDate();

	unsigned long getEpochTime();

};

#endif /* MYNTP_H_ */
