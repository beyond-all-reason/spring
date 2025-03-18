/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Sound/OpenAL/OggDecoder.h"

#include <cstring> //memset
#include <cassert>

#include "System/FileSystem/FileHandler.h"
#include "System/Sound/SoundLog.h"
#include "VorbisShared.h"


namespace {
namespace VorbisCallbacks {
	size_t VorbisStreamReadCB(void* ptr, size_t size, size_t nmemb, void* datasource)
	{
		CStreamBuffer* mem = static_cast<CStreamBuffer*>(datasource);
		return mem->Read(ptr, size * nmemb);
	}

	int VorbisStreamCloseCB(void* datasource)
	{
		return 0;
	}

	int VorbisStreamSeekCB(void* datasource, ogg_int64_t offset, int whence)
	{
		CStreamBuffer* mem = static_cast<CStreamBuffer*>(datasource);
		switch (whence) {
			case SEEK_SET: { mem->Seek(offset); } break;
			case SEEK_CUR: { mem->Seek(offset + mem->GetPos()); } break;
			case SEEK_END: { mem->Seek(offset + mem->size); } break;
			default: {} break;
		}

		return 0;
	}

	long VorbisStreamTellCB(void* datasource)
	{
		CStreamBuffer* mem = static_cast<CStreamBuffer*>(datasource);
		return (mem->GetPos());
	}
}
}

long OggDecoder::Read(uint8_t *buffer,int length, int bigendianp,int word,int sgned,int *bitstream)
{
	return ov_read(&ovFile, reinterpret_cast<char*>(buffer), length, bigendianp, word, sgned, bitstream);
}

// TODO make this function accept FileHandler
bool OggDecoder::LoadData(const uint8_t* mem, size_t len)
{
	stream = CStreamBuffer(mem, len);

	ov_callbacks vorbisCallbacks;
	vorbisCallbacks.read_func  = VorbisCallbacks::VorbisStreamReadCB;
	vorbisCallbacks.close_func = VorbisCallbacks::VorbisStreamCloseCB;
	vorbisCallbacks.seek_func  = VorbisCallbacks::VorbisStreamSeekCB;
	vorbisCallbacks.tell_func  = VorbisCallbacks::VorbisStreamTellCB;

	const int result = ov_open_callbacks(&stream, &ovFile, nullptr, 0, vorbisCallbacks);

	if (result < 0) {
		LOG_L(L_WARNING, "Could not open Ogg stream (reason: %s).", ErrorString(result).c_str());
		return false;
	}

	vorbisInfo = ov_info(&ovFile, -1);

	// DisplayInfo();
	return true;
}

int OggDecoder::GetChannels() const
{
	return vorbisInfo->channels;
}

long OggDecoder::GetRate() const
{
	return vorbisInfo->rate;
}

float OggDecoder::GetTotalTime()
{
	// for non-seekable streams, ov_time_total returns OV_EINVAL (-131) while
	// ov_time_tell always[?] returns the decoding time offset relative to EOS
	return (ov_seekable(&ovFile) == 0) ? ov_time_tell(&ovFile): ov_time_total(&ovFile, -1);
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

void OggDecoder::Clear()
{
	if (vorbisInfo) {
		ov_clear(&ovFile);
		vorbisInfo = 0;
	}
}

OggDecoder::~OggDecoder()
{
	Clear();
}

OggDecoder& OggDecoder::operator = (OggDecoder&& src) noexcept
{
	std::swap(ovFile, src.ovFile);
	std::swap(vorbisInfo, src.vorbisInfo);
	std::swap(stream, src.stream);
	return *this;
}
