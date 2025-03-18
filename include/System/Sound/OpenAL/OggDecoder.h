/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef OGGDECODER_H
#define OGGDECODER_H

#include <string>

#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include "VorbisShared.h"


class OggDecoder {
public:
	OggDecoder() = default;
	~OggDecoder();

	OggDecoder(OggDecoder&& src) noexcept { *this = std::move(src); }
	OggDecoder(const OggDecoder& src) = delete;
	OggDecoder& operator = (OggDecoder&& src) noexcept;
	OggDecoder& operator = (const OggDecoder& src) = delete;

	long Read(uint8_t *buffer, int length, int bigendianp, int word, int sgned, int *bitstream);
	bool LoadData(const uint8_t* mem, size_t len);
	int GetChannels() const;
	long GetRate() const;
	float GetTotalTime();

private:
	void Clear();
	void DisplayInfo();

	OggVorbis_File ovFile;
	vorbis_info* vorbisInfo = nullptr;
	CStreamBuffer stream;
};

#endif // OGGDECODER_H
