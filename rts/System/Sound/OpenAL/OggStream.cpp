/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring> //memset

#include "OggStream.h"

#include "System/FileSystem/FileHandler.h"
#include "System/Sound/SoundLog.h"
#include "System/SafeUtil.h"
#include "ALShared.h"
#include "VorbisShared.h"


COggStream::COggStream(ALuint _source)
	: pcmDecodeBuffer(nullptr)
	, source(_source)
	, format(AL_FORMAT_MONO16)
	, stopped(true)
	, paused(false)
	, decoder(OggDecoder())
{
	std::fill(buffers.begin(), buffers.end(), 0);
}

COggStream::~COggStream()
{
	Stop();
	spring::SafeDeleteArray(pcmDecodeBuffer);
}


COggStream& COggStream::operator=(COggStream&& rhs) noexcept
{
	if (this != &rhs) {
		spring::SafeDeleteArray(pcmDecodeBuffer);
		pcmDecodeBuffer = rhs.pcmDecodeBuffer;
		rhs.pcmDecodeBuffer = nullptr;

		for (auto i = 0; i < buffers.size(); ++i) {
			std::swap(buffers[i], rhs.buffers[i]);
		}

		std::swap(source, rhs.source);
		std::swap(format, rhs.format);

		std::swap(stopped, rhs.stopped);
		std::swap(paused, rhs.paused);

		std::swap(msecsPlayed, rhs.msecsPlayed);
		std::swap(lastTick, rhs.lastTick);

		std::swap(decoder, rhs.decoder);
	}

	return *this;
}

// open an Ogg stream from a given file and start playing it
void COggStream::Play(const std::string& path, float volume)
{
	// we're already playing another stream
	if (!stopped)
		return;

	auto loaded = std::visit([&](auto&& d) { return d.LoadFile(path); }, decoder);
	if (!loaded) {
		return;
	}

	format = std::visit([&](auto&& d) { return d.GetFormat(); }, decoder);

	alGenBuffers(2, buffers.data());
	CheckError("[COggStream::Play][1]");

	if (!StartPlaying()) {
		ReleaseBuffers();
	} else {
		stopped = false;
		paused  = false;
	}

	CheckError("[COggStream::Play][2]");
}

// stops the currently playing stream
void COggStream::Stop()
{
	if (stopped)
		return;

	ReleaseBuffers();

	msecsPlayed = spring_nulltime;
	lastTick = spring_gettime();

	source = 0;
	format = 0;

	std::visit([&](auto&& d) {
			return d.Stop();
			}, decoder);

	assert(!Valid());
}

float COggStream::GetTotalTime()
{
	return std::visit([&](auto&& d) {
			return d.GetTotalTime();
			}, decoder);
}

// clean up the OpenAL resources
void COggStream::ReleaseBuffers()
{
	stopped = true;
	paused = false;

#if 0
	EmptyBuffers();
#else
	// alDeleteBuffers fails with AL_INVALID_OPERATION if either buffer
	// is still bound to source, while alSourceUnqueueBuffers sometimes
	// generates an AL_INVALID_VALUE but doesn't appear to be necessary
	// since we can just detach both of them directly
	alSourcei(source, AL_BUFFER, AL_NONE);
	CheckError("[COggStream::ReleaseBuffers][1]");
#endif

	alDeleteBuffers(2, buffers.data());
	CheckError("[COggStream::ReleaseBuffers][2]");
	std::fill(buffers.begin(), buffers.end(), 0);

	std::visit([&](auto&& d) {
			d.ReleaseBuffers();
			}, decoder);
}


// returns true if both buffers were
// filled with data from the stream
bool COggStream::StartPlaying()
{
	msecsPlayed = spring_nulltime;
	lastTick = spring_gettime();

	if (!DecodeStream(buffers[0]))
		return false;
	if (!DecodeStream(buffers[1]))
		return false;

	alSourceQueueBuffers(source, 2, buffers.data());

	// CheckError returns true if *no* error occurred
	if (!CheckError("[COggStream::StartPlaying][1]"))
		return false;

	alSourcePlay(source);
	return (CheckError("[COggStream::StartPlaying][2]"));
}


// returns true if we're still playing
bool COggStream::IsPlaying()
{
	ALenum state = 0;
	alGetSourcei(source, AL_SOURCE_STATE, &state);

	return (state == AL_PLAYING);
}

bool COggStream::TogglePause()
{
	if (!stopped)
		paused = !paused;

	return paused;
}


// pop the processed buffers from the queue,
// refill them, and push them back in line
bool COggStream::UpdateBuffers()
{
	int buffersProcessed = 0;
	bool active = true;

	alGetSourcei(source, AL_BUFFERS_PROCESSED, &buffersProcessed);

	while (buffersProcessed-- > 0) {
		ALuint buffer;

		alSourceUnqueueBuffers(source, 1, &buffer);
		CheckError("[COggStream::UpdateBuffers][1]");

		// false if we've reached end of stream
		if ((active = DecodeStream(buffer))) {
			alSourceQueueBuffers(source, 1, &buffer);
			CheckError("[COggStream::UpdateBuffers][2]");
		}
	}

	return (active && CheckError("[COggStream::UpdateBuffers][3]"));
}


void COggStream::Update()
{
	if (stopped)
		return;

	const spring_time tick = spring_gettime();

	if (!paused) {
		// releasing buffers is only allowed once the source has actually
		// stopped playing, since it might still be reading from the last
		// decoded chunk
		if (UpdateBuffers(), !IsPlaying())
			ReleaseBuffers();

		msecsPlayed += (tick - lastTick);
	}

	lastTick = tick;
}


// read decoded data from audio stream into PCM buffer
bool COggStream::DecodeStream(ALuint buffer)
{
	if (!pcmDecodeBuffer) { //defer buffer allocation
		pcmDecodeBuffer = new char[BUFFER_SIZE] {0};
	}

	memset(pcmDecodeBuffer, 0, BUFFER_SIZE);

	int size = 0;
	int section = 0;
	int result = 0;

	while (size < BUFFER_SIZE) {
		result = std::visit([&](auto&& d) {
				return d.Read(pcmDecodeBuffer + size, BUFFER_SIZE - size, 0, 2, 1, &section);
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
	alBufferData(buffer, format, pcmDecodeBuffer, size, rate);
	return (CheckError("[COggStream::DecodeStream]"));
}


// dequeue any buffers pending on source (unused, see ReleaseBuffers)
void COggStream::EmptyBuffers()
{
	assert(source != 0);

#if 1
	int queuedBuffers = 0;

	alGetSourcei(source, AL_BUFFERS_QUEUED, &queuedBuffers);
	CheckError("[COggStream::EmptyBuffers][1]");

	while (queuedBuffers-- > 0) {
		ALuint buffer;

		alSourceUnqueueBuffers(source, 1, &buffer);
		CheckError("[COggStream::EmptyBuffers][2]");
		// done by caller
		// alDeleteBuffers(1, &buffer);
	}
#else
	// assumes both are still pending
	alSourceUnqueueBuffers(source, 2, buffers);
	CheckError("[COggStream::EmptyBuffers]");
#endif
}

bool COggStream::Valid() const
{
	return (source != 0 && std::visit([&](auto&& d) { return d.Valid(); }, decoder));
}
