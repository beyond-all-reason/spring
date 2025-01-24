/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BUFFERED_ARCHIVE_H
#define _BUFFERED_ARCHIVE_H

#include <atomic>
#include <optional>
#include <tuple>

#include "IArchive.h"
#include "System/Threading/SpringThreading.h"

/**
 * Provides a helper implementation for archive types that can only uncompress
 * one file to memory at a time.
 */
class CBufferedArchive : public IArchive
{
public:
	CBufferedArchive(const std::string& name, bool cached = true): IArchive(name) {
		noCache = !cached;
	}

	virtual ~CBufferedArchive();

	virtual int GetType() const override { return ARCHIVE_TYPE_BUF; }

	bool GetFile(unsigned int fid, std::vector<std::uint8_t>& buffer) override;

protected:
	virtual int GetFileImpl(unsigned int fid, std::vector<std::uint8_t>& buffer) = 0;

	// indexed by file-id
	std::vector<std::tuple<std::atomic<uint32_t>, std::optional<std::vector<uint8_t>>>> fileCache;
private:
	spring::spinlock mutex;
	bool noCache = false;
};

#endif // _BUFFERED_ARCHIVE_H
