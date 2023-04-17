#ifndef SOUNDDECODERS_H
#define SOUNDDECODERS_H

#include <variant>
#include <string>
#include <vector>

#include <al.h>
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>


class OggDecoder {
public:
	bool LoadFile(const std::string& path);
	ALenum GetFormat() const;
	long GetRate() const;
	long Read(char *buffer,int length, int bigendianp,int word,int sgned,int *bitstream);
	void DisplayInfo();
	float GetTotalTime();
	bool Valid() const;
	void Stop();
	void ReleaseBuffers();
private:
	std::vector<std::string> vorbisTags;
	std::string vendor;
	OggVorbis_File ovFile;
	vorbis_info* vorbisInfo=nullptr;
};

#endif // SOUNDDECODERS_H
