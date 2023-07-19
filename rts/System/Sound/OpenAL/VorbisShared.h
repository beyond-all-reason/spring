/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VORBIS_SHARED
#define VORBIS_SHARED

#include <cstdint>
#include <cstring>
#include <string>

std::string ErrorString(int code);

// Simple adapter based on std::streambuf for use with stream interfaces
struct CStreamBuffer {
	CStreamBuffer() = default;

	CStreamBuffer(const uint8_t* addr, size_t length)
		: begin(addr)
		, pos(0)
		, size(length) { }

	size_t Read(void* dst, size_t bufSize) {
		auto maxRead = std::min(size - pos, bufSize);
		memcpy(dst, begin + pos, maxRead);
		pos += maxRead;
		return maxRead;
	}

	void Seek(size_t toPos) {
		pos = std::min(toPos, size);
	}

	size_t GetPos() const { return pos; }

	const uint8_t* begin = nullptr;
	size_t   pos   = 0;
	size_t   size  = 0;
};

#endif
