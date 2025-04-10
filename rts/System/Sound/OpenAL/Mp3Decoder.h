/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MP3DECODER_H
#define MP3DECODER_H

#include "lib/dr_mp3/dr_mp3.h"

#include <cstdint>

class Mp3Decoder {
public:
	long Read(uint8_t* buffer, int length, int, int, int, int*);
	bool LoadData(const uint8_t* mem, size_t len);
	int GetChannels() const;
	long GetRate() const;
	float GetTotalTime();

private:
	drmp3 data;
};

#endif // MP3DECODER_H
