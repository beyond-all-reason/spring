/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BufferedArchive.h"
#include "System/GlobalConfig.h"
#include "System/MainDefines.h"
#include "System/Log/ILog.h"

#include <cassert>

CBufferedArchive::~CBufferedArchive()
{
	uint32_t   cachedSize = 0;
	uint32_t uncachedSize = 0;
	uint32_t fileCount = 0;

	for (const auto& [numAccessed, gotBuffered, fileData] : fileCache) {
		if (gotBuffered) {
			cachedSize += fileData.size();
			fileCount++;
		} else {
			uncachedSize += fileData.size();
		}
	}

	LOG_L(L_INFO, "[%s][name=%s] %u bytes cached in %u files, %u bytes cached in %u files",
		__func__, archiveFile.c_str(),
		  cachedSize, fileCount,
		uncachedSize, static_cast<uint32_t>(fileCache.size() - fileCount)
	);
}

bool CBufferedArchive::GetFile(uint32_t fid, std::vector<std::uint8_t>& buffer)
{
	assert(IsFileId(fid));

	int ret = 0;

	auto scopedSemAcq = AcquireSemaphoreScoped();

	if (!globalConfig.vfsCacheArchiveFiles || noCache) {
		if ((ret = GetFileImpl(fid, buffer)) != 1)
			LOG_L(L_ERROR, "[BufferedArchive::%s(fid=%u)][noCache=%d,vfsCache=%d] name=%s ret=%d size=" _STPF_, __func__, fid, static_cast<int>(noCache), static_cast<int>(globalConfig.vfsCacheArchiveFiles), archiveFile.c_str(), ret, buffer.size());

		return (ret == 1);
	}

	// NumFiles is virtual, can't do this in ctor
	{
		std::scoped_lock lck(mutex);
		if (fileCache.empty())
			fileCache.resize(NumFiles());
	}

	// numAccessed/gotBuffered are not atomic, and simultaneous access to the same fid will cause issues
	// however, the access pattern is such that each thread accesses a different fid, so this should be fine
	auto& [numAccessed, gotBuffered, fileData] = fileCache[fid];

	numAccessed++;

	if (gotBuffered) {
		buffer.assign(fileData.begin(), fileData.end());
		return true;
	}

	if ((ret = GetFileImpl(fid, buffer)) != 1)
		LOG_L(L_ERROR, "[BufferedArchive::%s(fid=%u)][noCache=%d,vfsCache=%d] name=%s ret=%d size=" _STPF_, __func__, fid, static_cast<int>(noCache), static_cast<int>(globalConfig.vfsCacheArchiveFiles), archiveFile.c_str(), ret, buffer.size());

	if (numAccessed == 2 && (ret == 1)) {
		fileData.assign(buffer.begin(), buffer.end());
		gotBuffered = true;
	}
	
	return (ret == 1);
}
