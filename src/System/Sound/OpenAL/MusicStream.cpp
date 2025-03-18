/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "MusicStream.h"

#include "System/Sound/SoundLog.h"
#include "System/SafeUtil.h"
#include "ALShared.h"
#include "VorbisShared.h"

#include <filesystem>
#include <string_view>
#include <variant>


MusicStream::MusicStream()
	: buffers{}
	, source(0)
	, format(AL_FORMAT_MONO16)
	, stopped(true)
	, paused(false)
	, msecsPlayed(spring_nulltime)
	, lastTick(spring_nulltime)
	, totalTime(0.0f)
{
}

MusicStream::~MusicStream()
{
	Stop();
}

// open a music stream from a given file and start playing it
void MusicStream::Play(const std::string& path, float volume, ALuint src)
{
	// we're already playing another stream
	if (!stopped)
		return;

	fileBuffer.Close();
	fileBuffer.Open(path);

	if (!fileBuffer.FileExists()) {
		LOG_L(L_ERROR, "[MusicStream::Play] File doesn't exist: %s", path.c_str());
		return;
	}

	source = src;

	if (fileBuffer.GetFileExt() == std::string_view{"mp3"}) {
		decoder = Mp3Decoder();
	} else {
		assert(fileBuffer.GetFileExt() == "ogg");
		decoder = OggDecoder();
	}

	if (!fileBuffer.IsBuffered()) {
		auto& buf = fileBuffer.GetBuffer();
		buf.resize(fileBuffer.FileSize());
		fileBuffer.Read(buf.data(), fileBuffer.FileSize());
	}

	const bool loaded = std::visit([&](auto&& d) {
			return d.LoadData(fileBuffer.GetBuffer().data(), fileBuffer.FileSize());
			} , decoder);
	if (!loaded) {
		LOG_L(L_ERROR, "[MusicStream::Play] Could not load file: %s", path.c_str());
		source = 0; // invalidate
		assert(!Valid());
		return;
	}

	int channels = std::visit([&](auto&& d) { return d.GetChannels(); }, decoder);
	format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

	totalTime = std::visit([&](auto&& d) { return d.GetTotalTime(); }, decoder);

	alGenBuffers(2, buffers.data());
	CheckError("[MusicStream::Play][1]");

	if (!StartPlaying()) {
		LOG_L(L_ERROR, "[MusicStream::Play] Failed to decode: %s",
			  path.c_str());
		ReleaseBuffers();
	} else {
		stopped = false;
		paused  = false;
	}

	CheckError("[MusicStream::Play][2]");
}

// stops the currently playing stream
void MusicStream::Stop()
{
	if (stopped)
		return;

	ReleaseBuffers();

	msecsPlayed = spring_nulltime;
	totalTime = 0.0f;
	lastTick = spring_gettime();

	format = 0;
	stopped = true;
	paused = false;

	assert(!Valid());
}

// clean up the OpenAL resources
void MusicStream::ReleaseBuffers()
{
	if (!source)
		return;

#if 0
	EmptyBuffers();
#else
	// alDeleteBuffers fails with AL_INVALID_OPERATION if either buffer
	// is still bound to source, while alSourceUnqueueBuffers sometimes
	// generates an AL_INVALID_VALUE but doesn't appear to be necessary
	// since we can just detach both of them directly
	alSourcei(source, AL_BUFFER, AL_NONE);
	CheckError("[MusicStream::ReleaseBuffers][1]");
#endif

	alDeleteBuffers(2, buffers.data());
	CheckError("[MusicStream::ReleaseBuffers][2]");
	std::fill(buffers.begin(), buffers.end(), 0);

	source = 0;
	assert(!Valid());
	assert(IsFinished());
}


// returns true if both buffers were
// filled with data from the stream
bool MusicStream::StartPlaying()
{
	msecsPlayed = spring_nulltime;
	lastTick = spring_gettime();
	
	if (!DecodeStream(buffers[0])) {
		return false;
	}
	
	int numDecodedStreams = 1;
	if (DecodeStream(buffers[1])) {
		++numDecodedStreams;
	} else {
		// small file or broken stream
	}
	
	alSourceQueueBuffers(source, numDecodedStreams, buffers.data());

	// CheckError returns true if *no* error occurred
	if (!CheckError("[MusicStream::StartPlaying][1]"))
		return false;

	alSourcePlay(source);
	return (CheckError("[MusicStream::StartPlaying][2]"));
}


// returns true if we're still playing
bool MusicStream::IsPlaying()
{
	ALenum state = 0;
	alGetSourcei(source, AL_SOURCE_STATE, &state);

	return (state == AL_PLAYING);
}

bool MusicStream::TogglePause()
{
	if (!stopped)
		paused = !paused;

	return paused;
}


// pop the processed buffers from the queue,
// refill them, and push them back in line
bool MusicStream::UpdateBuffers()
{
	int buffersProcessed = 0;
	bool active = true;

	alGetSourcei(source, AL_BUFFERS_PROCESSED, &buffersProcessed);

	while (buffersProcessed-- > 0) {
		ALuint buffer;

		alSourceUnqueueBuffers(source, 1, &buffer);
		CheckError("[MusicStream::UpdateBuffers][1]");

		// false if we've reached end of stream
		if ((active = DecodeStream(buffer))) {
			alSourceQueueBuffers(source, 1, &buffer);
			CheckError("[MusicStream::UpdateBuffers][2]");
		}
	}

	return (active && CheckError("[MusicStream::UpdateBuffers][3]"));
}


void MusicStream::Update()
{
	if (stopped)
		return;

	const spring_time tick = spring_gettime();

	if (!paused) {
		// releasing buffers is only allowed once the source has actually
		// stopped playing, since it might still be reading from the last
		// decoded chunk
		UpdateBuffers();
		if (!IsPlaying())
			ReleaseBuffers();

		msecsPlayed += (tick - lastTick);
	}

	lastTick = tick;
}


// read decoded data from audio stream into PCM buffer
bool MusicStream::DecodeStream(ALuint buffer)
{
	pcmDecodeBuffer.resize(DECODE_BUFFER_SIZE);

	int size = 0;
	int section = 0;
	int result = 0;

	while (size < static_cast<int>(pcmDecodeBuffer.size())) {
		result = std::visit([&](auto&& d) {
				return d.Read(pcmDecodeBuffer.data() + size, pcmDecodeBuffer.size() - size, 0, 2, 1, &section);
				}, decoder);

		if (result > 0) {
			size += result;
			continue;
		}

		if (result < 0) {
			LOG_L(L_WARNING, "Error reading Ogg stream (%s)", ErrorString(result).c_str());
			continue;
		}

		break;
	}

	if (size == 0)
		return false;

	long rate = std::visit([&](auto&& d) { return d.GetRate(); }, decoder);
	alBufferData(buffer, format, pcmDecodeBuffer.data(), size, rate);
	return (CheckError("[MusicStream::DecodeStream]"));
}


// dequeue any buffers pending on source (unused, see ReleaseBuffers)
void MusicStream::EmptyBuffers()
{
	assert(source != 0);

#if 1
	int queuedBuffers = 0;

	alGetSourcei(source, AL_BUFFERS_QUEUED, &queuedBuffers);
	CheckError("[MusicStream::EmptyBuffers][1]");

	while (queuedBuffers-- > 0) {
		ALuint buffer;

		alSourceUnqueueBuffers(source, 1, &buffer);
		CheckError("[MusicStream::EmptyBuffers][2]");
		// done by caller
		// alDeleteBuffers(1, &buffer);
	}
#else
	// assumes both are still pending
	alSourceUnqueueBuffers(source, 2, buffers);
	CheckError("[MusicStream::EmptyBuffers]");
#endif
}
