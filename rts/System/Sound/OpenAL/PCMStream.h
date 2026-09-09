/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <unordered_map>

#include <al.h>

#include "System/Sound/PCMStream.h"

class OpenALPCMStream final : public IPCMStream {
public:
	OpenALPCMStream();
	~OpenALPCMStream() override;

	bool Queue(PCMBlock&& block) override;
	void Play() override;
	void Pause() override;
	void Flush(std::int64_t positionUs) override;
	void SetVolume(float volume) override;
	void Close() override;
	bool IsAudible() const override { return audible.load(); }
	bool IsClosed() const override { return closeRequested.load() || closed.load(); }
	std::int64_t GetPositionUs() const override { return publishedPositionUs.load(); }

	// All methods below are sound-thread-only.
	void Update(bool suspended);
	void DestroyAL();
	bool CanRemove() const { return closed.load(); }

private:
	bool FillBuffer(ALuint buffer);
	void ApplyFlush();

	static constexpr std::size_t NUM_BUFFERS = 4;

	mutable std::mutex mutex;
	std::deque<PCMBlock> blocks;
	std::size_t queuedBytes = 0;
	std::size_t maxQueuedBytes = 2 * 1024 * 1024;
	std::atomic<std::int64_t> publishedPositionUs = 0;
	std::atomic<float> volume = 1.0f;
	std::atomic<bool> playing = true;
	std::atomic<bool> closeRequested = false;
	std::atomic<bool> flushRequested = false;
	std::atomic<std::int64_t> flushPositionUs = 0;

	std::array<ALuint, NUM_BUFFERS> buffers = {};
	std::unordered_map<ALuint, std::int64_t> bufferSamples;
	ALuint source = 0;
	std::int64_t basePtsUs = 0;
	std::int64_t processedSamples = 0;
	int sampleRate = 48000;
	bool clockStarted = false;
	bool endOfStream = false;
	std::atomic<bool> audible = false;
	std::atomic<bool> closed = false;
};
