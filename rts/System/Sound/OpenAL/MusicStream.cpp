/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "MusicStream.h"

#include "System/FileSystem/FileHandler.h"
#include "System/Sound/SoundLog.h"
#include "System/SafeUtil.h"
#include "ALShared.h"
#include "VorbisShared.h"
#include "System/Sound/OpenAL/OggDecoder.h"
#include "System/Sound/OpenAL/Mp3Decoder.h"

#include <cstring> //memset
#include <filesystem>
#include <string_view>
#include <variant>

// NOTE:
//   this buffer gets recycled by each new stream, across *all* audio-channels
//   as a result streams are limited to only ever being played within a single
//   channel (currently BGMusic), but cause far less memory fragmentation
// TODO:
//   can easily be fixed if necessary by giving each channel its own index and
//   passing that along to the callbacks via COggStream{::Play}
// CFileHandler fileBuffers[NUM_AUDIO_CHANNELS];
static CFileHandler fileBuffer("", "");

// FIXME these parts of MusicStream are outside the class because every instance
// of CSoundSource has a copy of MusicStream class but only one is ever active
static std::variant<OggDecoder, Mp3Decoder> decoder;


MusicStream::MusicStream(ALuint _source)
	: source(_source)
	, format(AL_FORMAT_MONO16)
	, stopped(true)
	, paused(false)
	, msecsPlayed(spring_nulltime)
	, lastTick(spring_nulltime)
	, totalTime(0.0f)
{
	std::fill(buffers.begin(), buffers.end(), 0);
}

MusicStream::~MusicStream()
{
	Stop();
}


MusicStream& MusicStream::operator=(MusicStream&& rhs) noexcept
{
	if (this != &rhs) {
		std::swap(pcmDecodeBuffer, rhs.pcmDecodeBuffer);

		for (auto i = 0; i < buffers.size(); ++i) {
			std::swap(buffers[i], rhs.buffers[i]);
		}

		std::swap(source, rhs.source);
		std::swap(format, rhs.format);

		std::swap(stopped, rhs.stopped);
		std::swap(paused, rhs.paused);

		std::swap(msecsPlayed, rhs.msecsPlayed);
		std::swap(lastTick, rhs.lastTick);
		std::swap(totalTime, rhs.totalTime);
	}

	return *this;
}

// open a music stream from a given file and start playing it
void MusicStream::Play(const std::string& path, float volume)
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

	if (fileBuffer.GetFileExt() == std::string_view{"mp3"}) {
		decoder = Mp3Decoder();
	} else {
		assert(fileBuffer.GetFileExt() == "ogg");
		decoder = OggDecoder();
	}

	if (!fileBuffer.IsBuffered()) {
		// TODO move buffer to class scope
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
	lastTick = spring_gettime();

	source = 0;
	format = 0;
	totalTime = 0.0f;

	assert(!Valid());
}

// clean up the OpenAL resources
void MusicStream::ReleaseBuffers()
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
	CheckError("[MusicStream::ReleaseBuffers][1]");
#endif

	alDeleteBuffers(2, buffers.data());
	CheckError("[MusicStream::ReleaseBuffers][2]");
	std::fill(buffers.begin(), buffers.end(), 0);
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
	
	const int numDecodedStreams = 1 + static_cast<int>(DecodeStream(buffers[1]));

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
		if (UpdateBuffers(), !IsPlaying())
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
