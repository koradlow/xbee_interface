// University of Southampton, 2012
// EMECS Group Design Project

// Structs definitions for messages that will be exchanged between the
// monitoring device and the base station are stored here
// This is to ensure message content is consistent across both devices
// while serializing and deserializing raw data

#ifndef __MESSAGETYPES_H
#define __MESSAGETYPES_H

#include <stdint.h>
#include <stdbool.h>

// IAR CC uses a different keyword for packed structures
#if defined ( __ICCARM__ )
  #define PACKEDSTRUCT __packed struct
#else
  #define PACKEDSTRUCT struct __attribute__((__packed__))
#endif

enum MessageType {
	messageTypeGPS,
	messageTypeAccelerometer,
	messageTypeTemperature,
	messageTypeRawTemperature,
	messageTypeHeartRate
};

typedef PACKEDSTRUCT {
	uint8_t degree;
	uint8_t minute;
	uint8_t second;
} Coordinate;

typedef PACKEDSTRUCT {
	Coordinate latitude;
	bool latitudeNorth;
	Coordinate longitude;
	bool longitudeWest;
	bool validPosFix;
} GPSMessage;

typedef PACKEDSTRUCT {
	int8_t x;
	int8_t y;
	int8_t z;
} AccelerometerMessage;

typedef PACKEDSTRUCT {
	double Tobj;
} TemperatureMessage;

typedef PACKEDSTRUCT {
	double Vobj;
	double Tenv;
} RawTemperatureMessage;

typedef PACKEDSTRUCT {
	uint8_t bpm;
} HeartRateMessage;

#endif	// __MESSAGETYPES_H