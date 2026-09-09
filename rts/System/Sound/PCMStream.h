/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>
#include <vector>

struct PCMBlock {
	std::vector<std::int16_t> samples;
	int sampleRate = 48000;
	int channels = 2;
	std::int64_t ptsUs = 0;
	bool endOfStream = false;
};

class IPCMStream {
public:
	virtual ~IPCMStream() = default;

	virtual bool Queue(PCMBlock&& block) = 0;
	virtual void Play() = 0;
	virtual void Pause() = 0;
	virtual void Flush(std::int64_t positionUs) = 0;
	virtual void SetVolume(float volume) = 0;
	virtual void Close() = 0;
	virtual bool IsAudible() const = 0;
	virtual bool IsClosed() const = 0;
	virtual std::int64_t GetPositionUs() const = 0;
};
