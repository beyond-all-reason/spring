/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#include "System/Sound/OpenAL/Mp3Decoder.h"

#include "System/Sound/SoundLog.h"
#include "lib/dr_mp3/dr_mp3.h"

int Mp3Decoder::GetChannels() const { return data.channels; }

long Mp3Decoder::GetRate() const { return data.sampleRate; }

long Mp3Decoder::Read(uint8_t* buffer, int length, int bigendianp, int word, int sgned, int* bitstream)
{
	auto bytesToFrames = sizeof(drmp3_int16) / sizeof(*buffer);
	return drmp3_read_pcm_frames_s16(
	           &data, length / bytesToFrames / data.channels, reinterpret_cast<drmp3_int16*>(buffer)) *
	       bytesToFrames * data.channels;
}

float Mp3Decoder::GetTotalTime()
{
	// linear complexity, but is cached in MusicStream
	return 1.0f * drmp3_get_pcm_frame_count(&data) / data.sampleRate;
}

bool Mp3Decoder::LoadData(const uint8_t* mem, size_t len)
{
	if (!drmp3_init_memory(&data, mem, len, nullptr)) {
		LOG_L(L_ERROR, "[Mp3Decoder::LoadFile] Failed to load");
		return false;
	}
	return true;
}
