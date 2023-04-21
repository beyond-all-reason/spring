/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef OGGDECODER_H
#define OGGDECODER_H

#include <string>

#include <al.h>
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>


class OggDecoder {
public:
	long Read(char *buffer, int length, int bigendianp, int word, int sgned, int *bitstream);
	bool LoadFile(const std::string& path);
	ALenum GetFormat() const;
	long GetRate() const;
	float GetTotalTime();

private:
	void Clear();
	void DisplayInfo();
	OggVorbis_File ovFile;
	vorbis_info* vorbisInfo = nullptr;
};

#endif // OGGDECODER_H
