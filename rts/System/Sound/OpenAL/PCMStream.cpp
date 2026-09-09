/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PCMStream.h"

#include <algorithm>

#include "ALShared.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

#include <tracy/Tracy.hpp>

CONFIG(int, VideoPCMQueueMB).defaultValue(2).minimumValue(1).maximumValue(16);

OpenALPCMStream::OpenALPCMStream()
	: maxQueuedBytes(static_cast<std::size_t>(configHandler->GetInt("VideoPCMQueueMB")) * 1024 * 1024)
{}

OpenALPCMStream::~OpenALPCMStream()
{
	// OpenAL resources must have been released by CSound on its audio thread.
	if (source != 0)
		LOG_L(L_ERROR, "[OpenALPCMStream] destroyed before audio-thread cleanup");
}

bool OpenALPCMStream::Queue(PCMBlock&& block)
{
	if (closeRequested || block.sampleRate <= 0 || (block.channels != 1 && block.channels != 2))
		return false;
	const std::size_t bytes = block.samples.size() * sizeof(std::int16_t);
	std::lock_guard<std::mutex> lock(mutex);
	if (queuedBytes + bytes > maxQueuedBytes)
		return false;
	queuedBytes += bytes;
	blocks.emplace_back(std::move(block));
	return true;
}

void OpenALPCMStream::Play() { playing = true; }
void OpenALPCMStream::Pause() { playing = false; }
void OpenALPCMStream::SetVolume(float value) { volume = std::clamp(value, 0.0f, 1.0f); }
void OpenALPCMStream::Close() { closeRequested = true; }

void OpenALPCMStream::Flush(std::int64_t positionUs)
{
	{
		std::lock_guard<std::mutex> lock(mutex);
		blocks.clear();
		queuedBytes = 0;
	}
	flushPositionUs = positionUs;
	flushRequested = true;
	publishedPositionUs = positionUs;
}

void OpenALPCMStream::ApplyFlush()
{
	ZoneScopedNC("PCMStreamFlush", tracy::Color::Gold);
	if (source != 0) {
		alSourceStop(source);
		ALint queued = 0;
		alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
		while (queued-- > 0) {
			ALuint buffer = 0;
			alSourceUnqueueBuffers(source, 1, &buffer);
		}
	}
	bufferSamples.clear();
	basePtsUs = flushPositionUs;
	processedSamples = 0;
	clockStarted = false;
	endOfStream = false;
}

bool OpenALPCMStream::FillBuffer(ALuint buffer)
{
	ZoneScopedNC("PCMStreamFillBuffer", tracy::Color::DarkOrange);
	PCMBlock block;
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (blocks.empty())
			return false;
		block = std::move(blocks.front());
		blocks.pop_front();
		queuedBytes -= block.samples.size() * sizeof(std::int16_t);
	}
	if (block.endOfStream)
		endOfStream = true;
	if (block.samples.empty())
		return false;

	if (!clockStarted) {
		basePtsUs = block.ptsUs;
		sampleRate = block.sampleRate;
		processedSamples = 0;
		clockStarted = true;
	}
	const ALenum format = (block.channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	alBufferData(buffer, format, block.samples.data(), block.samples.size() * sizeof(std::int16_t), block.sampleRate);
	bufferSamples[buffer] = block.samples.size() / block.channels;
	return CheckError("[OpenALPCMStream::FillBuffer]");
}

void OpenALPCMStream::Update(bool suspended)
{
	ZoneScopedNC("PCMStreamUpdate", tracy::Color::Orange);
	if (closed)
		return;
	if (closeRequested) {
		DestroyAL();
		closed = true;
		return;
	}
	if (source == 0) {
		alGenSources(1, &source);
		if (source == 0 || !CheckError("[OpenALPCMStream::Update][source]")) {
			DestroyAL();
			closed = true;
			return;
		}
		alGenBuffers(static_cast<ALsizei>(buffers.size()), buffers.data());
		alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
		alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);
		if (!CheckError("[OpenALPCMStream::Update][buffers]")) {
			DestroyAL();
			closed = true;
			return;
		}
		audible = true;
	}
	if (flushRequested.exchange(false))
		ApplyFlush();

	ALint processed = 0;
	alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
	while (processed-- > 0) {
		ALuint buffer = 0;
		alSourceUnqueueBuffers(source, 1, &buffer);
		processedSamples += bufferSamples[buffer];
		bufferSamples.erase(buffer);
		if (FillBuffer(buffer))
			alSourceQueueBuffers(source, 1, &buffer);
	}

	ALint queued = 0;
	alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
	for (ALuint buffer: buffers) {
		if (bufferSamples.contains(buffer))
			continue;
		if (!FillBuffer(buffer))
			break;
		alSourceQueueBuffers(source, 1, &buffer);
	}

	ALint sampleOffset = 0;
	alGetSourcei(source, AL_SAMPLE_OFFSET, &sampleOffset);
	if (clockStarted)
		publishedPositionUs = basePtsUs + ((processedSamples + sampleOffset) * 1000000LL / sampleRate);

	alSourcef(source, AL_GAIN, volume);
	ALint state = AL_STOPPED;
	alGetSourcei(source, AL_SOURCE_STATE, &state);
	if (!playing || suspended) {
		if (state == AL_PLAYING)
			alSourcePause(source);
	} else if (state != AL_PLAYING) {
		alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
		if (queued >= 2 || (endOfStream && queued > 0))
			alSourcePlay(source);
	}
}

void OpenALPCMStream::DestroyAL()
{
	if (source != 0) {
		alSourceStop(source);
		ALint queued = 0;
		alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
		while (queued-- > 0) {
			ALuint buffer = 0;
			alSourceUnqueueBuffers(source, 1, &buffer);
		}
		alDeleteSources(1, &source);
	}
	if (buffers[0] != 0)
		alDeleteBuffers(static_cast<ALsizei>(buffers.size()), buffers.data());
	buffers.fill(0);
	source = 0;
	audible = false;
	bufferSamples.clear();
}
