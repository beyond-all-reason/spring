/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace video {

enum class PlaybackState {
	Opening,
	Ready,
	Playing,
	Paused,
	Seeking,
	Finished,
	Stopped,
	Error,
};

const char* ToString(PlaybackState state);

struct VideoOptions {
	bool autoplay = true;
	bool loop = false;
	bool audio = true;
	float volume = 1.0f;
};

struct VideoInfo {
	PlaybackState state = PlaybackState::Opening;
	int width = 0;
	int height = 0;
	double duration = 0.0;
	double position = 0.0;
	bool hasAudio = false;
	std::string error;
	std::uint64_t droppedFrames = 0;
};

struct VideoFrame {
	int width = 0;
	int height = 0;
	std::int64_t ptsUs = 0;
	std::vector<std::uint8_t> bgra;
};

class VideoPlayer {
public:
	VideoPlayer(const std::string& path, const std::string& vfsModes, const VideoOptions& options);
	~VideoPlayer();

	VideoPlayer(const VideoPlayer&) = delete;
	VideoPlayer& operator=(const VideoPlayer&) = delete;

	void Play();
	void Pause();
	void Stop();
	void Seek(double seconds);
	void SetVolume(float volume);

	VideoInfo GetInfo() const;
	bool TakeDueFrame(VideoFrame& frame);

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};

} // namespace video
