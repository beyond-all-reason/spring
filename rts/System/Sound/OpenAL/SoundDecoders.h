#ifndef SOUNDDECODERS_H
#define SOUNDDECODERS_H

#include <string>

#include <al.h>
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>


class OggDecoder {
public:
	long Read(char *buffer, int length, int bigendianp, int word, int sgned, int *bitstream);
	bool LoadFile(const std::string& path);
	void Clear();
	ALenum GetFormat() const;
	long GetRate() const;
	float GetTotalTime();

private:
	void DisplayInfo();
	OggVorbis_File ovFile;
	vorbis_info* vorbisInfo = nullptr;
};

#endif // SOUNDDECODERS_H
