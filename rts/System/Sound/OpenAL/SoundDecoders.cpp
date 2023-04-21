#include "System/Sound/OpenAL/SoundDecoders.h"

#include <cstring> //memset
#include <cassert>

#include "System/FileSystem/FileHandler.h"
#include "System/Sound/SoundLog.h"
#include "VorbisShared.h"

namespace VorbisCallbacks {
	// NOTE:
	//   this buffer gets recycled by each new stream, across *all* audio-channels
	//   as a result streams are limited to only ever being played within a single
	//   channel (currently BGMusic), but cause far less memory fragmentation
	// TODO:
	//   can easily be fixed if necessary by giving each channel its own index and
	//   passing that along to the callbacks via COggStream{::Play}
	// CFileHandler fileBuffers[NUM_AUDIO_CHANNELS];
	CFileHandler fileBuffer("", "");

	size_t VorbisStreamReadCB(void* ptr, size_t size, size_t nmemb, void* datasource)
	{
		assert(datasource == &fileBuffer);
		return fileBuffer.Read(ptr, size * nmemb);
	}

	int VorbisStreamCloseCB(void* datasource)
	{
		assert(datasource == &fileBuffer);
		fileBuffer.Close();
		return 0;
	}

	int VorbisStreamSeekCB(void* datasource, ogg_int64_t offset, int whence)
	{
		assert(datasource == &fileBuffer);

		switch (whence) {
			case SEEK_SET: { fileBuffer.Seek(offset, std::ios_base::beg); } break;
			case SEEK_CUR: { fileBuffer.Seek(offset, std::ios_base::cur); } break;
			case SEEK_END: { fileBuffer.Seek(offset, std::ios_base::end); } break;
			default: {} break;
		}

		return 0;
	}

	long VorbisStreamTellCB(void* datasource)
	{
		assert(datasource == &fileBuffer);
		return (fileBuffer.GetPos());
	}
}

long OggDecoder::Read(char *buffer,int length, int bigendianp,int word,int sgned,int *bitstream) {
	return ov_read(&ovFile, buffer, length, bigendianp, word, sgned, bitstream);
}

bool OggDecoder::LoadFile(const std::string& path) {
	ov_callbacks vorbisCallbacks;
	vorbisCallbacks.read_func  = VorbisCallbacks::VorbisStreamReadCB;
	vorbisCallbacks.close_func = VorbisCallbacks::VorbisStreamCloseCB;
	vorbisCallbacks.seek_func  = VorbisCallbacks::VorbisStreamSeekCB;
	vorbisCallbacks.tell_func  = VorbisCallbacks::VorbisStreamTellCB;

	VorbisCallbacks::fileBuffer.Open(path);

	const int result = ov_open_callbacks(&VorbisCallbacks::fileBuffer, &ovFile, nullptr, 0, vorbisCallbacks);

	if (result < 0) {
		LOG_L(L_WARNING, "Could not open Ogg stream (reason: %s).", ErrorString(result).c_str());
		VorbisCallbacks::fileBuffer.Close();
		return false;
	}

	vorbisInfo = ov_info(&ovFile, -1);

		// DisplayInfo();
	return true;
}

ALenum OggDecoder::GetFormat() const {
	if (vorbisInfo->channels == 1) {
		return AL_FORMAT_MONO16;
	} else {
		return AL_FORMAT_STEREO16;
	}
}

long OggDecoder::GetRate() const {
	return vorbisInfo->rate;
}


float OggDecoder::GetTotalTime()
{
	return ov_time_total(&ovFile, -1);
}

// display Ogg info and comments
void OggDecoder::DisplayInfo()
{
	std::vector<std::string> vorbisTags;

	vorbis_comment* vorbisComment = ov_comment(&ovFile, -1);
	vorbisTags.resize(vorbisComment->comments);

	for (unsigned i = 0; i < vorbisComment->comments; ++i) {
		vorbisTags[i] = std::string(vorbisComment->user_comments[i], vorbisComment->comment_lengths[i]);
	}

	const std::string vendor = std::string(vorbisComment->vendor);
	LOG("[OggStream::%s]", __func__);
	LOG("\tversion:           %d", vorbisInfo->version);
	LOG("\tchannels:          %d", vorbisInfo->channels);
	LOG("\ttime (sec):        %lf", ov_time_total(&ovFile, -1));
	LOG("\trate (Hz):         %ld", vorbisInfo->rate);
	LOG("\tbitrate (upper):   %ld", vorbisInfo->bitrate_upper);
	LOG("\tbitrate (nominal): %ld", vorbisInfo->bitrate_nominal);
	LOG("\tbitrate (lower):   %ld", vorbisInfo->bitrate_lower);
	LOG("\tbitrate (window):  %ld", vorbisInfo->bitrate_window);
	LOG("\tvendor:            %s", vendor.c_str());
	LOG("\ttags:              %lu", static_cast<unsigned long>(vorbisTags.size()));

	for (const std::string& s: vorbisTags) {
		LOG("\t\t%s", s.c_str());
	}
}

void OggDecoder::Clear() {
	if (vorbisInfo) {
		ov_clear(&ovFile);
		vorbisInfo = 0;
	}
}
