/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MUSIC_STREAM_H
#define MUSIC_STREAM_H

#include "System/Misc/SpringTime.h"
#include "System/Sound/OpenAL/OggDecoder.h"
#include "System/Sound/OpenAL/Mp3Decoder.h"
#include "System/FileSystem/FileHandler.h"

#include <al.h>
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>

#include <array>
#include <string>
#include <variant>


class MusicStream
{
public:
	MusicStream();
	~MusicStream();

	MusicStream(const MusicStream& rhs) = delete;
	MusicStream& operator=(const MusicStream& rhs) = delete;

	MusicStream(MusicStream&& rhs) = delete;
	MusicStream& operator=(MusicStream&& rhs) = delete;

	void Play(const std::string& path, float volume, ALuint src);
	void Stop();
	void Update();

	float GetPlayTime() const { return (msecsPlayed.toSecsf()); }
	float GetTotalTime() const { return totalTime; }

	bool TogglePause();
	bool Valid() const { return source != 0; }
	bool IsFinished() { return !Valid() || (GetPlayTime() >= GetTotalTime()); }

private:
	bool IsPlaying();
	bool StartPlaying();

	bool DecodeStream(ALuint buffer);
	void EmptyBuffers();
	void ReleaseBuffers();

	/**
	 * @brief Decode next part of the stream and queue it for playing
	 * @return whether it is the end of the stream
	 *   (check for IsPlaying() whether the complete stream was played)
	 */
	bool UpdateBuffers();

	static constexpr unsigned int NUM_BUFFERS = 2;

	std::vector<uint8_t> pcmDecodeBuffer;

	std::array<ALuint, NUM_BUFFERS> buffers;
	ALuint source;
	ALenum format;

	bool stopped;
	bool paused;

	spring_time msecsPlayed;
	spring_time lastTick;
	float totalTime;

	std::variant <OggDecoder, Mp3Decoder> decoder;
	CFileHandler fileBuffer;
};

#endif // MUSIC_STREAM_H
