/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <atomic>

#include "System/Sound/PCMStream.h"

class NullPCMStream final : public IPCMStream {
public:
	bool Queue(PCMBlock&& block) override { return true; }
	void Play() override {}
	void Pause() override {}
	void Flush(std::int64_t positionUs) override { position = positionUs; }
	void SetVolume(float volume) override {}
	void Close() override {}
	bool IsAudible() const override { return false; }
	bool IsClosed() const override { return false; }
	std::int64_t GetPositionUs() const override { return position; }

private:
	std::atomic<std::int64_t> position = 0;
};
